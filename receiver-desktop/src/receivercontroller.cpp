#include "receivercontroller.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkInterface>
#include <QSettings>
#include <QStandardPaths>
#include <QSysInfo>
#include <QUuid>
#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>
#include <array>

#include <beamr/config.h>
#include <beamr/connectlink.h>
#include <beamr/log.h>

#include "controlserver.h"

namespace {

constexpr int kRequestTimeoutSec = 60;
constexpr int kAddressRefreshTicks = 5;
// Each screen decodes its own 1080p60 stream; four keeps a laptop cool.
constexpr int kMaxScreens = 4;

const QString kNameKey = QStringLiteral("receiver/name");
const QString kRequireApprovalKey = QStringLiteral("receiver/requireApproval");
const QString kTrustedKey = QStringLiteral("trustedDevices");

// BEAMR_PORT overrides the port, e.g. to run a second receiver on one computer.
quint16 controlPort()
{
    bool ok = false;
    const int port = qEnvironmentVariableIntValue("BEAMR_PORT", &ok);
    return ok && port > 0 && port <= 65535 ? quint16(port) : beamr::kDefaultControlPort;
}

QString defaultReceiverName()
{
    const QString host = QSysInfo::machineHostName();
    return host.isEmpty() ? QStringLiteral("beamr receiver") : host;
}

QString userFolder(QStandardPaths::StandardLocation location)
{
    QString base = QStandardPaths::writableLocation(location);
    if (base.isEmpty())
        base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(base).filePath(QStringLiteral("beamr"));
}

// Docker, VM and VPN bridges show up as "up" interfaces but a phone can't
// reach them; listing them only confuses people typing an address.
bool isVirtualInterface(const QNetworkInterface &iface)
{
    static const std::array<QLatin1StringView, 6> prefixes = {
        QLatin1StringView("docker"), QLatin1StringView("veth"), QLatin1StringView("br-"),
        QLatin1StringView("virbr"), QLatin1StringView("vmnet"), QLatin1StringView("vboxnet"),
    };
    if (iface.type() == QNetworkInterface::Virtual)
        return true;
    for (const QLatin1StringView prefix : prefixes) {
        if (iface.name().startsWith(prefix))
            return true;
    }
    return false;
}

} // namespace

ReceiverController::ReceiverController(QObject *parent)
    : QObject(parent)
    , m_recordingsDir(userFolder(QStandardPaths::MoviesLocation))
    , m_screenshotsDir(userFolder(QStandardPaths::PicturesLocation))
{
    QSettings settings;
    m_receiverName = settings.value(kNameKey, defaultReceiverName()).toString();
    m_requireApproval = settings.value(kRequireApprovalKey, true).toBool();

    const int count = settings.beginReadArray(kTrustedKey);
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        m_trusted.append({settings.value("id").toString(), settings.value("name").toString()});
    }
    settings.endArray();

    m_recordings = new RecordingsModel(m_recordingsDir, this);

    connect(&m_exportWatcher, &QFutureWatcher<QString>::started, this, &ReceiverController::exportingChanged);
    connect(&m_exportWatcher, &QFutureWatcher<QString>::finished, this, [this] {
        emit exportingChanged();
        const QString error = m_exportWatcher.result();
        emit notify(error.isEmpty() ? tr("Recording exported") : tr("Export failed: %1").arg(error));
    });

    connect(&m_ticker, &QTimer::timeout, this, &ReceiverController::tick);
    m_ticker.start(1000);
    appendScreen();
    refreshAddresses();

    auto *server = new ControlServer(this);
    if (!server->listen(controlPort())) {
        qCWarning(lcNet) << "can't listen on control port" << controlPort() << server->errorString();
        // The UI isn't connected yet; say it once it is.
        QTimer::singleShot(0, this, [this, error = server->errorString()] {
            emit notify(tr("Phones can't connect: port %1 is unavailable (%2)")
                            .arg(controlPort()).arg(error));
        });
    }
}

ReceiverController::~ReceiverController() = default;

int ReceiverController::castingCount() const
{
    return int(std::count_if(m_screens.cbegin(), m_screens.cend(), [](const CastScreen *s) { return s->casting(); }));
}

int ReceiverController::maxScreens() const
{
    return kMaxScreens;
}

