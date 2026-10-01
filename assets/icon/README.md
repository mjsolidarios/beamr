# beamr icons

Two clean device silhouettes identify phone-to-screen casting. The cyan screen
and white phone sit on beamr's ink background (`#0f1115`). The tray uses the same
silhouette in one color, without a tile, text, or fine detail.

## Edit and regenerate

- `beamr-mark.svg`: editable 24-unit master, with a 2.5-unit stroke.
- `beamr-mark-small.svg`: editable 16-unit optical variant. Its thicker,
  pixel-aligned shapes keep the phone opening and the device gap visible.
- `beamr-icon.svg` and `beamr-foreground.svg`: generated full-size artwork.

Install the generation dependencies in a Python environment, then run from the
repository root:

```sh
python3 -m pip install -r assets/icon/requirements.txt
python3 assets/icon/generate.py
```

CairoSVG also requires the Cairo system library. These are development tools
only; application builds consume the committed assets.

The generator updates the PNG masters, desktop PNG/ICO/ICNS, both tray SVGs,
Android launcher images at all five densities, and the Android About image.
Edit the two mark masters and regenerate rather than editing exported images.

## Small sizes and platform masks

The tray renders the optical variant at 16 and 20 pixels and the regular mark
at 22, 24, 32, 40, 48, and 64 pixels. Qt tints it for light/dark appearance and
marks it as a template for macOS. Windows ICO frames include 20 and 40 pixels
for fractional display scaling; the window icon loads that multi-size asset.

Desktop tiles have rounded transparent corners. Android legacy icons use a
full-bleed square; adaptive icons use a transparent foreground on a separate
ink background. Their artwork stays within the central 66 dp safe circle of
the 108 dp layer. The same foreground alpha supplies themed monochrome icons.
