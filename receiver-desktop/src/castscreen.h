#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QSize>
#include <QThread>
#include <QTimer>
#include <QVideoSink>
#include <QtQml/qqmlregistration.h>

#include "connectionrequestmodel.h"

class AudioPlayer;
class Recorder;
class QVideoFrame;
class VideoDecoder;

// One place on the receiver a phone can cast to: waiting with its own QR
// code, or showing one phone's screen with its own pause, recording and
// video. ReceiverController owns them.
class CastScreen : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by ReceiverController")

    Q_PROPERTY(QString screenId READ screenId CONSTANT)
    // 1-based, as shown to people; changes when an earlier screen goes.
    Q_PROPERTY(int number READ number NOTIFY numberChanged)
    Q_PROPERTY(QString connectLink READ connectLink NOTIFY connectLinkChanged)
    Q_PROPERTY(bool casting READ casting NOTIFY castingChanged)
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY castingChanged)
    Q_PROPERTY(QString deviceAddress READ deviceAddress NOTIFY castingChanged)
    Q_PROPERTY(bool demo READ demo NOTIFY castingChanged)
    // The phone dropped off; its screen is held for it to come back.
    Q_PROPERTY(bool reconnecting READ reconnecting NOTIFY reconnectingChanged)
    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
    Q_PROPERTY(int recordingSeconds READ recordingSeconds NOTIFY recordingSecondsChanged)
    Q_PROPERTY(QVideoSink *videoSink READ videoSink WRITE setVideoSink NOTIFY videoSinkChanged)
    Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY hasVideoChanged)
    // The phone is sending sound.
    Q_PROPERTY(bool hasAudio READ hasAudio NOTIFY hasAudioChanged)
    // Its sound plays; only one screen's does at a time (ReceiverController).
    Q_PROPERTY(bool audible READ audible NOTIFY audibleChanged)

public:
    explicit CastScreen(int number, QObject *parent = nullptr);
    ~CastScreen() override;

    QString screenId() const { return m_id; }
    int number() const { return m_number; }
    void setNumber(int number);
    QString connectLink() const { return m_connectLink; }
    void setConnectLink(const QString &link);

    bool casting() const { return !m_active.requestId.isEmpty(); }
    const ConnectionRequest &active() const { return m_active; }
    QString deviceName() const { return m_active.device.name; }
    QString deviceAddress() const { return m_active.address; }
    bool demo() const { return m_active.demo; }
    bool reconnecting() const { return m_reconnecting; }
    // One-time code in this screen's QR; a phone that shows it may cast here
    // without asking. Replaced each time it's used.
    QString pairToken() const { return m_pairToken; }
    void rotatePairToken();
    // Lets the casting phone reconnect here after a drop.
    QString resumeToken() const { return m_resumeToken; }
    bool paused() const { return m_paused; }
    void setPaused(bool paused);
    bool recording() const { return m_recording; }
    int recordingSeconds() const;
    QVideoSink *videoSink() const { return m_videoSink; }
    void setVideoSink(QVideoSink *sink);
    bool hasVideo() const { return m_hasVideo; }
    bool hasAudio() const { return m_hasAudio; }
    bool audible() const { return m_audible; }
    void setAudible(bool audible);

    // Shows `request`'s phone here, replacing whoever was casting.
    void start(const ConnectionRequest &request);
    // The phone's connection dropped: hold the screen for kResumeGraceMs.
    void holdForReconnect();
    // The same phone is back on a new connection; the cast carries on.
    void resume(const ConnectionRequest &request);
    void videoPacket(const QByteArray &packet, quint8 flags, qint64 ptsUs);
    void videoEnded();
    void audioPacket(const QByteArray &packet, qint64 ptsUs);
    void setRecordingsFolder(const QString &folder) { m_recordingsFolder = folder; }
    // Once a second, for the recording clock.
    void tick();

    Q_INVOKABLE void stopCasting();
    Q_INVOKABLE void togglePaused() { setPaused(!m_paused); }
    Q_INVOKABLE void toggleRecording();

signals:
    void numberChanged();
    void reconnectingChanged();
    // For ReceiverController: the pair code changed, so the QR must too.
    void pairTokenChanged();
    void connectLinkChanged();
    void castingChanged();
    void pausedChanged();
    void recordingChanged();
    void recordingSecondsChanged();
    void videoSinkChanged();
    void hasVideoChanged();
    void hasAudioChanged();
    void audibleChanged();
    // For ReceiverController: toasts, and hanging up on the phone.
    void notify(const QString &message);
    void castStopped(const QString &requestId);
    void recordingSaved();
    // The recorder needs a keyframe to start or resume from.
    void keyFrameNeeded(const QString &requestId);

private:
    void end();
    void stopRecording();
    void showFrame(const QVideoFrame &frame, int generation);
    void resetVideo();
    void resetAudio();
    void updateMute();

    const QString m_id;
    int m_number;
    QString m_connectLink;
    ConnectionRequest m_active;
    bool m_paused = false;
    QString m_pairToken;
    QString m_resumeToken;
    bool m_reconnecting = false;
    QTimer m_reconnectTimer;

    bool m_recording = false;
    qint64 m_recordedMs = 0;
    QElapsedTimer m_recordingSegment;
    int m_lastRecordingSeconds = 0;

    QPointer<QVideoSink> m_videoSink;
    bool m_hasVideo = false;
    // Bumped whenever the picture is cleared; older frames are dropped.
    int m_videoGeneration = 0;
    // Each screen decodes (and plays its sound) on its own thread, so one
    // busy stream can't stall the others.
    QThread m_decoderThread;
    VideoDecoder *m_decoder = nullptr;
    AudioPlayer *m_audio = nullptr;
    // Writes the stream to disk while recording; same thread as decoding.
    Recorder *m_recorder = nullptr;
    QString m_recordingsFolder;
    QSize m_videoSize;
    bool m_hasAudio = false;
    bool m_audible = false;
};