CastScreen *ReceiverController::appendScreen()
{
    auto *screen = new CastScreen(int(m_screens.size()) + 1, this);
    screen->setRecordingsFolder(m_recordingsDir);
    connect(screen, &CastScreen::notify, this, &ReceiverController::notify);
    connect(screen, &CastScreen::recordingSaved, m_recordings, &RecordingsModel::refresh);
    connect(screen, &CastScreen::keyFrameNeeded, this, &ReceiverController::keyFrameNeeded);
    connect(screen, &CastScreen::castStopped, this, &ReceiverController::castStopped);
    connect(screen, &CastScreen::castingChanged, this, [this, screen] {
        onCastingChanged(screen);
        emit stateChanged();
    });
    m_screens.append(screen);
    updateConnectLinks();
    return screen;
}

void ReceiverController::addScreen()
{
    if (!canAddScreen())
        return;
    appendScreen();
    emit screensChanged();
}

void ReceiverController::removeScreen(CastScreen *screen)
{
    if (m_screens.size() <= 1 || !m_screens.contains(screen))
        return;
    screen->stopCasting();
    m_screens.removeOne(screen);
    if (m_audioScreen == screen)
        setAudioScreen(nullptr);
    renumberScreens();
    emit screensChanged();
    screen->deleteLater();
}

void ReceiverController::setAudioScreen(CastScreen *screen)
{
    if (screen == m_audioScreen || (screen && !m_screens.contains(screen)))
        return;
    m_audioScreen = screen;
    for (CastScreen *s : std::as_const(m_screens))
        s->setAudible(s == m_audioScreen);
    emit audioScreenChanged();
}

void ReceiverController::onCastingChanged(CastScreen *screen)
{
    if (screen->casting()) {
        if (!m_audioScreen)
            setAudioScreen(screen);
        return;
    }
    if (screen != m_audioScreen)
        return;
    // Hand the sound to another phone that's still casting, if any.
    const auto other = std::find_if(m_screens.cbegin(), m_screens.cend(), [](const CastScreen *s) {
        return s->casting();
    });
    setAudioScreen(other != m_screens.cend() ? *other : nullptr);
}

void ReceiverController::renumberScreens()
{
    for (int i = 0; i < m_screens.size(); ++i)
        m_screens.at(i)->setNumber(i + 1);
}

void ReceiverController::updateConnectLinks()
{
    for (CastScreen *screen : std::as_const(m_screens)) {
        if (m_addresses.isEmpty()) {
            screen->setConnectLink({});
            continue;
        }
        beamr::ConnectLink link{m_receiverName, screen->screenId(), m_addresses, quint16(port())};
        screen->setConnectLink(link.toString());
    }
}

CastScreen *ReceiverController::screenById(const QString &screenId) const
{
    if (screenId.isEmpty())
        return nullptr;
    for (CastScreen *screen : m_screens) {
        if (screen->screenId() == screenId)
            return screen;
    }
    return nullptr;
}

CastScreen *ReceiverController::screenCasting(const QString &requestId) const
{
    if (requestId.isEmpty())
        return nullptr;
    for (CastScreen *screen : m_screens) {
        if (screen->active().requestId == requestId)
            return screen;
    }
    return nullptr;
}

int ReceiverController::screenNumber(const QString &screenId) const
{
    const CastScreen *screen = screenById(screenId);
    return screen ? screen->number() : 0;
}

CastScreen *ReceiverController::findTarget(const ConnectionRequest &request) const
{
    if (CastScreen *screen = screenById(request.screenId))
        return screen;
    for (CastScreen *screen : m_screens) {
        if (screen->casting() && screen->active().device.id == request.device.id)
            return screen;
    }
    for (CastScreen *screen : m_screens) {
        if (!screen->casting())
            return screen;
    }
    return canAddScreen() ? nullptr : m_screens.first();
}

CastScreen *ReceiverController::targetFor(const ConnectionRequest &request)
{
    if (CastScreen *screen = findTarget(request))
        return screen;
    CastScreen *screen = appendScreen();
    emit screensChanged();
    return screen;
}

QString ReceiverController::castReplacedBy(const QString &requestId) const
{
    const std::optional<ConnectionRequest> request = m_requests.find(requestId);
    const CastScreen *screen = request ? findTarget(*request) : nullptr;
    if (!screen || !screen->casting() || screen->active().device.id == request->device.id)
        return {};
    return screen->deviceName();
}

