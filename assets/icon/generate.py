#!/usr/bin/env python3
"""Generate every app/tray icon from the editable SVG marks in this folder.

    python3 -m pip install -r assets/icon/requirements.txt
    python3 assets/icon/generate.py

Only icon maintainers need these dependencies; generated assets are committed.
"""
from io import BytesIO
from pathlib import Path
import xml.etree.ElementTree as ET

import cairosvg
from PIL import Image

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent
BACKGROUND = "#0f1115"
SIZES = (16, 20, 24, 32, 40, 48, 64, 128, 256)
SVG_NS = "http://www.w3.org/2000/svg"
ET.register_namespace("", SVG_NS)


def mark_body(source, monochrome=False):
    root = ET.parse(HERE / source).getroot()
    if monochrome:
        for node in root.iter():
            for attr in ("fill", "stroke"):
                if node.get(attr, "none") != "none":
                    node.set(attr, "#ffffff")
    return "\n".join(ET.tostring(child, encoding="unicode").strip() for child in root
                     if child.tag != f"{{{SVG_NS}}}title")


def svg(body, size=1024):
    return (f'<svg xmlns="{SVG_NS}" width="{size}" height="{size}" '
            f'viewBox="0 0 {size} {size}">\n{body}\n</svg>\n')


def placed_mark(body, width=672, viewport=24):
    offset = (1024 - width) / 2
    return f'<g transform="translate({offset} {offset}) scale({width / viewport})">{body}</g>'


def render(source, size):
    # Supersample curves, but retain the final target's pixel-aligned geometry.
    png = cairosvg.svg2png(bytestring=source.encode(), output_width=size * 4,
                          output_height=size * 4)
    with Image.open(BytesIO(png)) as image:
        return image.convert("RGBA").resize((size, size), Image.Resampling.LANCZOS)


def main():
    body = mark_body("beamr-mark.svg")
    small_body = mark_body("beamr-mark-small.svg")
    tile = (f'<rect x="32" y="32" width="960" height="960" rx="224" '
            f'fill="{BACKGROUND}"/>')
    full = svg(tile + placed_mark(body))
    foreground = svg(placed_mark(body))
    (HERE / "beamr-icon.svg").write_text(full)
    (HERE / "beamr-foreground.svg").write_text(foreground)
    render(full, 1024).save(HERE / "beamr-icon.png")
    render(foreground, 1024).save(HERE / "beamr-foreground.png")

    desktop = ROOT / "receiver-desktop/platform"
    render(full, 256).save(desktop / "beamr.png")
    # Larger mark and pixel-tuned geometry in small taskbar/window icons.
    small = svg(tile + placed_mark(small_body, width=768, viewport=16))
    frames = {size: render(small if size <= 32 else full, size) for size in SIZES}
    frames[256].save(desktop / "beamr.ico", sizes=[(s, s) for s in SIZES],
                     append_images=[frames[s] for s in SIZES if s != 256])
    render(full, 1024).save(desktop / "beamr.icns", append_images=[
        render(small if size <= 32 else full, size)
        for size in (16, 32, 64, 128, 256, 512)])
    (desktop / "beamr-tray.svg").write_text(svg(mark_body("beamr-mark.svg", True), 24))
    (desktop / "beamr-tray-small.svg").write_text(
        svg(mark_body("beamr-mark-small.svg", True), 16))

    # Keep the About page's image in sync, including its rounded tile.
    render(full, 256).save(ROOT / "sender-android/qml/images/beamr-icon.png")

    res = ROOT / "sender-android/android/res"
    # Android applies the launcher mask: its legacy tile is full-bleed and
    # the adaptive layer has no baked-in background or rounded corners.
    legacy = svg(f'<rect width="1024" height="1024" fill="{BACKGROUND}"/>'
                 + placed_mark(body))
    # A 56 dp viewport on the 108 dp layer keeps all ink within the 66 dp
    # circular safe zone, including the screen's upper-right corner.
    adaptive = svg(placed_mark(body, width=1024 * 56 / 108))
    densities = {"mdpi": 1, "hdpi": 1.5, "xhdpi": 2, "xxhdpi": 3, "xxxhdpi": 4}
    for name, scale in densities.items():
        folder = res / f"mipmap-{name}"
        folder.mkdir(parents=True, exist_ok=True)
        render(legacy, round(48 * scale)).save(folder / "ic_launcher.png")
        render(adaptive, round(108 * scale)).save(folder / "ic_launcher_foreground.png")

    anydpi = res / "mipmap-anydpi-v26"
    anydpi.mkdir(parents=True, exist_ok=True)
    (anydpi / "ic_launcher.xml").write_text(
        '<?xml version="1.0" encoding="utf-8"?>\n'
        '<adaptive-icon xmlns:android="http://schemas.android.com/apk/res/android">\n'
        '    <background android:drawable="@color/ic_launcher_background" />\n'
        '    <foreground android:drawable="@mipmap/ic_launcher_foreground" />\n'
        '    <monochrome android:drawable="@mipmap/ic_launcher_foreground" />\n'
        '</adaptive-icon>\n')
    values = res / "values"
    values.mkdir(parents=True, exist_ok=True)
    (values / "ic_launcher_background.xml").write_text(
        '<?xml version="1.0" encoding="utf-8"?>\n<resources>\n'
        f'    <color name="ic_launcher_background">{BACKGROUND.upper()}</color>\n'
        '</resources>\n')
    print("Generated desktop, tray, Android launcher, and About icons.")


if __name__ == "__main__":
    main()
