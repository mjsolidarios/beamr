#include "castscreen.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QVideoFrame>

#include <beamr/log.h>

#include "audioplayer.h"
#include "recorder.h"
#include "videodecoder.h"

namespace {

// Short enough to keep the QR code sparse; only needs to be unique among
// one receiver's screens.
QString newScreenId()
{
    static const char alphabet[] = "abcdefghijkmnpqrstuvwxyz23456789";
    QString id;
    for (int i = 0; i < 6; ++i)
        id.append(QLatin1Char(alphabet[QRandomGenerator::global()->bounded(int(sizeof alphabet - 1))]));
    return id;
}

} // namespace

CastScreen::CastScreen(int number, QObject *parent)
    : QObject(parent)
    , m_id(newScreenId())
    , m_number(number)
{
    m_decoder = new VideoDecoder;
    m_decoder->moveToThread(&m_decoderThread);
    connect(&m_decoderThread, &QThread::finished, m_decoder, &QObject::deleteLater);
    connect(m_decoder, &VideoDecoder::frameDecoded, this, &CastScreen::showFrame);
    m_audio = new AudioPlayer;
    m_audio->moveToThread(&m_decoderThread);
    connect(&m_decoderThread, &QThread::finished, m_audio, &QObject::deleteLater);
    m_recorder = new Recorder;
    m_recorder->moveToThread(&m_decoderThread);
    connect(&m_decoderThread, &QThread::finished, m_recorder, &QObject::deleteLater);
    connect(m_recorder, &Recorder::finished, this, [this](const QString &path, const QString &error) {
        if (!error.isEmpty()) {
            emit notify(tr("Recording failed: %1").arg(error));
        } else {
            emit notify(tr("Saved %1").arg(QFileInfo(path).fileName()));
            emit recordingSaved();
        }
    });
    m_decoderThread.setObjectName(QStringLiteral("beamr-decoder-%1").arg(m_id));
    m_decoderThread.start();
}

CastScreen::~CastScreen()
{
    m_decoderThread.quit();
    m_decoderThread.wait();
}

void CastScreen::setNumber(int number)
{
    if (number == m_number)
        return;
    m_number = number;
    emit numberChanged();
}

void CastScreen::setConnectLink(const QString &link)
{
    if (link == m_connectLink)
        return;
    m_connectLink = link;
    emit connectLinkChanged();
}

void CastScreen::setVideoSink(QVideoSink *sink)
{
    if (sink == m_videoSink)
        return;
    m_videoSink = sink;
    emit videoSinkChanged();
}

void CastScreen::setPaused(bool paused)
{
    if (paused == m_paused || (paused && !casting()))
        return;
    m_paused = paused;

    // Recording follows pause, so a paused cast never ends up in the file.
    if (m_recording) {
        if (m_paused)
            m_recordedMs += m_recordingSegment.elapsed();
        else
            m_recordingSegment.restart();
        m_lastRecordingSeconds = recordingSeconds();
        emit recordingSecondsChanged();
    }
    updateMute();
    QMetaObject::invokeMethod(m_recorder, [recorder = m_recorder, paused] { recorder->setPaused(paused); });
    if (!paused && m_recording && !m_active.demo)
        emit keyFrameNeeded(m_active.requestId);
    emit pausedChanged();
}

int CastScreen::recordingSeconds() const
{
    if (!m_recording)
        return 0;
    const qint64 ms = m_recordedMs + (m_paused ? 0 : m_recordingSegment.elapsed());
    return int(ms / 1000);
}

void CastScreen::start(const ConnectionRequest &request)
{
    if (casting()) {
        if (m_recording)
            stopRecording();
        emit castStopped(m_active.requestId);
    }
    resetVideo();
    resetAudio();

    m_active = request;
    if (m_paused) {
        m_paused = false;
        updateMute();
        emit pausedChanged();
    }
    emit castingChanged();
}

void CastScreen::stopCasting()
{
    if (!casting())
        return;
    const QString name = m_active.device.name;
    end();
    emit notify(tr("%1 stopped casting").arg(name));
}

void CastScreen::end()
{
    if (m_recording)
        stopRecording();
    const QString requestId = m_active.requestId;
    m_active = {};
    resetVideo();
    resetAudio();
    if (m_paused) {
        m_paused = false;
        updateMute();
        emit pausedChanged();
    }
    emit castingChanged();
    emit castStopped(requestId);
}

