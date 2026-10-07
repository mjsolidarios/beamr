#include "sendercontroller.h"

#include <QGuiApplication>
#include <QHostAddress>
#include <QImage>
#include <QMutex>
#include <QNetworkInterface>
#include <QQmlEngine>
#include <QQuickImageProvider>
#include <QRegularExpression>
#include <QSettings>
#include <QSslSocket>
#include <QStringList>
#include <QStyleHints>
#include <QSysInfo>
#include <QTimer>
#include <QUuid>

#include <algorithm>

#ifdef Q_OS_ANDROID
#include <QCoreApplication>
#include <QJniEnvironment>
#include <QJniObject>
#include <QPermissions>
#endif

#include <beamr/config.h>
#include <beamr/connectlink.h>
#include <beamr/log.h>
#include <beamr/protocol.h>
#include <beamr/tlsidentity.h>

#include "discoveryclient.h"
#include "receiversession.h"
#include "streamsender.h"

namespace protocol = beamr::protocol;

namespace {

constexpr int kMaxRecent = 5;
// Each receiver gets its own copy of the video, ~8 Mbit/s apiece; four is
// about what phone Wi-Fi uploads sustain.
constexpr int kMaxReceivers = 4;

const QString kDeviceIdKey = QStringLiteral("device/id");
const QString kDeviceNameKey = QStringLiteral("device/name");
const QString kRecentKey = QStringLiteral("recentReceivers");
const QString kColorSchemeKey = QStringLiteral("appearance/colorScheme");
const QString kShareAudioKey = QStringLiteral("cast/shareAudio");
const QString kQualityKey = QStringLiteral("cast/quality");
const QString kOnboardingDoneKey = QStringLiteral("onboarding/done");
const QString kTilePromptedKey = QStringLiteral("tile/prompted");

// The one controller the capture service reports to. QML creates it once.
SenderController *s_instance = nullptr;

// Cast-card thumbnail. requestImage runs on the render thread; the engine
// owns the provider once it's installed.
class PreviewImageProvider : public QQuickImageProvider
{
public:
    PreviewImageProvider()
        : QQuickImageProvider(QQuickImageProvider::Image)
    {
    }

    QImage requestImage(const QString &, QSize *size, const QSize &) override
    {
        QMutexLocker lock(&m_mutex);
        if (size)
            *size = m_image.size();
        return m_image;
    }

    void setImage(const QImage &image)
    {
        QMutexLocker lock(&m_mutex);
        m_image = image;
    }

    void clear()
    {
        QMutexLocker lock(&m_mutex);
        m_image = {};
    }

private:
    QMutex m_mutex;
    QImage m_image;
};

PreviewImageProvider *g_preview = nullptr;

struct Endpoint
{
    QString host;
    quint16 port = 0;
    QString error;
};

Endpoint parseEndpoint(const QString &input)
{
    static const QRegularExpression hostPattern(QStringLiteral("^[A-Za-z0-9]([A-Za-z0-9.-]*[A-Za-z0-9])?$"));

    const QString text = input.trimmed();
    if (text.isEmpty())
        return {{}, 0, QObject::tr("Enter the address shown on your computer.")};

    Endpoint endpoint;
    endpoint.host = text;
    endpoint.port = beamr::kDefaultControlPort;

    // One colon is host:port; more than that is a bare IPv6 address.
    if (text.count(u':') == 1) {
        const qsizetype colon = text.indexOf(u':');
        endpoint.host = text.left(colon);
        bool ok = false;
        const uint port = text.mid(colon + 1).toUInt(&ok);
        if (!ok || port == 0 || port > 65535)
            return {{}, 0, QObject::tr("The port after “:” must be a number from 1 to 65535.")};
        endpoint.port = quint16(port);
    }

    if (QHostAddress(endpoint.host).isNull() && !hostPattern.match(endpoint.host).hasMatch())
        return {{}, 0, QObject::tr("That doesn't look like an address. It should look like 192.168.1.20.")};
    return endpoint;
}

QString displayEndpoint(const Endpoint &endpoint)
{
    if (endpoint.port == beamr::kDefaultControlPort)
        return endpoint.host;
    return endpoint.host + u':' + QString::number(endpoint.port);
}

#ifdef Q_OS_ANDROID
QString buildField(const char *name)
{
    return QJniObject::getStaticObjectField<jstring>("android/os/Build", name).toString().trimmed();
}

// The name the user gave the phone in Settings > About phone.
QString androidDeviceName()
{
    QJniObject context = QNativeInterface::QAndroidApplication::context();
    QJniObject resolver = context.callObjectMethod("getContentResolver", "()Landroid/content/ContentResolver;");
    if (!resolver.isValid())
        return {};
    return QJniObject::callStaticObjectMethod(
               "android/provider/Settings$Global", "getString",
               "(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;",
               resolver.object(), QJniObject::fromString(QStringLiteral("device_name")).object<jstring>())
        .toString()
        .trimmed();
}
#endif

QString defaultDeviceModel()
{
#ifdef Q_OS_ANDROID
    const QString manufacturer = buildField("MANUFACTURER");
    const QString model = buildField("MODEL");
    if (model.startsWith(manufacturer, Qt::CaseInsensitive))
        return model;
    return (manufacturer.left(1).toUpper() + manufacturer.mid(1) + u' ' + model).trimmed();
#else
    return QSysInfo::prettyProductName();
#endif
}

QString defaultDeviceName()
{
#ifdef Q_OS_ANDROID
    const QString name = androidDeviceName();
    if (!name.isEmpty())
        return name;
    return buildField("MODEL");
#else
    return QSysInfo::machineHostName();
#endif
}

// Of a receiver's addresses, the one on the same network as this phone.
QString reachableHost(const QStringList &hosts)
{
    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QString &host : hosts) {
        const QHostAddress address(host);
        for (const QNetworkInterface &iface : interfaces) {
            if (!iface.flags().testFlag(QNetworkInterface::IsUp))
                continue;
            for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.ip().isLoopback()
                    && address.isInSubnet(entry.ip(), entry.prefixLength())) {
                    return host;
                }
            }
        }
    }
    return hosts.value(0);
}

} // namespace

