#include "sendercontroller.h"

#include <QGuiApplication>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QRegularExpression>
#include <QSettings>
#include <QStyleHints>
#include <QSysInfo>
#include <QUuid>

#include <algorithm>

#ifdef Q_OS_ANDROID
#include <QCoreApplication>
#include <QJniEnvironment>
#include <QJniObject>
#endif

#include <beamr/config.h>
#include <beamr/connectlink.h>
#include <beamr/log.h>
#include <beamr/protocol.h>

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

// The one controller the capture service reports to. QML creates it once.
SenderController *s_instance = nullptr;

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

    QGuiApplication::styleHints()->setColorScheme(
        Qt::ColorScheme(settings.value(kColorSchemeKey, int(Qt::ColorScheme::Unknown)).toInt()));
#ifdef Q_OS_ANDROID
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, &SenderController::updateSystemBars);
    // Once the window is up.
    QMetaObject::invokeMethod(this, &SenderController::updateSystemBars, Qt::QueuedConnection);
#endif

    connect(&m_sessions, &SessionModel::countChanged, this, &SenderController::sessionsChanged);
    connect(&m_sessions, &SessionModel::dataChanged, this, &SenderController::sessionsChanged);

    m_stream = new StreamSender;
    m_stream->moveToThread(&m_streamThread);
    connect(&m_streamThread, &QThread::finished, m_stream, &QObject::deleteLater);
    connect(m_stream, &StreamSender::opened, this, &SenderController::videoOpened);
    connect(m_stream, &StreamSender::closed, this, &SenderController::videoClosed);
    m_streamThread.setObjectName(QStringLiteral("beamr-stream"));
    m_streamThread.start();

    s_instance = this;
#ifdef Q_OS_ANDROID
    registerCaptureNatives();
    registerScanNatives();
#endif
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

void SenderController::addReceiver(const QString &address, const QString &screen)
{
    if (const QString error = validateAddress(address); !error.isEmpty()) {
        setMessage(error, true);
        return;
    }
    clearMessage();

    const Endpoint endpoint = parseEndpoint(address);
    auto *session = new ReceiverSession(endpoint.host, endpoint.port, displayEndpoint(endpoint), m_device, screen, this);
    connect(session, &ReceiverSession::welcomed, this, [this, session] { rememberReceiver(session); });
    connect(session, &ReceiverSession::approved, this, [this, session] { onSessionApproved(session); });
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
                                         audio = session->playsAudio()] {
        stream->open(id, host, port, token, audio);
    });
}

void SenderController::startCasting()
{
    if (m_castState != Off || approvedCount() == 0)
        return;
#ifdef Q_OS_ANDROID
    clearMessage();
    setCastState(Starting);
    QJniObject::callStaticMethod<void>("com/beamr/sender/CaptureBridge", "requestCapture",
                                       "(Landroid/content/Context;Z)V",
                                       QNativeInterface::QAndroidApplication::context().object(),
                                       jboolean(m_shareAudio));
#else
    setMessage(tr("Screen casting needs the Android app."), true);
#endif
}

void SenderController::stopCasting()
{
    endCapture();
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
    }
}

void SenderController::setAudioState(const QString &state)
{
    if (state == m_audioState)
        return;
    m_audioState = state;
    emit audioStateChanged();
}

void SenderController::setShareAudio(bool share)
{
    if (share == m_shareAudio)
        return;
    m_shareAudio = share;
    QSettings().setValue(kShareAudioKey, share);
    emit shareAudioChanged();
}

void SenderController::captureStopped(const QString &reason)
{
    const bool wasCasting = m_castState != Off;
    m_captureRunning = false;
    if (!wasCasting)
        return;
    endCapture();

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
    // Asked for, or the control connection is going down too and will
    // explain itself.
    if (!error.isEmpty() && m_castState == On)
        setMessage(tr("Video to “%1” dropped: %2").arg(session->name(), error), true);
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
    addReceiver(address, link.screen);
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
        });
    if (!ok)
        qCWarning(lcBeamr) << "can't register capture callbacks; casting won't work";
}
#endif