void CastScreen::videoPacket(const QByteArray &packet, quint8 flags, qint64 ptsUs)
{
    // Keep decoding while paused so resuming shows the current picture at once.
    QMetaObject::invokeMethod(m_decoder, [decoder = m_decoder, packet, generation = m_videoGeneration] {
        decoder->decode(packet, generation);
    });
    if (m_recording) {
        QMetaObject::invokeMethod(m_recorder, [recorder = m_recorder, packet, flags, ptsUs] {
            recorder->video(packet, flags, ptsUs);
        });
    }
}

void CastScreen::videoEnded()
{
    resetVideo();
    resetAudio();
}

void CastScreen::audioPacket(const QByteArray &packet, qint64 ptsUs)
{
    QMetaObject::invokeMethod(m_audio, [audio = m_audio, packet] { audio->decode(packet); });
    if (m_recording) {
        QMetaObject::invokeMethod(m_recorder, [recorder = m_recorder, packet, ptsUs] {
            recorder->audio(packet, ptsUs);
        });
    }
    if (!m_hasAudio) {
        m_hasAudio = true;
        qCInfo(lcCodec) << "screen" << m_number << "has sound";
        emit hasAudioChanged();
    }
}

void CastScreen::resetAudio()
{
    QMetaObject::invokeMethod(m_audio, &AudioPlayer::reset);
    if (m_hasAudio) {
        m_hasAudio = false;
        emit hasAudioChanged();
    }
}

void CastScreen::setAudible(bool audible)
{
    if (audible == m_audible)
        return;
    m_audible = audible;
    updateMute();
    emit audibleChanged();
}

void CastScreen::updateMute()
{
    // A paused picture with live sound would be confusing; pause both.
    QMetaObject::invokeMethod(m_audio, [audio = m_audio, muted = !m_audible || m_paused] { audio->setMuted(muted); });
}

void CastScreen::showFrame(const QVideoFrame &frame, int generation)
{
    // A frame still in flight from a stream that has ended.
    if (generation != m_videoGeneration || !casting() || m_active.demo)
        return;
    if (!m_paused && m_videoSink)
        m_videoSink->setVideoFrame(frame);
    m_videoSize = frame.size();
    if (!m_hasVideo) {
        m_hasVideo = true;
        qCInfo(lcCodec) << "screen" << m_number << "first frame" << frame.size();
        emit hasVideoChanged();
    }
}

void CastScreen::resetVideo()
{
    ++m_videoGeneration;
    QMetaObject::invokeMethod(m_decoder, &VideoDecoder::reset);
    if (m_videoSink)
        m_videoSink->setVideoFrame({});
    if (m_hasVideo) {
        m_hasVideo = false;
        emit hasVideoChanged();
    }
}

void CastScreen::toggleRecording()
{
    if (m_recording) {
        stopRecording();
        return;
    }
    if (!casting())
        return;

    m_recording = true;
    m_recordedMs = 0;
    m_recordingSegment.start();
    m_lastRecordingSeconds = 0;
    if (!m_active.demo) {
        // beamr-Pixel-8-20260930-142501.mp4
        QString device = m_active.device.name;
        device.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9._-]+")), QStringLiteral("-"));
        const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
        const QString path = QDir(m_recordingsFolder).filePath(QStringLiteral("beamr-%1-%2.mp4").arg(device, stamp));
        QMetaObject::invokeMethod(m_recorder, [recorder = m_recorder, path, size = m_videoSize, audio = m_hasAudio] {
            recorder->start(path, size, audio);
        });
        // Start now rather than at the phone's next scheduled keyframe.
        emit keyFrameNeeded(m_active.requestId);
    }
    emit recordingChanged();
    emit recordingSecondsChanged();
}

void CastScreen::stopRecording()
{
    m_recording = false;
    emit recordingChanged();
    emit recordingSecondsChanged();
    if (m_active.demo) {
        emit notify(tr("Demo recording stopped. Demo mode doesn't write files."));
        return;
    }
    // Reports back through Recorder::finished: saved, or why not.
    QMetaObject::invokeMethod(m_recorder, &Recorder::stop);
}

void CastScreen::tick()
{
    if (!m_recording)
        return;
    const int seconds = recordingSeconds();
    if (seconds != m_lastRecordingSeconds) {
        m_lastRecordingSeconds = seconds;
        emit recordingSecondsChanged();
    }
}
