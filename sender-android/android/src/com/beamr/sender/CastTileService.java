package com.beamr.sender;

import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.service.quicksettings.Tile;
import android.service.quicksettings.TileService;

// A Quick Settings tile: stops the cast when one is running, otherwise opens
// beamr and casts to the computer used last.
public class CastTileService extends TileService {

    // Asks the system to redraw the tile, e.g. when casting starts or stops.
    static void refresh(Context context) {
        try {
            TileService.requestListeningState(context, new ComponentName(context, CastTileService.class));
        } catch (RuntimeException e) {
            // Tile not added, or the system refused; nothing to update.
        }
    }

    @Override
    public void onStartListening() {
        update();
    }

    @Override
    public void onClick() {
        ScreenCaptureService service = ScreenCaptureService.instance();
        if (service != null) {
            service.stopCapture(CaptureBridge.STOPPED_BY_USER);
            update();
            return;
        }
        // Screen capture needs its consent dialog, which needs the app.
        Intent intent = new Intent(this, MainActivity.class)
                .setAction(MainActivity.ACTION_QUICK_CAST)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP);
        if (Build.VERSION.SDK_INT >= 34) {
            startActivityAndCollapse(PendingIntent.getActivity(this, 0, intent,
                    PendingIntent.FLAG_IMMUTABLE | PendingIntent.FLAG_UPDATE_CURRENT));
        } else {
            startActivityAndCollapseLegacy(intent);
        }
    }

    @SuppressWarnings("deprecation")
    private void startActivityAndCollapseLegacy(Intent intent) {
        startActivityAndCollapse(intent);
    }

    private void update() {
        Tile tile = getQsTile();
        if (tile == null)
            return;
        boolean casting = ScreenCaptureService.instance() != null;
        tile.setState(casting ? Tile.STATE_ACTIVE : Tile.STATE_INACTIVE);
        tile.setLabel("beamr");
        tile.setSubtitle(casting ? "Casting · tap to stop" : "Cast to last computer");
        tile.updateTile();
    }
}
