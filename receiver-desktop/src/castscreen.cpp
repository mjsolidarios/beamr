#include "castscreen.h"

#include <QRandomGenerator>
#include <QVideoFrame>

#include <beamr/log.h>

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

    m_active = request;
    if (m_paused) {
        m_paused = false;
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
    if (m_paused) {
        m_paused = false;
        emit pausedChanged();
    }
    emit castingChanged();
    emit castStopped(requestId);
}

void CastScreen::videoPacket(const QByteArray &packet)
{
    // Keep decoding while paused so resuming shows the current picture at once.
    QMetaObject::invokeMethod(m_decoder, [decoder = m_decoder, packet, generation = m_videoGeneration] {
        decoder->decode(packet, generation);
    });
}

void CastScreen::videoEnded()
{
    resetVideo();
}

void CastScreen::showFrame(const QVideoFrame &frame, int generation)
{
    // A frame still in flight from a stream that has ended.
    if (generation != m_videoGeneration || !casting() || m_active.demo)
        return;
    if (!m_paused && m_videoSink)
        m_videoSink->setVideoFrame(frame);
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
    emit recordingChanged();
    emit recordingSecondsChanged();
}

void CastScreen::stopRecording()
{
    m_recording = false;
    emit recordingChanged();
    emit recordingSecondsChanged();
    // The recorder that writes the stream to disk arrives with video decoding
    // (Milestone 2); until then only the demo can reach this.
    emit notify(m_active.demo ? tr("Demo recording stopped. Demo mode doesn't write files.")
                              : tr("Recording stopped"));
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
