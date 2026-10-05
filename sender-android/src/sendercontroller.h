#pragma once

#include <QObject>
#include <QSize>
#include <QThread>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <beamr/device.h>
#include <beamr/notices.h>

#include "sessionmodel.h"

class ReceiverSession;
class DiscoveryClient;
class StreamSender;

// What the home screen drives: the receivers this phone is connected to,
// and the one screen capture whose video goes to every receiver that
// allowed it. Also remembers receivers that answered, so they're one tap
// away next time.
class SenderController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(SessionModel *sessions READ sessions CONSTANT)
    Q_PROPERTY(int approvedCount READ approvedCount NOTIFY sessionsChanged)
    Q_PROPERTY(int streamingCount READ streamingCount NOTIFY sessionsChanged)
    // Receivers that dropped off and are being reconnected to.
    Q_PROPERTY(int reconnectingCount READ reconnectingCount NOTIFY sessionsChanged)
    Q_PROPERTY(bool canAddReceiver READ canAddReceiver NOTIFY sessionsChanged)
    Q_PROPERTY(int maxReceivers READ maxReceivers CONSTANT)
    Q_PROPERTY(CastState castState READ castState NOTIFY castStateChanged)
    Q_PROPERTY(QSize castSize READ castSize NOTIFY castSizeChanged)
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)
    Q_PROPERTY(bool messageIsError READ messageIsError NOTIFY messageChanged)
    // The receiver the message is about, when trying it again might help.
    Q_PROPERTY(QString retryAddress READ retryAddress NOTIFY messageChanged)
    Q_PROPERTY(QString deviceName READ deviceName WRITE setDeviceName NOTIFY deviceNameChanged)
    Q_PROPERTY(QString deviceModel READ deviceModel CONSTANT)
    Q_PROPERTY(QString lastAddress READ lastAddress NOTIFY recentReceiversChanged)
    Q_PROPERTY(QVariantList recentReceivers READ recentReceivers NOTIFY recentReceiversChanged)
    // Receivers answering on this network right now, not already connected:
    // [{name, address, freeScreens}]. Filled while `discovering` is on.
    Q_PROPERTY(QVariantList nearbyReceivers READ nearbyReceivers NOTIFY nearbyReceiversChanged)
    Q_PROPERTY(bool discovering READ discovering WRITE setDiscovering NOTIFY discoveringChanged)
    Q_PROPERTY(int defaultPort READ defaultPort CONSTANT)
    Q_PROPERTY(bool canScan READ canScan CONSTANT)
    // The first-launch introduction has been seen (or skipped).
    Q_PROPERTY(bool onboardingDone READ onboardingDone WRITE setOnboardingDone NOTIFY onboardingDoneChanged)
    // Android's text size setting (Settings > Display > Font size), so text
    // grows for people who need it. Theme.sp() applies it.
    Q_PROPERTY(qreal fontScale READ fontScale NOTIFY fontScaleChanged)
    // For the About page: the Qt this build runs on.
    Q_PROPERTY(QString qtVersion READ qtVersion CONSTANT)
    // Cast what apps play, too. Takes effect on the next cast.
    Q_PROPERTY(bool shareAudio READ shareAudio WRITE setShareAudio NOTIFY shareAudioChanged)
    // Picture quality for casts: Smooth (1080p60), Balanced (1080p30) or
    // DataSaver (720p30, for weak Wi-Fi). Takes effect on the next cast.
    Q_PROPERTY(Quality quality READ quality WRITE setQuality NOTIFY qualityChanged)
    // Frames a second for the chosen quality, for showing it.
    Q_PROPERTY(int frameRate READ frameRate NOTIFY qualityChanged)
    // How sound went for the current cast: on, off, denied, unavailable;
    // empty when not casting.
    Q_PROPERTY(QString audioState READ audioState NOTIFY audioStateChanged)
    // Qt::ColorScheme: Unknown follows the system.
    Q_PROPERTY(int colorScheme READ colorScheme WRITE setColorScheme NOTIFY colorSchemeChanged)

