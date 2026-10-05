#pragma once

#include <QFutureWatcher>
#include <QMediaDevices>
#include <QList>
#include <QObject>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <beamr/device.h>
#include <beamr/notices.h>
#include <beamr/tlsidentity.h>

#include "castscreen.h"
#include "connectionrequestmodel.h"
#include "recordingsmodel.h"

// Receiver state shown by the UI: its screens and who casts to each, who
// may cast, and the recordings, screenshots and export tools.
class ReceiverController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // Casting when any screen is.
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(int castingCount READ castingCount NOTIFY stateChanged)
    Q_PROPERTY(QList<CastScreen *> screens READ screens NOTIFY screensChanged)
    Q_PROPERTY(int maxScreens READ maxScreens CONSTANT)
    Q_PROPERTY(bool canAddScreen READ canAddScreen NOTIFY screensChanged)
    // The one screen whose sound plays, so phones don't talk over each
    // other; null for silence. The first cast gets it.
    Q_PROPERTY(CastScreen *audioScreen READ audioScreen WRITE setAudioScreen NOTIFY audioScreenChanged)
    // Qt::ColorScheme: Unknown follows the system.
    Q_PROPERTY(int colorScheme READ colorScheme WRITE setColorScheme NOTIFY colorSchemeChanged)
    // Speakers and headphones to play phones' sound on: [{id, name}], the
    // system default first with an empty id.
    Q_PROPERTY(QVariantList audioOutputs READ audioOutputs NOTIFY audioOutputsChanged)
    Q_PROPERTY(QString audioOutput READ audioOutput WRITE setAudioOutput NOTIFY audioOutputChanged)
    Q_PROPERTY(QString receiverName READ receiverName WRITE setReceiverName NOTIFY receiverNameChanged)
    Q_PROPERTY(QStringList addresses READ addresses NOTIFY addressesChanged)
    Q_PROPERTY(int port READ port CONSTANT)
    Q_PROPERTY(bool requireApproval READ requireApproval WRITE setRequireApproval NOTIFY requireApprovalChanged)
    Q_PROPERTY(QVariantList trustedDevices READ trustedDevices NOTIFY trustedDevicesChanged)
    Q_PROPERTY(bool exporting READ exporting NOTIFY exportingChanged)
    Q_PROPERTY(ConnectionRequestModel *requests READ requests CONSTANT)
    Q_PROPERTY(RecordingsModel *recordings READ recordings CONSTANT)
    Q_PROPERTY(int requestTimeoutSeconds READ requestTimeoutSeconds CONSTANT)
    Q_PROPERTY(QUrl recordingsFolder READ recordingsFolder CONSTANT)
    Q_PROPERTY(QUrl screenshotsFolder READ screenshotsFolder CONSTANT)
    Q_PROPERTY(bool demoAvailable READ demoAvailable CONSTANT)
    // For the About section: the Qt this build runs on.
    Q_PROPERTY(QString qtVersion READ qtVersion CONSTANT)
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)

