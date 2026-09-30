package com.beamr.sender;

import android.Manifest;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ServiceInfo;
import android.content.res.Configuration;
import android.hardware.display.DisplayManager;
import android.hardware.display.VirtualDisplay;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.Surface;

import java.nio.ByteBuffer;

// Mirrors the screen into a hardware H.264 encoder and hands every encoded
// buffer to C++. Runs as a foreground service because Android only allows
// MediaProjection from one.
public class ScreenCaptureService extends Service {
    static final String EXTRA_RESULT_CODE = "com.beamr.sender.RESULT_CODE";
    static final String EXTRA_RESULT_DATA = "com.beamr.sender.RESULT_DATA";
    // Whether to cast sound too.
    static final String EXTRA_AUDIO = "com.beamr.sender.AUDIO";
    // One of the QUALITY_* presets.
    static final String EXTRA_QUALITY = "com.beamr.sender.QUALITY";
    // Keep in step with SenderController::Quality.
    static final int QUALITY_SMOOTH = 0;
    static final int QUALITY_BALANCED = 1;
    static final int QUALITY_DATA_SAVER = 2;
    private static final String ACTION_STOP = "com.beamr.sender.STOP";

    private static final String TAG = "beamr";
    private static final String MIME = MediaFormat.MIMETYPE_VIDEO_AVC;
    private static final String CHANNEL_ID = "casting";
    private static final int NOTIFICATION_ID = 1;

    private static final int KEYFRAME_INTERVAL_SEC = 2;
    // A still screen produces no frames; repeat the last one so a receiver
    // that joins, or drops a frame, recovers quickly.
    private static final long REPEAT_FRAME_AFTER_US = 100_000;

    private static volatile ScreenCaptureService sInstance;

    private final Object mLock = new Object();
    private HandlerThread mThread;
    private Handler mHandler;
    private MediaProjection mProjection;
    private VirtualDisplay mDisplay;
    private MediaCodec mCodec;
    private Surface mSurface;
    private volatile AudioCapture mAudio;
    private int mWidth;
    private int mHeight;
    private boolean mStopped;

    // The preset's limits: Smooth 1080p60 at 8 Mbit/s, Balanced 1080p30 at
    // 5 Mbit/s, Data saver 720p30 at 2.5 Mbit/s. Smooth matches beamr/config.h.
    private int mMaxLongSide = 1920;
    private int mMaxShortSide = 1080;
    private int mFrameRate = 60;
    private int mBitrateBps = 8_000_000;
    // Android 14+ reports what's captured (the screen or a single app) and
    // its size; before that, it's always the whole screen.
    private boolean mContentSizeReported;

    static ScreenCaptureService instance() {
        return sInstance;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (intent == null || ACTION_STOP.equals(intent.getAction())) {
            stopCapture(CaptureBridge.STOPPED_BY_USER);
            return START_NOT_STICKY;
        }

        // Android 14+ requires the foreground service before the projection.
        startForeground(NOTIFICATION_ID, buildNotification(), ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PROJECTION);

        mThread = new HandlerThread("beamr-encoder");
        mThread.start();
        mHandler = new Handler(mThread.getLooper());

        int resultCode = intent.getIntExtra(EXTRA_RESULT_CODE, 0);
        Intent data = resultData(intent);
        boolean audio = intent.getBooleanExtra(EXTRA_AUDIO, false);
        applyQuality(intent.getIntExtra(EXTRA_QUALITY, QUALITY_SMOOTH));
        mHandler.post(() -> start(resultCode, data, audio));
        return START_NOT_STICKY;
    }

    private void applyQuality(int quality) {
        switch (quality) {
        case QUALITY_BALANCED:
            mFrameRate = 30;
            mBitrateBps = 5_000_000;
            break;
        case QUALITY_DATA_SAVER:
            mMaxLongSide = 1280;
            mMaxShortSide = 720;
            mFrameRate = 30;
            mBitrateBps = 2_500_000;
            break;
        default:
            break;
        }
    }

    @SuppressWarnings("deprecation")
    private static Intent resultData(Intent intent) {
        if (Build.VERSION.SDK_INT >= 33)
            return intent.getParcelableExtra(EXTRA_RESULT_DATA, Intent.class);
        return intent.getParcelableExtra(EXTRA_RESULT_DATA);
    }

