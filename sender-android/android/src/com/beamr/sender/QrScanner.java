package com.beamr.sender;

import android.app.Activity;
import android.content.Context;

import com.google.mlkit.common.MlKitException;
import com.google.mlkit.vision.barcode.common.Barcode;
import com.google.mlkit.vision.codescanner.GmsBarcodeScannerOptions;
import com.google.mlkit.vision.codescanner.GmsBarcodeScanning;

// Scans a receiver's QR code with Google's scanner UI, which runs in Play
// services and needs no camera permission. Results come back through the
// natives, which SenderController registers at startup.
public final class QrScanner {
    // Reasons passed to nativeScanFailed().
    public static final String CANCELED = "";
    public static final String UNAVAILABLE = "unavailable";

    private QrScanner() {}

    static native void nativeScanned(String text);
    static native void nativeScanFailed(String reason);

    public static void scan(Context context) {
        if (!(context instanceof Activity)) {
            nativeScanFailed(UNAVAILABLE);
            return;
        }
        Activity activity = (Activity) context;
        activity.runOnUiThread(() -> {
            GmsBarcodeScannerOptions options = new GmsBarcodeScannerOptions.Builder()
                    .setBarcodeFormats(Barcode.FORMAT_QR_CODE)
                    .enableAutoZoom()
                    .build();
            try {
                GmsBarcodeScanning.getClient(activity, options)
                        .startScan()
                        .addOnSuccessListener(barcode -> {
                            String text = barcode.getRawValue();
                            nativeScanned(text != null ? text : "");
                        })
                        .addOnCanceledListener(() -> nativeScanFailed(CANCELED))
                        .addOnFailureListener(e -> {
                            boolean canceled = e instanceof MlKitException
                                    && ((MlKitException) e).getErrorCode() == MlKitException.CODE_SCANNER_CANCELLED;
                            nativeScanFailed(canceled ? CANCELED : UNAVAILABLE);
                        });
            } catch (RuntimeException e) {
                // No Play services on this phone.
                nativeScanFailed(UNAVAILABLE);
            }
        });
    }
}