public:
    enum State { Idle, Casting };
    Q_ENUM(State)

    explicit ReceiverController(QObject *parent = nullptr);
    ~ReceiverController() override;

    State state() const { return castingCount() > 0 ? Casting : Idle; }
    int castingCount() const;
    QList<CastScreen *> screens() const { return m_screens; }
    int maxScreens() const;
    bool canAddScreen() const { return m_screens.size() < maxScreens(); }
    CastScreen *audioScreen() const { return m_audioScreen; }
    void setAudioScreen(CastScreen *screen);
    int colorScheme() const { return m_colorScheme; }
    void setColorScheme(int scheme);
    QVariantList audioOutputs() const;
    QString audioOutput() const { return m_audioOutput; }
    void setAudioOutput(const QString &id);
    QString receiverName() const { return m_receiverName; }
    void setReceiverName(const QString &name);
    QStringList addresses() const { return m_addresses; }
    int port() const;
    bool requireApproval() const { return m_requireApproval; }
    void setRequireApproval(bool require);
    QVariantList trustedDevices() const;
    bool exporting() const { return m_exportWatcher.isRunning(); }
    ConnectionRequestModel *requests() { return &m_requests; }
    RecordingsModel *recordings() { return m_recordings; }
    int requestTimeoutSeconds() const;
    QUrl recordingsFolder() const { return QUrl::fromLocalFile(m_recordingsDir); }
    QUrl screenshotsFolder() const { return QUrl::fromLocalFile(m_screenshotsDir); }
    bool demoAvailable() const;
    QString qtVersion() const { return QString::fromLatin1(qVersion()); }
    QString appVersion() const;

    // Entry points for the network layer: a sender asked to cast (to
    // `screenId`, or anywhere when empty), or hung up before or during its
    // cast.
    void handleIncomingRequest(const QString &requestId, const beamr::DeviceInfo &device,
                               const QString &address, const QString &screenId, const QString &pair,
                               const QString &resume);
    // `intentional`: it said bye. Otherwise its screen waits for it to come back.
    void senderLeft(const QString &requestId, bool intentional);
    // Sent with an accepted answer so the phone can come back after a drop.
    QString resumeTokenFor(const QString &requestId) const;
    // The sender's video: whether a stream for `requestId` may start, its
    // packets, and its end.
    bool isCasting(const QString &requestId) const;
    void videoPacket(const QString &requestId, const QByteArray &packet, quint8 flags, qint64 ptsUs);
    void videoEnded(const QString &requestId);
    void audioPacket(const QString &requestId, const QByteArray &packet, qint64 ptsUs);

    Q_INVOKABLE void addScreen();
    // Stops its cast, if any. The last screen stays.
    Q_INVOKABLE void removeScreen(CastScreen *screen);
    // For request cards: which screen a phone asked for (0 for any), and
    // whose cast allowing it would end ("" when none).
    Q_INVOKABLE int screenNumber(const QString &screenId) const;
    Q_INVOKABLE QString castReplacedBy(const QString &requestId) const;
    Q_INVOKABLE void accept(const QString &requestId, bool alwaysAllow);
    Q_INVOKABLE void decline(const QString &requestId);
    Q_INVOKABLE QString nextScreenshotPath();
    Q_INVOKABLE void exportRecording(int row, const QUrl &destination);
    Q_INVOKABLE void trashRecording(int row);
    Q_INVOKABLE void forgetDevice(const QString &deviceId);
    Q_INVOKABLE void simulateRequest();
    Q_INVOKABLE QString openSourceNotices() const { return beamr::notices::text(); }
    Q_INVOKABLE QVariantList openSourceLicenses() const { return beamr::notices::licenses(); }
    Q_INVOKABLE QString openSourceLicense(const QString &id) const { return beamr::notices::license(id); }

signals:
    void stateChanged();
    void screensChanged();
    void audioScreenChanged();
    void colorSchemeChanged();
    void audioOutputsChanged();
    void audioOutputChanged();
    void receiverNameChanged();
    void addressesChanged();
    void requireApprovalChanged();
    void trustedDevicesChanged();
    void exportingChanged();
    void notify(const QString &message);

    // For the network layer.
    void requestAnswered(const QString &requestId, bool accepted);
    void requestExpired(const QString &requestId);
    void castStopped(const QString &requestId);
    void keyFrameNeeded(const QString &requestId);

private:
    struct TrustedDevice
    {
        QString id;
        QString name;
    };

    CastScreen *appendScreen();
    CastScreen *screenById(const QString &screenId) const;
    CastScreen *screenCasting(const QString &requestId) const;
    // Where an allowed request goes: the screen it asked for, else the one
    // this phone already casts to, else a free one, else a new one (null),
    // else the first.
    CastScreen *findTarget(const ConnectionRequest &request) const;
    CastScreen *targetFor(const ConnectionRequest &request);
    void renumberScreens();
    // Keeps the sound on a casting screen when its owner stops.
    void onCastingChanged(CastScreen *screen);
    void updateConnectLinks();
    void addRequest(ConnectionRequest request);
    void startCasting(const ConnectionRequest &request);
    bool isTrusted(const QString &deviceId) const;
    void trust(const beamr::DeviceInfo &device);
    void saveTrusted() const;
    void refreshAddresses();
    void tick();

    QList<CastScreen *> m_screens;
    CastScreen *m_audioScreen = nullptr;
    QString m_audioOutput;
    int m_colorScheme = 0;
    QMediaDevices *m_mediaDevices = nullptr;
    QString m_receiverName;
    beamr::TlsIdentity m_identity;
    QStringList m_addresses;
    bool m_requireApproval = true;
    QList<TrustedDevice> m_trusted;

    QString m_recordingsDir;
    QString m_screenshotsDir;
    ConnectionRequestModel m_requests;
    RecordingsModel *m_recordings = nullptr;
    QFutureWatcher<QString> m_exportWatcher;
    QTimer m_ticker;
    int m_tickCount = 0;
    int m_demoCounter = 0;
};