    private void start(int resultCode, Intent data, boolean audio) {
        try {
            MediaProjectionManager manager = getSystemService(MediaProjectionManager.class);
            mProjection = manager.getMediaProjection(resultCode, data);
            if (mProjection == null)
                throw new IllegalStateException("no media projection");
            // Fires when the user stops casting from the status bar chip.
            mProjection.registerCallback(new MediaProjection.Callback() {
                @Override
                public void onStop() {
                    stopCapture(CaptureBridge.STOPPED_BY_USER);
                }

                // Android 14+: the size of what's shared, whether the whole
                // screen or a single app, now and whenever it changes (a
                // rotation, or the app resizing). Fit the video to it.
                @Override
                public void onCapturedContentResize(int width, int height) {
                    mContentSizeReported = true;
                    if (width > 0 && height > 0)
                        resizeTo(width, height);
                }
            }, mHandler);

            DisplayMetrics metrics = displayMetrics();
            startEncoder(metrics.widthPixels, metrics.heightPixels);
            mDisplay = mProjection.createVirtualDisplay("beamr", mWidth, mHeight, metrics.densityDpi,
                    DisplayManager.VIRTUAL_DISPLAY_FLAG_AUTO_MIRROR, mSurface, null, mHandler);
            sInstance = this;
            Log.i(TAG, "capturing " + mWidth + "x" + mHeight);
            CaptureBridge.nativeCaptureStarted(mWidth, mHeight);
            startAudio(audio);
            CastTileService.refresh(this);
        } catch (Exception e) {
            Log.e(TAG, "can't start capture", e);
            stopCapture(e.getMessage() != null ? e.getMessage() : e.toString());
        }
    }

    private void startAudio(boolean wanted) {
        if (!wanted) {
            CaptureBridge.nativeAudioState(CaptureBridge.AUDIO_OFF);
            return;
        }
        if (checkSelfPermission(Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) {
            CaptureBridge.nativeAudioState(CaptureBridge.AUDIO_DENIED);
            return;
        }
        mAudio = AudioCapture.start(mProjection);
        CaptureBridge.nativeAudioState(mAudio != null ? CaptureBridge.AUDIO_ON : CaptureBridge.AUDIO_UNAVAILABLE);
    }

    // The virtual display keeps its size across rotation; re-create the
    // encoder at the new orientation and point the display at it. Android
    // 14+ reports this through onCapturedContentResize() instead.
    @Override
    public void onConfigurationChanged(Configuration configuration) {
        super.onConfigurationChanged(configuration);
        Handler handler = mHandler;
        if (handler != null && !mContentSizeReported) {
            handler.post(() -> {
                DisplayMetrics metrics = displayMetrics();
                resizeTo(metrics.widthPixels, metrics.heightPixels);
            });
        }
    }

    // Re-creates the encoder for content of this size and points the virtual
    // display at it; nothing to do if the video size doesn't change.
    private void resizeTo(int contentWidth, int contentHeight) {
        if (mDisplay == null || mStopped)
            return;
        int[] size = encodedSize(contentWidth, contentHeight, null);
        if (size[0] == mWidth && size[1] == mHeight)
            return;
        try {
            MediaCodec oldCodec;
            synchronized (mLock) {
                oldCodec = mCodec;
            }
            Surface oldSurface = mSurface;
            startEncoder(contentWidth, contentHeight);
            mDisplay.resize(mWidth, mHeight, displayMetrics().densityDpi);
            mDisplay.setSurface(mSurface);
            release(oldCodec);
            oldSurface.release();
            Log.i(TAG, "resized to " + mWidth + "x" + mHeight);
            CaptureBridge.nativeCaptureStarted(mWidth, mHeight);
        } catch (Exception e) {
            Log.e(TAG, "can't resize capture", e);
            stopCapture(e.getMessage() != null ? e.getMessage() : e.toString());
        }
    }

    void requestKeyFrame() {
        synchronized (mLock) {
            if (mCodec == null)
                return;
            Bundle params = new Bundle();
            params.putInt(MediaCodec.PARAMETER_KEY_REQUEST_SYNC_FRAME, 0);
            try {
                mCodec.setParameters(params);
            } catch (IllegalStateException e) {
                // Stopping; nothing to refresh.
            }
        }
    }

    void stopCapture(String reason) {
        synchronized (mLock) {
            if (mStopped)
                return;
            mStopped = true;
        }
        sInstance = null;

        if (mAudio != null) {
            mAudio.stop();
            mAudio = null;
        }
        if (mDisplay != null)
            mDisplay.release();
        MediaCodec codec;
        synchronized (mLock) {
            codec = mCodec;
            mCodec = null;
        }
        release(codec);
        if (mSurface != null)
            mSurface.release();
        if (mProjection != null)
            mProjection.stop();
        if (mThread != null)
            mThread.quitSafely();

        stopForeground(STOP_FOREGROUND_REMOVE);
        stopSelf();
        Log.i(TAG, "capture stopped: " + (reason.isEmpty() ? "by user" : reason));
        CaptureBridge.nativeCaptureStopped(reason);
        CastTileService.refresh(this);
    }

    // Creates and starts an encoder sized for content of this size (the
    // screen, or a single app); it becomes mCodec and its input surface mSurface.
    private void startEncoder(int contentWidth, int contentHeight) throws Exception {
        MediaCodec codec = MediaCodec.createEncoderByType(MIME);
        int[] size = encodedSize(contentWidth, contentHeight,
                codec.getCodecInfo().getCapabilitiesForType(MIME).getVideoCapabilities());
        mWidth = size[0];
        mHeight = size[1];

        MediaFormat format = MediaFormat.createVideoFormat(MIME, mWidth, mHeight);
        format.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface);
        format.setInteger(MediaFormat.KEY_BIT_RATE, mBitrateBps);
        format.setInteger(MediaFormat.KEY_FRAME_RATE, mFrameRate);
        format.setFloat(MediaFormat.KEY_MAX_FPS_TO_ENCODER, mFrameRate);
        format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, KEYFRAME_INTERVAL_SEC);
        format.setLong(MediaFormat.KEY_REPEAT_PREVIOUS_FRAME_AFTER, REPEAT_FRAME_AFTER_US);
        // Every keyframe carries SPS/PPS, so the receiver can start anywhere.
        format.setInteger(MediaFormat.KEY_PREPEND_HEADER_TO_SYNC_FRAMES, 1);
        // B-frames add a frame of latency.
        format.setInteger(MediaFormat.KEY_MAX_B_FRAMES, 0);
        format.setInteger(MediaFormat.KEY_PRIORITY, 0);
        if (Build.VERSION.SDK_INT >= 30)
            format.setInteger(MediaFormat.KEY_LATENCY, 1);

