#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QThread>
#include <QVideoSink>
#include <QtQml/qqmlregistration.h>

#include "connectionrequestmodel.h"

class AudioPlayer;
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
    void videoPacket(const QByteArray &packet);
    void videoEnded();
    void audioPacket(const QByteArray &packet);
    // Once a second, for the recording clock.
    void tick();

    Q_INVOKABLE void stopCasting();
    Q_INVOKABLE void togglePaused() { setPaused(!m_paused); }
    Q_INVOKABLE void toggleRecording();

signals:
    void numberChanged();
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
    bool m_hasAudio = false;
    bool m_audible = false;
};
