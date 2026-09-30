#pragma once

#include <QObject>
#include <QSize>
#include <QThread>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <beamr/device.h>

#include "sessionmodel.h"

class ReceiverSession;
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
    Q_PROPERTY(int defaultPort READ defaultPort CONSTANT)
    Q_PROPERTY(bool canScan READ canScan CONSTANT)
    // Qt::ColorScheme: Unknown follows the system.
    Q_PROPERTY(int colorScheme READ colorScheme WRITE setColorScheme NOTIFY colorSchemeChanged)

public:
    // Mirrors ReceiverSession::State for QML.
    enum SessionState { Connecting, AwaitingApproval, Ready, Streaming };
    Q_ENUM(SessionState)
    // Starting: waiting for the user to allow screen capture.
    enum CastState { Off, Starting, On };
    Q_ENUM(CastState)

    explicit SenderController(QObject *parent = nullptr);
    ~SenderController() override;

    SessionModel *sessions() { return &m_sessions; }
    int approvedCount() const;
    int streamingCount() const;
    bool canAddReceiver() const;
    int maxReceivers() const;
    CastState castState() const { return m_castState; }
    bool canScan() const;
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
    int defaultPort() const;

    // Empty when `address` can be added, otherwise what's wrong with it.
    Q_INVOKABLE QString validateAddress(const QString &address) const;
    // `screen` picks one of the receiver's screens (from its QR code).
    Q_INVOKABLE void addReceiver(const QString &address, const QString &screen = {});
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
    void colorSchemeChanged();

private:
    void setMessage(const QString &message, bool isError, const QString &retryAddress = {});
    void setCastState(CastState state);
    void updateSystemBars();
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
    QString m_message;
    bool m_messageIsError = false;
    QString m_retryAddress;
    QVariantList m_recent;
};
