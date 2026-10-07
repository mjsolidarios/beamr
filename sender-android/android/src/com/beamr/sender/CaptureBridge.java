package com.beamr.sender;

import android.app.Activity;
import android.app.StatusBarManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.graphics.drawable.Icon;
import android.net.Uri;
import android.os.Build;
import android.provider.Settings;
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
    // The Quick Settings prompt finished. True when the user saw it, or the tile was already added.
    static native void nativeTilePrompted(boolean answered);
    static native void nativeQuickCast();
    // "screen" or "app". The string is only valid during the call.
    static native void nativeShareTarget(String target);
    // One JPEG of the shared picture. The array is only valid during the call.
    static native void nativePreview(byte[] jpeg);

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
    // permission), then starts ScreenCaptureService. title and text are the
    // first casting notification, before the capture reports what it is.
    public static void requestCapture(Context context, boolean audio, int quality, String title, String text) {
        Intent intent = new Intent(context, ProjectionRequestActivity.class)
                .putExtra(ScreenCaptureService.EXTRA_AUDIO, audio)
                .putExtra(ScreenCaptureService.EXTRA_QUALITY, quality)
                .putExtra(ScreenCaptureService.EXTRA_TITLE, title)
                .putExtra(ScreenCaptureService.EXTRA_TEXT, text);
        if (!(context instanceof Activity))
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        context.startActivity(intent);
    }

    // Picture quality and sound for the capture that's already running.
    // False when nothing is casting.
    public static boolean applySettings(boolean audio, int quality) {
        ScreenCaptureService service = ScreenCaptureService.instance();
        if (service == null)
            return false;
        service.applySettings(audio, quality);
        return true;
    }

    // Starts sound on the running capture, after the permission was granted.
    public static void startCastAudio() {
        ScreenCaptureService service = ScreenCaptureService.instance();
        if (service != null)
            service.ensureAudio();
    }

    public static void updateNotification(String title, String text) {
        ScreenCaptureService service = ScreenCaptureService.instance();
        if (service != null)
            service.updateNotification(title, text);
    }

    public static void openAppSettings(Context context) {
        Intent intent = new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS,
                Uri.fromParts("package", context.getPackageName(), null));
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        context.startActivity(intent);
    }

    // 1 when Android 13+ will show the dialog (the result is reported later),
    // 0 when this Android has no such dialog, -1 when the call failed.
    public static int requestTile(Context context) {
        if (Build.VERSION.SDK_INT < 33)
            return 0;
        StatusBarManager manager = context.getSystemService(StatusBarManager.class);
        if (manager == null)
            return 0;
        int iconId = context.getResources().getIdentifier("ic_tile_cast", "drawable", context.getPackageName());
        if (iconId == 0)
            return -1;
        ComponentName component = new ComponentName(context, CastTileService.class);
        try {
            manager.requestAddTileService(component, "beamr", Icon.createWithResource(context, iconId),
                    context.getMainExecutor(), result -> nativeTilePrompted(
                            result == StatusBarManager.TILE_ADD_REQUEST_RESULT_TILE_NOT_ADDED
                                    || result == StatusBarManager.TILE_ADD_REQUEST_RESULT_TILE_ALREADY_ADDED
                                    || result == StatusBarManager.TILE_ADD_REQUEST_RESULT_TILE_ADDED));
        } catch (RuntimeException e) {
            return -1;
        }
        return 1;
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