#ifdef Q_OS_ANDROID
void registerCaptureNatives();
void registerScanNatives();
#endif

SenderController::SenderController(QObject *parent)
    : QObject(parent)
{
    QSettings settings;
    m_device.id = settings.value(kDeviceIdKey).toString();
    if (m_device.id.isEmpty()) {
        m_device.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        settings.setValue(kDeviceIdKey, m_device.id);
    }
    m_device.model = defaultDeviceModel();
    m_device.name = settings.value(kDeviceNameKey).toString();
    if (m_device.name.isEmpty())
        m_device.name = defaultDeviceName();

    const int count = settings.beginReadArray(kRecentKey);
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        m_recent.append(QVariantMap{{"address", settings.value("address").toString()},
                                    {"name", settings.value("name").toString()}});
    }
    settings.endArray();

    m_shareAudio = settings.value(kShareAudioKey, true).toBool();
    m_quality = Quality(std::clamp(settings.value(kQualityKey, int(Smooth)).toInt(), int(Smooth), int(DataSaver)));
    m_onboardingDone = settings.value(kOnboardingDoneKey, false).toBool();
    m_tilePrompted = settings.value(kTilePromptedKey, false).toBool();

    QGuiApplication::styleHints()->setColorScheme(
        Qt::ColorScheme(settings.value(kColorSchemeKey, int(Qt::ColorScheme::Unknown)).toInt()));
#ifdef Q_OS_ANDROID
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, &SenderController::updateSystemBars);
    // Once the window is up.
    QMetaObject::invokeMethod(this, &SenderController::updateSystemBars, Qt::QueuedConnection);
#endif

    connect(&m_sessions, &SessionModel::countChanged, this, &SenderController::sessionsChanged);
    connect(&m_sessions, &SessionModel::dataChanged, this, &SenderController::sessionsChanged);
    connect(&m_sessions, &SessionModel::countChanged, this, &SenderController::updateCastNotification);
    connect(&m_sessions, &SessionModel::dataChanged, this, &SenderController::updateCastNotification);

    m_discovery = new DiscoveryClient(this);
    connect(m_discovery, &DiscoveryClient::receiversChanged, this, &SenderController::nearbyReceiversChanged);
    // Nearby leaves out receivers we're connected to.
    connect(&m_sessions, &SessionModel::countChanged, this, &SenderController::nearbyReceiversChanged);

    m_stream = new StreamSender;
    m_stream->moveToThread(&m_streamThread);
    connect(&m_streamThread, &QThread::finished, m_stream, &QObject::deleteLater);
    connect(m_stream, &StreamSender::opened, this, &SenderController::videoOpened);
    connect(m_stream, &StreamSender::closed, this, &SenderController::videoClosed);
    connect(m_stream, &StreamSender::congestionChanged, this, [this](bool congested) {
        // A late report from the cast we just stopped is about that cast.
        if (m_castState != On)
            congested = false;
        if (congested == m_networkStruggling)
            return;
        m_networkStruggling = congested;
        emit networkStrugglingChanged();
    });
    m_streamThread.setObjectName(QStringLiteral("beamr-stream"));
    m_streamThread.start();

    s_instance = this;
#ifdef Q_OS_ANDROID
    // Extra libraries are loaded before main. If OpenSSL is missing, say so
    // here; otherwise the failure only shows up as a refused cast.
    if (!QSslSocket::supportsSsl()) {
        qCWarning(lcNet) << "TLS initialization failed";
        QTimer::singleShot(0, this, [this] {
            setMessage(tr("This phone can't start a secure connection. Reinstall beamr."), true);
        });
    }
    registerCaptureNatives();
    registerScanNatives();
    // Text size can change in Android's settings while beamr is in the
    // background; pick it up when it comes back.
    refreshFontScale();
    connect(qApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
        if (state != Qt::ApplicationActive)
            return;
        refreshFontScale();
        resumeAudioIfGranted();
    });
    // The Quick Settings tile may have started the app.
    if (QJniObject::callStaticMethod<jboolean>("com/beamr/sender/CaptureBridge", "takePendingQuickCast"))
        QMetaObject::invokeMethod(this, &SenderController::quickCast, Qt::QueuedConnection);
#endif
}