bool ReceiverController::isCasting(const QString &requestId) const
{
    const CastScreen *screen = screenCasting(requestId);
    return screen && !screen->demo();
}

void ReceiverController::videoPacket(const QString &requestId, const QByteArray &packet, quint8 flags,
                                     qint64 ptsUs)
{
    if (CastScreen *screen = screenCasting(requestId); screen && !screen->demo())
        screen->videoPacket(packet, flags, ptsUs);
}

void ReceiverController::audioPacket(const QString &requestId, const QByteArray &packet, qint64 ptsUs)
{
    if (CastScreen *screen = screenCasting(requestId); screen && !screen->demo())
        screen->audioPacket(packet, ptsUs);
}

void ReceiverController::videoEnded(const QString &requestId)
{
    if (CastScreen *screen = screenCasting(requestId))
        screen->videoEnded();
}

void ReceiverController::setReceiverName(const QString &name)
{
    QString trimmed = name.trimmed();
    if (trimmed.isEmpty())
        trimmed = defaultReceiverName();
    if (trimmed == m_receiverName)
        return;
    m_receiverName = trimmed;
    QSettings().setValue(kNameKey, m_receiverName);
    emit receiverNameChanged();
    updateConnectLinks();
}

int ReceiverController::port() const
{
    return controlPort();
}

void ReceiverController::setRequireApproval(bool require)
{
    if (require == m_requireApproval)
        return;
    m_requireApproval = require;
    QSettings().setValue(kRequireApprovalKey, m_requireApproval);
    emit requireApprovalChanged();
}

QVariantList ReceiverController::trustedDevices() const
{
    QVariantList list;
    for (const TrustedDevice &device : m_trusted)
        list.append(QVariantMap{{"deviceId", device.id}, {"name", device.name}});
    return list;
}

int ReceiverController::requestTimeoutSeconds() const
{
    return kRequestTimeoutSec;
}

QString ReceiverController::appVersion() const
{
    return QString::fromLatin1(beamr::kVersion);
}

bool ReceiverController::demoAvailable() const
{
#ifdef QT_DEBUG
    return true;
#else
    return qEnvironmentVariableIsSet("BEAMR_DEMO");
#endif
}

void ReceiverController::handleIncomingRequest(const QString &requestId, const beamr::DeviceInfo &device,
                                               const QString &address, const QString &screenId)
{
    addRequest({requestId, device, address, QDateTime(), false, screenId});
}

void ReceiverController::senderLeft(const QString &requestId)
{
    if (const std::optional<ConnectionRequest> request = m_requests.take(requestId)) {
        emit notify(tr("%1 cancelled the request").arg(request->device.name));
        return;
    }
    if (CastScreen *screen = screenCasting(requestId))
        screen->stopCasting();
}

void ReceiverController::addRequest(ConnectionRequest request)
{
    if (request.requestId.isEmpty())
        request.requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    request.expiresAt = QDateTime::currentDateTime().addSecs(kRequestTimeoutSec);

    if (!m_requireApproval || isTrusted(request.device.id)) {
        qCInfo(lcBeamr) << "auto-accepting" << request.device.name << request.address;
        emit requestAnswered(request.requestId, true);
        startCasting(request);
        return;
    }

    qCInfo(lcBeamr) << "cast request from" << request.device.name << request.address;
    m_requests.add(request);
}

void ReceiverController::accept(const QString &requestId, bool alwaysAllow)
{
    const std::optional<ConnectionRequest> request = m_requests.take(requestId);
    if (!request)
        return;
    if (alwaysAllow)
        trust(request->device);
    emit requestAnswered(requestId, true);
    startCasting(*request);
}

void ReceiverController::decline(const QString &requestId)
{
    if (m_requests.take(requestId))
        emit requestAnswered(requestId, false);
}

void ReceiverController::startCasting(const ConnectionRequest &request)
{
    CastScreen *screen = targetFor(request);
    screen->start(request);
    emit notify(m_screens.size() > 1 ? tr("%1 is casting to screen %2").arg(request.device.name).arg(screen->number())
                                     : tr("Casting from %1").arg(request.device.name));
}

QString ReceiverController::nextScreenshotPath()
{
    QDir().mkpath(m_screenshotsDir);
    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz"));
    return QDir(m_screenshotsDir).filePath(QStringLiteral("beamr-%1.png").arg(stamp));
}

