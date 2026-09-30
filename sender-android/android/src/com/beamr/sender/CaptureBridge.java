package com.beamr.sender;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.view.HapticFeedbackConstants;
import android.view.View;

import java.nio.ByteBuffer;

// The seam between SenderController (C++) and the Android capture pipeline.
// C++ calls the static methods; the service reports back through the natives,
// which SenderController registers at startup.
public final class CaptureBridge {
    // Reasons passed to nativeCaptureStopped().
    public static final String STOPPED_BY_USER = "";
    public static final String DENIED = "denied";
    // Sound states passed to nativeAudioState().
    public static final String AUDIO_ON = "on";
    public static final String AUDIO_OFF = "off";
    public static final String AUDIO_DENIED = "denied";
    public static final String AUDIO_UNAVAILABLE = "unavailable";

    private CaptureBridge() {}

    static native void nativeCaptureStarted(int width, int height);
    static native void nativeCaptureStopped(String error);
    // Called on the encoder thread; the buffer is only valid during the call.
    static native void nativeFrame(ByteBuffer buffer, int offset, int size, long ptsUs, int flags);
    // One Opus packet; called on the audio thread, same rules as nativeFrame.
    static native void nativeAudio(ByteBuffer buffer, int offset, int size, long ptsUs);
    static native void nativeAudioState(String state);
    static native void nativeQuickCast();

    // The tile asked to cast. C++ may not be running yet (a cold start), so
    // it's kept until SenderController asks for it.
    private static volatile boolean sQuickCastPending;
    private static volatile boolean sNativesReady;

    static void quickCastRequested() {
        if (sNativesReady)
            nativeQuickCast();
        else
            sQuickCastPending = true;
    }

    // Called by SenderController once its natives are registered: true if the
    // tile asked before then.
    public static boolean takePendingQuickCast() {
        sNativesReady = true;
        boolean pending = sQuickCastPending;
        sQuickCastPending = false;
        return pending;
    }

    // Shows the system consent dialog (and, for sound, the record-audio
    // permission), then starts ScreenCaptureService.
    public static void requestCapture(Context context, boolean audio, int quality) {
        Intent intent = new Intent(context, ProjectionRequestActivity.class)
                .putExtra(ScreenCaptureService.EXTRA_AUDIO, audio)
                .putExtra(ScreenCaptureService.EXTRA_QUALITY, quality);
        if (!(context instanceof Activity))
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        context.startActivity(intent);
    }

    public static void stopCapture() {
        ScreenCaptureService service = ScreenCaptureService.instance();
        if (service != null)
            service.stopCapture(STOPPED_BY_USER);
    }

    // A short tick for an outcome: a receiver allowed us, or something failed.
    public static void haptic(Context context, boolean success) {
        if (!(context instanceof Activity))
            return;
        Activity activity = (Activity) context;
        activity.runOnUiThread(() -> {
            View view = activity.getWindow().getDecorView();
            int effect;
            if (Build.VERSION.SDK_INT >= 30)
                effect = success ? HapticFeedbackConstants.CONFIRM : HapticFeedbackConstants.REJECT;
            else
                effect = success ? HapticFeedbackConstants.CONTEXT_CLICK : HapticFeedbackConstants.LONG_PRESS;
            view.performHapticFeedback(effect);
        });
    }

    public static void requestKeyFrame() {
        ScreenCaptureService service = ScreenCaptureService.instance();
        if (service != null)
            service.requestKeyFrame();
    }
}