void SenderController::refreshFontScale()
{
#ifdef Q_OS_ANDROID
    QJniObject context = QNativeInterface::QAndroidApplication::context();
    QJniObject configuration = context.callObjectMethod("getResources", "()Landroid/content/res/Resources;")
                                   .callObjectMethod("getConfiguration", "()Landroid/content/res/Configuration;");
    // Past about 1.6 the layouts can't hold; that's still a big step up.
    const qreal scale = std::clamp<qreal>(configuration.getField<jfloat>("fontScale"), 0.85, 1.6);
    if (!qFuzzyCompare(scale, m_fontScale)) {
        m_fontScale = scale;
        emit fontScaleChanged();
    }
#endif
}

void SenderController::quickCast()
{
    if (m_castState != Off)
        return;
    // Already connected: just start.
    if (approvedCount() > 0) {
        startCasting();
        return;
    }
    const QString address = lastAddress();
    if (address.isEmpty()) {
        setMessage(tr("Connect to a computer once, and the beamr tile will cast to it next time."), false);
        return;
    }
    // Casting starts on its own once the computer lets this phone in.
    if (!isConnectedTo(address))
        addReceiver(address);
}

SenderController::~SenderController()
{
    s_instance = nullptr;
    endCapture();
    for (ReceiverSession *session : m_sessions.sessions())
        session->close();
    m_streamThread.quit();
    m_streamThread.wait();
}

int SenderController::approvedCount() const
{
    const auto &sessions = m_sessions.sessions();
    return int(std::count_if(sessions.cbegin(), sessions.cend(),
                             [](const ReceiverSession *s) { return s->isApproved(); }));
}

int SenderController::streamingCount() const
{
    const auto &sessions = m_sessions.sessions();
    return int(std::count_if(sessions.cbegin(), sessions.cend(),
                             [](const ReceiverSession *s) { return s->state() == ReceiverSession::Streaming; }));
}

int SenderController::reconnectingCount() const
{
    const auto &sessions = m_sessions.sessions();
    return int(std::count_if(sessions.cbegin(), sessions.cend(),
                             [](const ReceiverSession *s) { return s->state() == ReceiverSession::Reconnecting; }));
}

bool SenderController::canAddReceiver() const
{
    return m_sessions.rowCount() < kMaxReceivers;
}

int SenderController::maxReceivers() const
{
    return kMaxReceivers;
}

void SenderController::setDeviceName(const QString &name)
{
    const QString trimmed = name.trimmed().left(64);
    QSettings settings;
    if (trimmed.isEmpty())
        settings.remove(kDeviceNameKey);
    else
        settings.setValue(kDeviceNameKey, trimmed);

    const QString effective = trimmed.isEmpty() ? defaultDeviceName() : trimmed;
    if (effective == m_device.name)
        return;
    m_device.name = effective;
    emit deviceNameChanged();
}

QString SenderController::lastAddress() const
{
    return m_recent.isEmpty() ? QString() : m_recent.first().toMap().value("address").toString();
}

int SenderController::defaultPort() const
{
    return beamr::kDefaultControlPort;
}

QString SenderController::validateAddress(const QString &address) const
{
    const Endpoint endpoint = parseEndpoint(address);
    if (!endpoint.error.isEmpty())
        return endpoint.error;
    if (isConnectedTo(address))
        return tr("This phone is already connected to %1.").arg(displayEndpoint(endpoint));
    if (!canAddReceiver())
        return tr("You can cast to up to %1 receivers at once.").arg(kMaxReceivers);
    return {};
}

bool SenderController::isConnectedTo(const QString &address) const
{
    const Endpoint endpoint = parseEndpoint(address);
    if (!endpoint.error.isEmpty())
        return false;
    const QString display = displayEndpoint(endpoint);
    const auto &sessions = m_sessions.sessions();
    return std::any_of(sessions.cbegin(), sessions.cend(),
                       [&](const ReceiverSession *s) { return s->endpoint() == display; });
}

QVariantList SenderController::nearbyReceivers() const
{
    QVariantList nearby;
    const QVariantList found = m_discovery->receivers();
    for (const QVariant &entry : found) {
        if (!isConnectedTo(entry.toMap().value("address").toString()))
            nearby.append(entry);
    }
    return nearby;
}

void SenderController::setDiscovering(bool discovering)
{
    if (discovering == m_discovering)
        return;
    m_discovering = discovering;
    m_discovery->setActive(discovering);
    emit discoveringChanged();
}

void SenderController::addReceiver(const QString &address, const QString &screen, const QString &pair,
                                   const QString &key)
{
    if (const QString error = validateAddress(address); !error.isEmpty()) {
        setMessage(error, true);
        return;
    }
    QByteArray fingerprint;
    if (!key.isEmpty()) {
        fingerprint = beamr::TlsIdentity::fingerprintFromText(key);
        if (fingerprint.isEmpty()) {
            setMessage(tr("That QR code is missing a valid security key. Scan the code in the beamr window again."),
                       true);
            return;
        }
    }
    clearMessage();

    const Endpoint endpoint = parseEndpoint(address);
    auto *session = new ReceiverSession(endpoint.host, endpoint.port, displayEndpoint(endpoint), m_device, screen,
                                        pair, fingerprint, this);
    connect(session, &ReceiverSession::welcomed, this, [this, session] { rememberReceiver(session); });
    connect(session, &ReceiverSession::approved, this, [this, session] { onSessionApproved(session); });
    connect(session, &ReceiverSession::keyFrameRequested, this, [this] {
        QMetaObject::invokeMethod(m_stream, &StreamSender::requestKeyFrame);
    });
    connect(session, &ReceiverSession::ended, this, [this, session](const QString &message, bool isError) {
        onSessionEnded(session, message, isError);
    });
    m_sessions.add(session);
    session->start();
}