void ReceiverController::exportRecording(int row, const QUrl &destination)
{
    const QString source = m_recordings->filePath(row);
    const QString target = destination.toLocalFile();
    if (source.isEmpty() || target.isEmpty() || m_exportWatcher.isRunning())
        return;

    // Copying a multi-GB recording must not block the UI.
    m_exportWatcher.setFuture(QtConcurrent::run([source, target]() -> QString {
        if (QFileInfo(source) == QFileInfo(target))
            return {};
        if (QFile::exists(target) && !QFile::remove(target))
            return QObject::tr("can't replace %1").arg(target);
        QFile file(source);
        if (!file.copy(target))
            return file.errorString();
        return {};
    }));
}

void ReceiverController::trashRecording(int row)
{
    const QString path = m_recordings->filePath(row);
    if (path.isEmpty())
        return;
    const QString name = m_recordings->fileName(row);
    if (QFile::moveToTrash(path))
        emit notify(tr("Moved %1 to the trash").arg(name));
    else
        emit notify(tr("Couldn't move %1 to the trash").arg(name));
    m_recordings->refresh();
}

void ReceiverController::forgetDevice(const QString &deviceId)
{
    const auto removed = m_trusted.removeIf([&](const TrustedDevice &d) { return d.id == deviceId; });
    if (removed == 0)
        return;
    saveTrusted();
    emit trustedDevicesChanged();
}

void ReceiverController::simulateRequest()
{
    if (!demoAvailable())
        return;

    static const std::array<std::pair<const char *, const char *>, 3> phones = {{
        {"Pixel 8 Pro", "Google Pixel 8 Pro"},
        {"Galaxy S24", "Samsung SM-S921B"},
        {"OnePlus 12", "OnePlus CPH2581"},
    }};
    const auto &phone = phones[m_demoCounter % phones.size()];
    const int n = m_demoCounter++;

    ConnectionRequest request;
    request.device = {QStringLiteral("demo-%1").arg(n % phones.size()),
                      QString::fromLatin1(phone.first), QString::fromLatin1(phone.second)};
    request.address = QStringLiteral("192.168.1.%1").arg(40 + n % phones.size());
    request.demo = true;
    addRequest(request);
}

bool ReceiverController::isTrusted(const QString &deviceId) const
{
    return std::any_of(m_trusted.cbegin(), m_trusted.cend(),
                       [&](const TrustedDevice &d) { return d.id == deviceId; });
}

void ReceiverController::trust(const beamr::DeviceInfo &device)
{
    for (TrustedDevice &known : m_trusted) {
        if (known.id == device.id) {
            known.name = device.name;
            saveTrusted();
            emit trustedDevicesChanged();
            return;
        }
    }
    m_trusted.append({device.id, device.name});
    saveTrusted();
    emit trustedDevicesChanged();
}

void ReceiverController::saveTrusted() const
{
    QSettings settings;
    settings.beginWriteArray(kTrustedKey, int(m_trusted.size()));
    for (int i = 0; i < m_trusted.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("id", m_trusted.at(i).id);
        settings.setValue("name", m_trusted.at(i).name);
    }
    settings.endArray();
}

void ReceiverController::refreshAddresses()
{
    QStringList addresses;
    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : interfaces) {
        const auto flags = iface.flags();
        if (!flags.testFlag(QNetworkInterface::IsUp) || !flags.testFlag(QNetworkInterface::IsRunning)
            || flags.testFlag(QNetworkInterface::IsLoopBack) || isVirtualInterface(iface)) {
            continue;
        }
        for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
            const QHostAddress ip = entry.ip();
            if (ip.protocol() == QAbstractSocket::IPv4Protocol && !ip.isLoopback() && !ip.isLinkLocal())
                addresses.append(ip.toString());
        }
    }
    addresses.removeDuplicates();

    if (addresses != m_addresses) {
        m_addresses = addresses;
        emit addressesChanged();
        updateConnectLinks();
    }
}

void ReceiverController::tick()
{
    const QList<ConnectionRequest> expired = m_requests.takeExpired(QDateTime::currentDateTime());
    for (const ConnectionRequest &request : expired) {
        emit requestExpired(request.requestId);
        emit notify(tr("Request from %1 expired").arg(request.device.name));
    }
    m_requests.refreshCountdowns();

    for (CastScreen *screen : std::as_const(m_screens))
        screen->tick();

    if (++m_tickCount % kAddressRefreshTicks == 0)
        refreshAddresses();
}
