package com.beamr.sender;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.media.projection.MediaProjectionManager;
import android.os.Build;
import android.os.Bundle;

import java.util.ArrayList;
import java.util.List;

// Asks for the permissions casting uses (notifications, so the Stop button
// can show; recording audio, for sound), then for screen capture consent,
// and hands the grant to ScreenCaptureService.
public class ProjectionRequestActivity extends Activity {
    private static final int REQUEST_PERMISSIONS = 1;
    private static final int REQUEST_CAPTURE = 2;

    private boolean mAnswered;
    private boolean mAudio;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // Recreated after a configuration change: the request is already up.
        mAudio = getIntent().getBooleanExtra(ScreenCaptureService.EXTRA_AUDIO, false);
        if (savedInstanceState != null)
            return;

        List<String> missing = new ArrayList<>();
        if (Build.VERSION.SDK_INT >= 33
                && checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED)
            missing.add(Manifest.permission.POST_NOTIFICATIONS);
        if (mAudio && checkSelfPermission(Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED)
            missing.add(Manifest.permission.RECORD_AUDIO);

        if (missing.isEmpty())
            askForCapture();
        else
            requestPermissions(missing.toArray(new String[0]), REQUEST_PERMISSIONS);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] results) {
        // Casting works without either; the service checks what was granted.
        if (requestCode == REQUEST_PERMISSIONS)
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
                    .putExtra(ScreenCaptureService.EXTRA_RESULT_DATA, data)
                    .putExtra(ScreenCaptureService.EXTRA_AUDIO, mAudio)
                    .putExtra(ScreenCaptureService.EXTRA_QUALITY,
                            getIntent().getIntExtra(ScreenCaptureService.EXTRA_QUALITY,
                                    ScreenCaptureService.QUALITY_SMOOTH));
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