void SenderController::removeReceiver(const QString &sessionId)
{
    ReceiverSession *session = m_sessions.find(sessionId);
    if (!session)
        return;
    session->close();
    onSessionEnded(session, {}, false);
}

void SenderController::disconnectAll()
{
    const QList<ReceiverSession *> sessions = m_sessions.sessions();
    for (ReceiverSession *session : sessions)
        removeReceiver(session->id());
}

void SenderController::onSessionApproved(ReceiverSession *session)
{
    haptic(true);
    switch (m_castState) {
    case Off:
        // Casting is why they connected; go straight to the consent dialog.
        startCasting();
        break;
    case Starting:
        // Joins when capture starts.
        break;
    case On:
        // New, or back after a drop: either way any "video dropped" is old news.
        clearMessage();
        openVideo(session);
        break;
    }
}

void SenderController::onSessionEnded(ReceiverSession *session, const QString &message, bool isError)
{
    QMetaObject::invokeMethod(m_stream, [stream = m_stream, id = session->id()] { stream->close(id); });
    m_sessions.remove(session);
    session->deleteLater();

    if (!message.isEmpty()) {
        setMessage(message, isError, isError ? session->endpoint() : QString());
        if (isError)
            haptic(false);
    }
    // Nobody left to watch: stop recording the screen.
    if (approvedCount() == 0 && m_castState != Off)
        endCapture();
}

void SenderController::openVideo(ReceiverSession *session)
{
    QMetaObject::invokeMethod(m_stream, [stream = m_stream, id = session->id(), host = session->host(),
                                         port = session->port(), token = session->streamToken(),
                                         audio = session->playsAudio(),
                                         fingerprint = session->peerFingerprint()] {
        stream->open(id, host, port, token, audio, fingerprint);
    });
}

void SenderController::startCasting()
{
    if (m_castState != Off || approvedCount() == 0)
        return;
#ifdef Q_OS_ANDROID
    clearMessage();
    m_castQuality = m_quality;
    m_castShareAudio = m_shareAudio;
    setCastState(Starting);
    emit qualityChanged(); // frameRate now follows this cast
    emit castSettingsChanged();
    m_notificationTitle = castNotificationTitle();
    m_notificationText = castNotificationText();
    const QJniObject title = QJniObject::fromString(m_notificationTitle);
    const QJniObject text = QJniObject::fromString(m_notificationText);
    QJniObject::callStaticMethod<void>("com/beamr/sender/CaptureBridge", "requestCapture",
                                       "(Landroid/content/Context;ZILjava/lang/String;Ljava/lang/String;)V",
                                       QNativeInterface::QAndroidApplication::context().object(),
                                       jboolean(m_shareAudio), jint(m_quality), title.object<jstring>(),
                                       text.object<jstring>());
#else
    setMessage(tr("Screen casting needs the Android app."), true);
#endif
}

void SenderController::stopCasting()
{
    setChangingShare(false);
    endCapture();
}

void SenderController::applyCastSettings()
{
    if (m_castState != On || m_changingShare || m_applyingSettings || !castSettingsPending())
        return;
#ifdef Q_OS_ANDROID
    // Sound was just turned on for this cast, and Android has not said yes yet.
    // A quality change must not ask again after a denial; Open settings does that.
    const bool turningSoundOn = m_shareAudio && !m_castShareAudio;
    const bool needPermission = turningSoundOn && !audioPermissionGranted()
                                && m_audioState != QLatin1String("unavailable");
    if (needPermission) {
        m_applyingSettings = true;
        qApp->requestPermission(QMicrophonePermission{}, this, [this](const QPermission &) {
            m_applyingSettings = false;
            if (m_castState != On || m_changingShare)
                return;
            pushCastSettings();
        });
        return;
    }
    pushCastSettings();
#else
    m_castQuality = m_quality;
    m_castShareAudio = m_shareAudio;
    emit qualityChanged();
    emit castSettingsChanged();
#endif
}

void SenderController::useDataSaver()
{
    setQuality(DataSaver);
    if (m_castState == On)
        applyCastSettings();
}

void SenderController::changeShareTarget()
{
    if (m_castState != On || m_changingShare || approvedCount() == 0)
        return;
    setChangingShare(true);
#ifdef Q_OS_ANDROID
    QJniObject::callStaticMethod<void>("com/beamr/sender/CaptureBridge", "stopCapture");
#else
    setChangingShare(false);
#endif
}

void SenderController::openAppSettings()
{
#ifdef Q_OS_ANDROID
    QJniObject::callStaticMethod<void>("com/beamr/sender/CaptureBridge", "openAppSettings",
                                       "(Landroid/content/Context;)V",
                                       QNativeInterface::QAndroidApplication::context().object());
#endif
}