        codec.setCallback(new MediaCodec.Callback() {
            @Override
            public void onInputBufferAvailable(MediaCodec mc, int index) {
                // Input comes from the surface.
            }

            @Override
            public void onOutputBufferAvailable(MediaCodec mc, int index, MediaCodec.BufferInfo info) {
                try {
                    ByteBuffer buffer = mc.getOutputBuffer(index);
                    if (buffer != null && info.size > 0 && isCurrent(mc))
                        CaptureBridge.nativeFrame(buffer, info.offset, info.size, info.presentationTimeUs, info.flags);
                    mc.releaseOutputBuffer(index, false);
                } catch (IllegalStateException e) {
                    // Released while this callback was queued.
                }
            }

            @Override
            public void onError(MediaCodec mc, MediaCodec.CodecException e) {
                Log.e(TAG, "encoder error", e);
                if (isCurrent(mc))
                    stopCapture("encoder error: " + e.getDiagnosticInfo());
            }

            @Override
            public void onOutputFormatChanged(MediaCodec mc, MediaFormat format) {
            }
        }, mHandler);
        codec.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
        Surface surface = codec.createInputSurface();
        synchronized (mLock) {
            mCodec = codec;
        }
        mSurface = surface;
        codec.start();
    }

    private boolean isCurrent(MediaCodec codec) {
        synchronized (mLock) {
            return codec == mCodec;
        }
    }

    private static void release(MediaCodec codec) {
        if (codec == null)
            return;
        try {
            codec.stop();
        } catch (IllegalStateException e) {
            // Never started, or already in error.
        }
        codec.release();
    }

    // Fits the content into the preset's size (either orientation) in steps
    // the encoder accepts, keeping the aspect ratio.
    private int[] encodedSize(int screenWidth, int screenHeight, MediaCodecInfo.VideoCapabilities caps) {
        int longSide = Math.max(screenWidth, screenHeight);
        int shortSide = Math.min(screenWidth, screenHeight);
        double scale = Math.min(1.0, Math.min((double) mMaxLongSide / longSide, (double) mMaxShortSide / shortSide));
        for (int attempt = 0; attempt < 12; ++attempt) {
            int width = align16(screenWidth * scale);
            int height = align16(screenHeight * scale);
            if (caps == null || caps.isSizeSupported(width, height))
                return new int[] { width, height };
            scale *= 0.85;
        }
        return new int[] { align16(screenWidth * scale), align16(screenHeight * scale) };
    }

    private static int align16(double value) {
        return Math.max(16, ((int) value) / 16 * 16);
    }

    @SuppressWarnings("deprecation")
    private DisplayMetrics displayMetrics() {
        Display display = getSystemService(DisplayManager.class).getDisplay(Display.DEFAULT_DISPLAY);
        DisplayMetrics metrics = new DisplayMetrics();
        display.getRealMetrics(metrics);
        return metrics;
    }

    private Notification buildNotification() {
        NotificationManager manager = getSystemService(NotificationManager.class);
        manager.createNotificationChannel(
                new NotificationChannel(CHANNEL_ID, "Casting", NotificationManager.IMPORTANCE_LOW));

        Intent open = getPackageManager().getLaunchIntentForPackage(getPackageName());
        PendingIntent openIntent = PendingIntent.getActivity(this, 0, open, PendingIntent.FLAG_IMMUTABLE);
        PendingIntent stopIntent = PendingIntent.getService(this, 1,
                new Intent(this, ScreenCaptureService.class).setAction(ACTION_STOP), PendingIntent.FLAG_IMMUTABLE);

        return new Notification.Builder(this, CHANNEL_ID)
                .setSmallIcon(android.R.drawable.ic_menu_share)
                .setContentTitle("Casting your screen")
                .setContentText("beamr is sharing this screen with your computer.")
                .setContentIntent(openIntent)
                .setOngoing(true)
                .addAction(new Notification.Action.Builder(null, "Stop", stopIntent).build())
                .build();
    }
}
