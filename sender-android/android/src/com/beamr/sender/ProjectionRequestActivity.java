package com.beamr.sender;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.media.projection.MediaProjectionManager;
import android.os.Build;
import android.os.Bundle;

// Asks for notification permission (so the Stop button can show), then for
// screen capture consent, and hands the grant to ScreenCaptureService.
public class ProjectionRequestActivity extends Activity {
    private static final int REQUEST_NOTIFICATIONS = 1;
    private static final int REQUEST_CAPTURE = 2;

    private boolean mAnswered;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // Recreated after a configuration change: the request is already up.
        if (savedInstanceState != null)
            return;

        if (Build.VERSION.SDK_INT >= 33
                && checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[] { Manifest.permission.POST_NOTIFICATIONS }, REQUEST_NOTIFICATIONS);
        } else {
            askForCapture();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] results) {
        // Casting works without the notification; ask for capture either way.
        if (requestCode == REQUEST_NOTIFICATIONS)
            askForCapture();
    }

    private void askForCapture() {
        MediaProjectionManager manager = getSystemService(MediaProjectionManager.class);
        startActivityForResult(manager.createScreenCaptureIntent(), REQUEST_CAPTURE);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode != REQUEST_CAPTURE)
            return;
        mAnswered = true;
        if (resultCode == RESULT_OK && data != null) {
            Intent service = new Intent(this, ScreenCaptureService.class)
                    .putExtra(ScreenCaptureService.EXTRA_RESULT_CODE, resultCode)
                    .putExtra(ScreenCaptureService.EXTRA_RESULT_DATA, data);
            startForegroundService(service);
        } else {
            CaptureBridge.nativeCaptureStopped(CaptureBridge.DENIED);
        }
        finish();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        // Dismissed before the consent dialog answered, e.g. by Back.
        if (isFinishing() && !mAnswered)
            CaptureBridge.nativeCaptureStopped(CaptureBridge.DENIED);
    }
}