void SenderController::endCapture()
{
    if (m_captureRunning || m_castState == Starting) {
#ifdef Q_OS_ANDROID
        QJniObject::callStaticMethod<void>("com/beamr/sender/CaptureBridge", "stopCapture");
#endif
    }
    m_captureRunning = false;
    QMetaObject::invokeMethod(m_stream, &StreamSender::closeAll);
    setCastState(Off);
    setAudioState({});
    clearSharePreview();
    m_notificationTitle.clear();
    m_notificationText.clear();
    if (m_networkStruggling) {
        m_networkStruggling = false;
        emit networkStrugglingChanged();
    }
    if (m_castSize.isValid()) {
        m_castSize = {};
        emit castSizeChanged();
    }
}

void SenderController::captureStarted(QSize size)
{
    // Everyone left while the consent dialog was up.
    if (m_castState == Off) {
#ifdef Q_OS_ANDROID
        QJniObject::callStaticMethod<void>("com/beamr/sender/CaptureBridge", "stopCapture");
#endif
        return;
    }

    // Called again, with the new size, when the phone rotates; the streams
    // carry on and receivers pick the size up from the video.
    const bool firstStart = !m_captureRunning;
    m_captureRunning = true;
    setCastState(On);
    if (size != m_castSize) {
        m_castSize = size;
        emit castSizeChanged();
    }
    if (firstStart) {
        for (ReceiverSession *session : m_sessions.sessions()) {
            if (session->state() == ReceiverSession::Ready)
                openVideo(session);
        }
        // The consent dialog just closed. Let Live show, then offer the tile once.
        QTimer::singleShot(1200, this, [this] {
            if (m_castState == On && !m_applyingSettings)
                maybeOfferTile();
        });
    }
    // Settings changed while the consent dialog was up.
    if (castSettingsPending())
        applyCastSettings();
    updateCastNotification();
}

void SenderController::setAudioState(const QString &state)
{
    if (state == m_audioState)
        return;
    m_audioState = state;
    emit audioStateChanged();
    updateCastNotification();
}

void SenderController::installPreviewProvider(QQmlEngine *engine)
{
    if (!engine || g_preview)
        return;
    g_preview = new PreviewImageProvider;
    engine->addImageProvider(QStringLiteral("beamrpreview"), g_preview);
}

void SenderController::setShareTarget(const QString &target)
{
    if (m_castState == Off)
        return;
    if (target != QLatin1String("screen") && target != QLatin1String("app"))
        return;
    if (target == m_shareTarget)
        return;
    m_shareTarget = target;
    emit shareTargetChanged();
    updateCastNotification();
}

void SenderController::setPreview(const QByteArray &jpeg)
{
    if (m_castState == Off || !g_preview)
        return;
    const QImage image = QImage::fromData(jpeg, "JPEG");
    if (image.isNull())
        return;
    g_preview->setImage(image);
    ++m_previewRevision;
    emit previewRevisionChanged();
}

void SenderController::clearSharePreview()
{
    const bool hadTarget = !m_shareTarget.isEmpty();
    const bool hadFrame = m_previewRevision != 0;
    m_shareTarget.clear();
    m_previewRevision = 0;
    if (g_preview)
        g_preview->clear();
    if (hadTarget)
        emit shareTargetChanged();
    if (hadFrame)
        emit previewRevisionChanged();
}

void SenderController::setOnboardingDone(bool done)
{
    if (done == m_onboardingDone)
        return;
    m_onboardingDone = done;
    QSettings().setValue(kOnboardingDoneKey, done);
    emit onboardingDoneChanged();
}

void SenderController::setQuality(Quality quality)
{
    if (quality == m_quality)
        return;
    m_quality = quality;
    QSettings().setValue(kQualityKey, int(quality));
    emit qualityChanged();
    emit castSettingsChanged();
}

void SenderController::setShareAudio(bool share)
{
    if (share == m_shareAudio)
        return;
    m_shareAudio = share;
    QSettings().setValue(kShareAudioKey, share);
    emit shareAudioChanged();
    emit castSettingsChanged();
}

void SenderController::captureStopped(const QString &reason)
{
    const bool recapture = m_changingShare;
    setChangingShare(false);
    const bool wasCasting = m_castState != Off;
    m_captureRunning = false;
    if (!wasCasting)
        return;
    endCapture();

    // The old capture is gone. Ask what to share, and keep the receivers.
    if (recapture && reason.isEmpty() && approvedCount() > 0) {
        startCasting();
        return;
    }

    if (reason == QLatin1StringView("denied")) {
        setMessage(tr("Screen sharing wasn't allowed. Tap Start casting to try again."), false);
    } else if (!reason.isEmpty()) {
        setMessage(tr("Casting stopped: %1").arg(reason), true);
        haptic(false);
    }
}

void SenderController::videoOpened(const QString &sessionId)
{
    if (ReceiverSession *session = m_sessions.find(sessionId))
        session->setStreaming(true);
}

void SenderController::videoClosed(const QString &sessionId, const QString &error)
{
    ReceiverSession *session = m_sessions.find(sessionId);
    if (!session)
        return;
    session->setStreaming(false);
    if (error.isEmpty() || m_castState != On)
        return;
    // Often the whole connection is going down and the session will say so,
    // or come back on its own; the video just noticed first. Give it a moment.
    QTimer::singleShot(1500, this, [this, sessionId, name = session->name(), error] {
        ReceiverSession *session = m_sessions.find(sessionId);
        if (session && m_castState == On && session->state() == ReceiverSession::Ready)
            setMessage(tr("Video to “%1” dropped: %2").arg(name, error), true);
    });
}

