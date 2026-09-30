package com.beamr.sender;

import android.annotation.SuppressLint;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioPlaybackCaptureConfiguration;
import android.media.AudioRecord;
import android.media.MediaCodec;
import android.media.MediaCodecList;
import android.media.MediaFormat;
import android.media.projection.MediaProjection;
import android.util.Log;

import java.nio.ByteBuffer;

// Records what apps play (not the microphone) through the screen's
// MediaProjection and encodes it to Opus for the receivers. Apps that opt
// out of capture, and calls, stay silent; that's Android's rule.
final class AudioCapture {
    private static final String TAG = "beamr";
    private static final String MIME = MediaFormat.MIMETYPE_AUDIO_OPUS;
    // Keep in step with beamr/protocol.h.
    private static final int SAMPLE_RATE = 48_000;
    private static final int CHANNELS = 2;
    private static final int BITRATE_BPS = 128_000;
    // 20 ms, Opus's natural frame size.
    private static final int CHUNK_FRAMES = SAMPLE_RATE / 50;
    private static final int CHUNK_BYTES = CHUNK_FRAMES * CHANNELS * 2;
    private static final long DEQUEUE_TIMEOUT_US = 10_000;

    private final AudioRecord mRecord;
    private final MediaCodec mCodec;
    private final Thread mThread;
    private volatile boolean mRunning = true;

    private AudioCapture(AudioRecord record, MediaCodec codec) {
        mRecord = record;
        mCodec = codec;
        mThread = new Thread(this::run, "beamr-audio");
    }

    // Null when this phone can't: no Opus encoder, or the recorder won't
    // start. The caller has checked the RECORD_AUDIO permission.
    @SuppressLint("MissingPermission")
    static AudioCapture start(MediaProjection projection) {
        MediaFormat format = MediaFormat.createAudioFormat(MIME, SAMPLE_RATE, CHANNELS);
        format.setInteger(MediaFormat.KEY_BIT_RATE, BITRATE_BPS);
        String encoder = new MediaCodecList(MediaCodecList.REGULAR_CODECS).findEncoderForFormat(format);
        if (encoder == null) {
            Log.w(TAG, "no Opus encoder; casting without sound");
            return null;
        }

        AudioPlaybackCaptureConfiguration config = new AudioPlaybackCaptureConfiguration.Builder(projection)
                .addMatchingUsage(AudioAttributes.USAGE_MEDIA)
                .addMatchingUsage(AudioAttributes.USAGE_GAME)
                .addMatchingUsage(AudioAttributes.USAGE_UNKNOWN)
                .build();
        AudioFormat pcm = new AudioFormat.Builder()
                .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
                .setSampleRate(SAMPLE_RATE)
                .setChannelMask(AudioFormat.CHANNEL_IN_STEREO)
                .build();
        int minBuffer = AudioRecord.getMinBufferSize(SAMPLE_RATE, AudioFormat.CHANNEL_IN_STEREO,
                AudioFormat.ENCODING_PCM_16BIT);

        AudioRecord record = null;
        MediaCodec codec = null;
        try {
            record = new AudioRecord.Builder()
                    .setAudioPlaybackCaptureConfig(config)
                    .setAudioFormat(pcm)
                    .setBufferSizeInBytes(Math.max(minBuffer, 4 * CHUNK_BYTES))
                    .build();
            codec = MediaCodec.createByCodecName(encoder);
            codec.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
            codec.start();
            record.startRecording();
            AudioCapture capture = new AudioCapture(record, codec);
            capture.mThread.start();
            Log.i(TAG, "capturing sound with " + encoder);
            return capture;
        } catch (Exception e) {
            Log.e(TAG, "can't capture sound", e);
            if (record != null)
                record.release();
            if (codec != null)
                codec.release();
            return null;
        }
    }

    void stop() {
        mRunning = false;
        try {
            mThread.join(500);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
    }

    private void run() {
        byte[] chunk = new byte[CHUNK_BYTES];
        MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
        long framesRead = 0;
        try {
            while (mRunning) {
                int read = mRecord.read(chunk, 0, chunk.length);
                if (read <= 0)
                    continue;
                int index = mCodec.dequeueInputBuffer(DEQUEUE_TIMEOUT_US);
                if (index >= 0) {
                    ByteBuffer input = mCodec.getInputBuffer(index);
                    input.clear();
                    input.put(chunk, 0, read);
                    long ptsUs = framesRead * 1_000_000L / SAMPLE_RATE;
                    mCodec.queueInputBuffer(index, 0, read, ptsUs, 0);
                }
                framesRead += read / (CHANNELS * 2);
                drain(info);
            }
        } catch (IllegalStateException e) {
            Log.w(TAG, "sound capture ended", e);
        } finally {
            mRecord.stop();
            mRecord.release();
            try {
                mCodec.stop();
            } catch (IllegalStateException e) {
                // Already in error.
            }
            mCodec.release();
        }
    }

    private void drain(MediaCodec.BufferInfo info) {
        for (;;) {
            int index = mCodec.dequeueOutputBuffer(info, 0);
            if (index < 0)
                return;
            ByteBuffer output = mCodec.getOutputBuffer(index);
            // The codec-config buffer (OpusHead) isn't needed: the receiver
            // knows the format.
            if (output != null && info.size > 0 && (info.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) == 0)
                CaptureBridge.nativeAudio(output, info.offset, info.size, info.presentationTimeUs);
            mCodec.releaseOutputBuffer(index, false);
        }
    }
}
