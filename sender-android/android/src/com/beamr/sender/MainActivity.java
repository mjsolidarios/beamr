package com.beamr.sender;

import android.content.Intent;
import android.os.Bundle;

import org.qtproject.qt.android.bindings.QtActivity;

// Qt's activity, plus picking up requests from the Quick Settings tile.
public class MainActivity extends QtActivity {
    static final String ACTION_QUICK_CAST = "com.beamr.sender.QUICK_CAST";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (savedInstanceState == null)
            handle(getIntent());
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        handle(intent);
    }

    private static void handle(Intent intent) {
        if (intent != null && ACTION_QUICK_CAST.equals(intent.getAction()))
            CaptureBridge.quickCastRequested();
    }
}