void SenderController::forgetReceiver(const QString &address)
{
    const auto removed = m_recent.removeIf([&](const QVariant &entry) {
        return entry.toMap().value("address").toString() == address;
    });
    if (removed == 0)
        return;
    saveRecent();
    emit recentReceiversChanged();
}

bool SenderController::canScan() const
{
#ifdef Q_OS_ANDROID
    return true;
#else
    return false;
#endif
}

void SenderController::scanQrCode()
{
#ifdef Q_OS_ANDROID
    QJniObject::callStaticMethod<void>("com/beamr/sender/QrScanner", "scan", "(Landroid/content/Context;)V",
                                       QNativeInterface::QAndroidApplication::context().object());
#endif
}

void SenderController::qrScanned(const QString &text)
{
    const beamr::ConnectLink link = beamr::ConnectLink::parse(text);
    QString address;
    if (link.isValid()) {
        if (beamr::TlsIdentity::fingerprintFromText(link.key).isEmpty()) {
            setMessage(tr("That QR code is missing a valid security key. Scan the code in the beamr window again."),
                       true);
            haptic(false);
            return;
        }
        address = displayEndpoint({reachableHost(link.hosts), link.port, {}});
    } else if (!text.contains(u'/') && parseEndpoint(text).error.isEmpty()) {
        // A plain address works too.
        address = text.trimmed();
    } else {
        setMessage(tr("That QR code isn't from beamr. Scan the code in the beamr window on your computer."), true);
        haptic(false);
        return;
    }
    if (const QString error = validateAddress(address); !error.isEmpty()) {
        setMessage(error, true);
        haptic(false);
        return;
    }
    addReceiver(address, link.screen, link.pair, link.key);
}

void SenderController::qrScanFailed(const QString &reason)
{
    if (reason.isEmpty())
        return; // Canceled.
    setMessage(tr("The QR scanner isn't available on this phone. Enter the address instead."), true);
}

int SenderController::colorScheme() const
{
    return QSettings().value(kColorSchemeKey, int(Qt::ColorScheme::Unknown)).toInt();
}

void SenderController::setColorScheme(int scheme)
{
    if (scheme == colorScheme())
        return;
    QSettings().setValue(kColorSchemeKey, scheme);
    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme(scheme));
    emit colorSchemeChanged();
}

void SenderController::updateSystemBars()
{
#ifdef Q_OS_ANDROID
    const bool light = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Light;
    QJniObject::callStaticMethod<void>("com/beamr/sender/SystemBars", "setDarkContent", "(Landroid/content/Context;Z)V",
                                       QNativeInterface::QAndroidApplication::context().object(), jboolean(light));
#endif
}

void SenderController::clearMessage()
{
    setMessage({}, false);
}

void SenderController::moveToBackground()
{
#ifdef Q_OS_ANDROID
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    activity.callMethod<jboolean>("moveTaskToBack", "(Z)Z", jboolean(true));
#endif
}

void SenderController::haptic(bool success)
{
#ifdef Q_OS_ANDROID
    QJniObject::callStaticMethod<void>("com/beamr/sender/CaptureBridge", "haptic",
                                       "(Landroid/content/Context;Z)V",
                                       QNativeInterface::QAndroidApplication::context().object(),
                                       jboolean(success));
#else
    Q_UNUSED(success)
#endif
}

void SenderController::setMessage(const QString &message, bool isError, const QString &retryAddress)
{
    if (message == m_message && isError == m_messageIsError && retryAddress == m_retryAddress)
        return;
    m_message = message;
    m_messageIsError = isError;
    m_retryAddress = retryAddress;
    emit messageChanged();
}

void SenderController::setCastState(CastState state)
{
    if (state == m_castState)
        return;
    m_castState = state;
    emit castStateChanged();
    emit castSettingsChanged();
}

bool SenderController::castSettingsPending() const
{
    return m_castState == On && !m_changingShare && (m_quality != m_castQuality || m_shareAudio != m_castShareAudio);
}

void SenderController::setChangingShare(bool changing)
{
    if (changing == m_changingShare)
        return;
    m_changingShare = changing;
    emit castStateChanged();
    emit castSettingsChanged();
}

void SenderController::pushCastSettings()
{
#ifdef Q_OS_ANDROID
    const bool applied = QJniObject::callStaticMethod<jboolean>(
        "com/beamr/sender/CaptureBridge", "applySettings", "(ZI)Z", jboolean(m_shareAudio), jint(m_quality));
    if (!applied)
        return;
#endif
    const bool rateChanged = m_castQuality != m_quality;
    m_castQuality = m_quality;
    m_castShareAudio = m_shareAudio;
    if (rateChanged)
        emit qualityChanged();
    emit castSettingsChanged();
    updateCastNotification();
}

void SenderController::updateCastNotification()
{
#ifdef Q_OS_ANDROID
    if (m_castState != On)
        return;
    const QString title = castNotificationTitle();
    const QString text = castNotificationText();
    if (title == m_notificationTitle && text == m_notificationText)
        return;
    m_notificationTitle = title;
    m_notificationText = text;
    const QJniObject jTitle = QJniObject::fromString(title);
    const QJniObject jText = QJniObject::fromString(text);
    QJniObject::callStaticMethod<void>("com/beamr/sender/CaptureBridge", "updateNotification",
                                       "(Ljava/lang/String;Ljava/lang/String;)V", jTitle.object<jstring>(),
                                       jText.object<jstring>());
#endif
}