public:
    // Mirrors ReceiverSession::State for QML.
    enum SessionState { Connecting, AwaitingApproval, Ready, Streaming, Reconnecting };
    Q_ENUM(SessionState)
    // Starting: waiting for the user to allow screen capture.
    enum CastState { Off, Starting, On };
    Q_ENUM(CastState)
    // Keep in step with ScreenCaptureService.QUALITY_*.
    enum Quality { Smooth, Balanced, DataSaver };
    Q_ENUM(Quality)

    explicit SenderController(QObject *parent = nullptr);
    ~SenderController() override;

    SessionModel *sessions() { return &m_sessions; }
    int approvedCount() const;
    int streamingCount() const;
    int reconnectingCount() const;
    bool canAddReceiver() const;
    int maxReceivers() const;
    CastState castState() const { return m_castState; }
    bool canScan() const;
    QString qtVersion() const { return QString::fromLatin1(qVersion()); }
    qreal fontScale() const { return m_fontScale; }
    bool onboardingDone() const { return m_onboardingDone; }
    void setOnboardingDone(bool done);
    Quality quality() const { return m_quality; }
    void setQuality(Quality quality);
    // For the cast in progress, else for the next one.
    int frameRate() const { return (m_castState != Off ? m_castQuality : m_quality) == Smooth ? 60 : 30; }
    bool shareAudio() const { return m_shareAudio; }
    void setShareAudio(bool share);
    QString audioState() const { return m_audioState; }
    int colorScheme() const;
    void setColorScheme(int scheme);
    QSize castSize() const { return m_castSize; }
    QString message() const { return m_message; }
    bool messageIsError() const { return m_messageIsError; }
    QString retryAddress() const { return m_retryAddress; }
    QString deviceName() const { return m_device.name; }
    void setDeviceName(const QString &name);
    QString deviceModel() const { return m_device.model; }
    QString lastAddress() const;
    QVariantList recentReceivers() const { return m_recent; }
    QVariantList nearbyReceivers() const;
    bool discovering() const { return m_discovering; }
    // On while the home screen is showing and the app is in front.
    void setDiscovering(bool discovering);
    int defaultPort() const;

    // Empty when `address` can be added, otherwise what's wrong with it.
    Q_INVOKABLE QString validateAddress(const QString &address) const;
    // `screen` and `pair` come from a receiver's QR code: which of its
    // screens, and the one-time code that skips asking. `key` is the
    // certificate fingerprint from that code; empty pins the first
    // certificate the connection sees.
    Q_INVOKABLE void addReceiver(const QString &address, const QString &screen = {}, const QString &pair = {},
                                 const QString &key = {});
    Q_INVOKABLE QString openSourceNotices() const { return beamr::notices::text(); }
    Q_INVOKABLE QVariantList openSourceLicenses() const { return beamr::notices::licenses(); }
    Q_INVOKABLE QString openSourceLicense(const QString &id) const { return beamr::notices::license(id); }
    Q_INVOKABLE void removeReceiver(const QString &sessionId);
    Q_INVOKABLE void disconnectAll();
    Q_INVOKABLE bool isConnectedTo(const QString &address) const;
    Q_INVOKABLE void startCasting();
    Q_INVOKABLE void stopCasting();
    Q_INVOKABLE void forgetReceiver(const QString &address);
    // Opens the QR scanner; a receiver's code connects to it.
    Q_INVOKABLE void scanQrCode();
    Q_INVOKABLE void clearMessage();
    // Sends the app to the background without quitting, so a cast survives Back.
    Q_INVOKABLE void moveToBackground();
    // Short vibration marking an outcome; no-op off Android.
    Q_INVOKABLE void haptic(bool success);

signals:
    void sessionsChanged();
    void castStateChanged();
    void castSizeChanged();
    void messageChanged();
    void deviceNameChanged();
    void recentReceiversChanged();
    void nearbyReceiversChanged();
    void discoveringChanged();
    void colorSchemeChanged();
    void shareAudioChanged();
    void qualityChanged();
    void onboardingDoneChanged();
    void fontScaleChanged();
    void audioStateChanged();

private:
    void setMessage(const QString &message, bool isError, const QString &retryAddress = {});
    void setCastState(CastState state);
    void updateSystemBars();
    // From the Quick Settings tile: cast to the computer used last.
    void quickCast();
    void refreshFontScale();
    void qrScanned(const QString &text);
    void qrScanFailed(const QString &reason);
    void onSessionApproved(ReceiverSession *session);
    void onSessionEnded(ReceiverSession *session, const QString &message, bool isError);
    void openVideo(ReceiverSession *session);
    // Stops capture and every video connection; sessions stay up.
    void endCapture();
    void rememberReceiver(const ReceiverSession *session);
    void saveRecent() const;

    // Called by the Android capture service through CaptureNatives.
    friend struct CaptureNatives;
    friend struct ScanNatives;
    void captureStarted(QSize size);
    void setAudioState(const QString &state);
    void captureStopped(const QString &reason);
    void videoOpened(const QString &sessionId);
    void videoClosed(const QString &sessionId, const QString &error);

    beamr::DeviceInfo m_device;
    SessionModel m_sessions;
    CastState m_castState = Off;
    bool m_captureRunning = false;
    QSize m_castSize;
    QThread m_streamThread;
    StreamSender *m_stream = nullptr;
    DiscoveryClient *m_discovery = nullptr;
    bool m_discovering = false;
    QString m_message;
    bool m_messageIsError = false;
    QString m_retryAddress;
    bool m_shareAudio = true;
    Quality m_quality = Smooth;
    Quality m_castQuality = Smooth;
    bool m_onboardingDone = false;
    qreal m_fontScale = 1.0;
    QString m_audioState;
    QVariantList m_recent;
};
