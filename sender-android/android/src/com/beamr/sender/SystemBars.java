package com.beamr.sender;

import android.app.Activity;
import android.content.Context;
import android.view.Window;

import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsControllerCompat;

// Keeps the status and navigation bar icons readable over the app's theme.
// Qt sets them when the app picks a theme but not when it goes back to
// following the system.
public final class SystemBars {
    private SystemBars() {}

    public static void setDarkContent(Context context, boolean darkContent) {
        if (!(context instanceof Activity))
            return;
        Activity activity = (Activity) context;
        activity.runOnUiThread(() -> {
            Window window = activity.getWindow();
            if (window == null)
                return;
            WindowInsetsControllerCompat controller =
                    WindowCompat.getInsetsController(window, window.getDecorView());
            controller.setAppearanceLightStatusBars(darkContent);
            controller.setAppearanceLightNavigationBars(darkContent);
        });
    }
}