QString SenderController::castNotificationTitle() const
{
    QStringList names;
    int reconnecting = 0;
    int streaming = 0;
    for (const ReceiverSession *session : m_sessions.sessions()) {
        if (!session->isApproved())
            continue;
        names.append(session->name());
        if (session->state() == ReceiverSession::Reconnecting)
            ++reconnecting;
        else if (session->state() == ReceiverSession::Streaming)
            ++streaming;
    }
    const bool quiet = streaming == 0 && reconnecting > 0;
    if (names.isEmpty())
        return quiet ? tr("Reconnecting…") : tr("Casting");
    if (names.size() == 1)
        return quiet ? tr("Reconnecting to %1").arg(names.at(0)) : tr("Casting to %1").arg(names.at(0));
    if (quiet)
        return tr("Reconnecting…");
    if (names.size() == 2)
        return tr("Casting to %1 and %2").arg(names.at(0), names.at(1));
    return tr("Casting to %1 computers").arg(names.size());
}

QString SenderController::castNotificationText() const
{
    const QString picture = m_shareTarget == QLatin1String("app") ? tr("One app")
                            : m_shareTarget == QLatin1String("screen") ? tr("Entire screen")
                                                                       : tr("Your screen");
    const QString sound = m_audioState == QLatin1String("on")                    ? tr("with sound")
                          : m_audioState == QLatin1String("denied")              ? tr("sound needs permission")
                          : m_audioState == QLatin1String("unavailable") || !m_castShareAudio ? tr("picture only")
                                                                                              : tr("with sound");
    return tr("%1 · %2").arg(picture, sound);
}

void SenderController::maybeOfferTile()
{
    if (m_tilePrompted || m_tilePromptInFlight || m_castState != On)
        return;
#ifdef Q_OS_ANDROID
    // 1: dialog requested. 0: this Android has none. -1: try again next cast.
    const int result = QJniObject::callStaticMethod<jint>(
        "com/beamr/sender/CaptureBridge", "requestTile", "(Landroid/content/Context;)I",
        QNativeInterface::QAndroidApplication::context().object());
    if (result == 0)
        tilePromptFinished(true);
    else if (result > 0)
        m_tilePromptInFlight = true;
#endif
}

void SenderController::tilePromptFinished(bool answered)
{
    m_tilePromptInFlight = false;
    if (!answered || m_tilePrompted)
        return;
    m_tilePrompted = true;
    QSettings().setValue(kTilePromptedKey, true);
}

void SenderController::resumeAudioIfGranted()
{
#ifdef Q_OS_ANDROID
    if (m_castState != On || !m_shareAudio || m_audioState != QLatin1String("denied") || !audioPermissionGranted())
        return;
    QJniObject::callStaticMethod<void>("com/beamr/sender/CaptureBridge", "startCastAudio");
#endif
}

bool SenderController::audioPermissionGranted() const
{
#ifdef Q_OS_ANDROID
    return qApp->checkPermission(QMicrophonePermission{}) == Qt::PermissionStatus::Granted;
#else
    return false;
#endif
}

void SenderController::rememberReceiver(const ReceiverSession *session)
{
    const QString address = session->endpoint();
    m_recent.removeIf([&](const QVariant &entry) { return entry.toMap().value("address").toString() == address; });
    m_recent.prepend(QVariantMap{{"address", address}, {"name", session->name()}});
    while (m_recent.size() > kMaxRecent)
        m_recent.removeLast();
    saveRecent();
    emit recentReceiversChanged();
}

void SenderController::saveRecent() const
{
    QSettings settings;
    settings.beginWriteArray(kRecentKey, int(m_recent.size()));
    for (int i = 0; i < m_recent.size(); ++i) {
        const QVariantMap entry = m_recent.at(i).toMap();
        settings.setArrayIndex(i);
        settings.setValue("address", entry.value("address"));
        settings.setValue("name", entry.value("name"));
    }
    settings.endArray();
}

#ifdef Q_OS_ANDROID
// JNI entry points for com.beamr.sender.CaptureBridge. They run on Android
// threads; frames go straight to the stream thread, the rest to the UI thread.
struct CaptureNatives
{
    static void captureStarted(JNIEnv *, jclass, jint width, jint height)
    {
        const QSize size(width, height);
        QMetaObject::invokeMethod(qApp, [size] {
            if (s_instance)
                s_instance->captureStarted(size);
        });
    }

    static void captureStopped(JNIEnv *, jclass, jstring reason)
    {
        const QString text = QJniObject(reason).toString();
        QMetaObject::invokeMethod(qApp, [text] {
            if (s_instance)
                s_instance->captureStopped(text);
        });
    }

    static void audio(JNIEnv *env, jclass, jobject buffer, jint offset, jint size, jlong ptsUs)
    {
        SenderController *controller = s_instance;
        const auto *base = static_cast<const char *>(env->GetDirectBufferAddress(buffer));
        if (!controller || !base || size <= 0)
            return;
        const QByteArray data(base + offset, size);
        StreamSender *stream = controller->m_stream;
        QMetaObject::invokeMethod(stream, [stream, data, ptsUs = qint64(ptsUs)] { stream->sendAudio(data, ptsUs); });
    }

