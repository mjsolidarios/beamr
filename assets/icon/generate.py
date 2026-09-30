#!/usr/bin/env python3
"""Makes every app icon size from the two 1024 px masters in this folder.

    python3 assets/icon/generate.py        (needs Pillow)

beamr-icon.png is the full square icon; beamr-foreground.png is the same
mark on transparency, for Android's adaptive icon.
"""
from pathlib import Path

from PIL import Image

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent
BACKGROUND = (0x0F, 0x11, 0x15, 255)

icon = Image.open(HERE / "beamr-icon.png").convert("RGBA")
foreground = Image.open(HERE / "beamr-foreground.png").convert("RGBA")

# Android: legacy square icons, and adaptive-icon foregrounds. An adaptive
# icon is 108 dp with only the middle 72 dp shown and a 66 dp circle safe
# under every launcher mask, so the mark is fitted into 56% of the canvas.
res = ROOT / "sender-android/android/res"
densities = {"mdpi": 1, "hdpi": 1.5, "xhdpi": 2, "xxhdpi": 3, "xxxhdpi": 4}
mark = foreground.crop(foreground.getbbox())
for name, scale in densities.items():
    folder = res / f"mipmap-{name}"
    folder.mkdir(parents=True, exist_ok=True)
    size = round(48 * scale)
    icon.resize((size, size), Image.LANCZOS).save(folder / "ic_launcher.png")

    canvas_size = round(108 * scale)
    canvas = Image.new("RGBA", (canvas_size, canvas_size), (0, 0, 0, 0))
    fit = 0.56 * canvas_size / max(mark.size)
    scaled = mark.resize((round(mark.width * fit), round(mark.height * fit)), Image.LANCZOS)
    canvas.alpha_composite(scaled, ((canvas_size - scaled.width) // 2, (canvas_size - scaled.height) // 2))
    canvas.save(folder / "ic_launcher_foreground.png")

anydpi = res / "mipmap-anydpi-v26"
anydpi.mkdir(parents=True, exist_ok=True)
(anydpi / "ic_launcher.xml").write_text(
    '<?xml version="1.0" encoding="utf-8"?>\n'
    '<adaptive-icon xmlns:android="http://schemas.android.com/apk/res/android">\n'
    '    <background android:drawable="@color/ic_launcher_background" />\n'
    '    <foreground android:drawable="@mipmap/ic_launcher_foreground" />\n'
    '    <monochrome android:drawable="@mipmap/ic_launcher_foreground" />\n'
    "</adaptive-icon>\n")
values = res / "values"
values.mkdir(parents=True, exist_ok=True)
(values / "ic_launcher_background.xml").write_text(
    '<?xml version="1.0" encoding="utf-8"?>\n'
    "<resources>\n"
    '    <color name="ic_launcher_background">#0F1115</color>\n'
    "</resources>\n")

# Desktop: the window icon, and the Windows executable's icon.
desktop = ROOT / "receiver-desktop/platform"
desktop.mkdir(parents=True, exist_ok=True)
icon.resize((256, 256), Image.LANCZOS).save(desktop / "beamr.png")
icon.save(desktop / "beamr.ico", sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
print("icons written")