    static void quickCast(JNIEnv *, jclass)
    {
        QMetaObject::invokeMethod(qApp, [] {
            if (s_instance)
                s_instance->quickCast();
        });
    }

    static void shareTarget(JNIEnv *, jclass, jstring target)
    {
        const QString text = QJniObject(target).toString();
        QMetaObject::invokeMethod(qApp, [text] {
            if (s_instance)
                s_instance->setShareTarget(text);
        });
    }

    static void preview(JNIEnv *env, jclass, jbyteArray jpeg)
    {
        if (!jpeg)
            return;
        const jsize n = env->GetArrayLength(jpeg);
        // A 320px JPEG is a few dozen kilobytes; anything larger isn't one.
        if (n <= 0 || n > 1024 * 1024)
            return;
        QByteArray bytes(n, Qt::Uninitialized);
        env->GetByteArrayRegion(jpeg, 0, n, reinterpret_cast<jbyte *>(bytes.data()));
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            return;
        }
        QMetaObject::invokeMethod(qApp, [bytes] {
            if (s_instance)
                s_instance->setPreview(bytes);
        });
    }

    static void tilePrompted(JNIEnv *, jclass, jboolean answered)
    {
        const bool accepted = answered;
        QMetaObject::invokeMethod(qApp, [accepted] {
            if (s_instance)
                s_instance->tilePromptFinished(accepted);
        });
    }

    static void audioState(JNIEnv *, jclass, jstring state)
    {
        const QString text = QJniObject(state).toString();
        QMetaObject::invokeMethod(qApp, [text] {
            if (s_instance && s_instance->m_castState != SenderController::Off)
                s_instance->setAudioState(text);
        });
    }

    static void frame(JNIEnv *env, jclass, jobject buffer, jint offset, jint size, jlong ptsUs, jint flags)
    {
        SenderController *controller = s_instance;
        const auto *base = static_cast<const char *>(env->GetDirectBufferAddress(buffer));
        if (!controller || !base || size <= 0)
            return;
        // Copied now: Android reuses the buffer as soon as we return.
        const QByteArray data(base + offset, size);
        StreamSender *stream = controller->m_stream;
        QMetaObject::invokeMethod(stream, [stream, data, flags = quint8(flags), ptsUs = qint64(ptsUs)] {
            stream->sendFrame(data, flags, ptsUs);
        });
    }
};

// JNI entry points for com.beamr.sender.QrScanner, called on the Android UI thread.
struct ScanNatives
{
    static void scanned(JNIEnv *, jclass, jstring text)
    {
        const QString value = QJniObject(text).toString();
        QMetaObject::invokeMethod(qApp, [value] {
            if (s_instance)
                s_instance->qrScanned(value);
        });
    }

    static void scanFailed(JNIEnv *, jclass, jstring reason)
    {
        const QString value = QJniObject(reason).toString();
        QMetaObject::invokeMethod(qApp, [value] {
            if (s_instance)
                s_instance->qrScanFailed(value);
        });
    }
};

void registerScanNatives()
{
    QJniEnvironment env;
    const bool ok = env.registerNativeMethods(
        "com/beamr/sender/QrScanner",
        {
            {"nativeScanned", "(Ljava/lang/String;)V", reinterpret_cast<void *>(&ScanNatives::scanned)},
            {"nativeScanFailed", "(Ljava/lang/String;)V", reinterpret_cast<void *>(&ScanNatives::scanFailed)},
        });
    if (!ok)
        qCWarning(lcBeamr) << "can't register QR scanner callbacks; scanning won't work";
}

void registerCaptureNatives()
{
    QJniEnvironment env;
    const bool ok = env.registerNativeMethods(
        "com/beamr/sender/CaptureBridge",
        {
            {"nativeCaptureStarted", "(II)V", reinterpret_cast<void *>(&CaptureNatives::captureStarted)},
            {"nativeCaptureStopped", "(Ljava/lang/String;)V", reinterpret_cast<void *>(&CaptureNatives::captureStopped)},
            {"nativeFrame", "(Ljava/nio/ByteBuffer;IIJI)V", reinterpret_cast<void *>(&CaptureNatives::frame)},
            {"nativeAudio", "(Ljava/nio/ByteBuffer;IIJ)V", reinterpret_cast<void *>(&CaptureNatives::audio)},
            {"nativeAudioState", "(Ljava/lang/String;)V", reinterpret_cast<void *>(&CaptureNatives::audioState)},
            {"nativeTilePrompted", "(Z)V", reinterpret_cast<void *>(&CaptureNatives::tilePrompted)},
            {"nativeQuickCast", "()V", reinterpret_cast<void *>(&CaptureNatives::quickCast)},
            {"nativeShareTarget", "(Ljava/lang/String;)V", reinterpret_cast<void *>(&CaptureNatives::shareTarget)},
            {"nativePreview", "([B)V", reinterpret_cast<void *>(&CaptureNatives::preview)},
        });
    if (!ok)
        qCWarning(lcBeamr) << "can't register capture callbacks; casting won't work";
}
#endif
