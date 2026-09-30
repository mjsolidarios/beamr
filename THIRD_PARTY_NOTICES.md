# Third-party notices

beamr itself is MIT licensed (see [LICENSE](LICENSE)). It is built on, and
ships with, the following open-source software.

## Qt

beamr's phone and desktop apps are built with the [Qt](https://www.qt.io)
framework (Qt 6: Core, Gui, Qml, Quick, Quick Controls, Network, Svg,
Multimedia, Concurrent) and developed in [Qt Creator](https://www.qt.io/product/development-tools).

Qt is used under the **GNU Lesser General Public License v3** (open-source
licensing). The full text is in [licenses/LGPL-3.0.txt](licenses/LGPL-3.0.txt),
and it builds on [licenses/GPL-3.0.txt](licenses/GPL-3.0.txt).

- Qt's libraries are linked dynamically and ship as separate files next to
  beamr (DLLs on Windows, shared libraries on Linux, `.so` files inside the
  Android APK). You can replace them with your own build of the same Qt
  version.
- Qt's source code is available at <https://code.qt.io> and
  <https://download.qt.io/official_releases/qt/>. beamr uses Qt unmodified.
- Qt is a registered trademark of The Qt Company Ltd. beamr isn't affiliated
  with or endorsed by The Qt Company.

Qt Creator is only a development tool. It doesn't ship with beamr and puts no
terms on it.

## FFmpeg

The desktop app decodes H.264 video and Opus sound with the FFmpeg libraries
that come with Qt Multimedia, linked dynamically, under the **GNU LGPL v2.1**.
Its public headers are vendored in [third_party/ffmpeg](third_party/ffmpeg),
unmodified; see [third_party/ffmpeg/COPYING.LGPLv2.1](third_party/ffmpeg/COPYING.LGPLv2.1).
Source: <https://ffmpeg.org/download.html>.

## QR Code generator library

[third_party/qrcodegen](third_party/qrcodegen), by Project Nayuki, MIT
License. The license text is at the top of each file.

## Lucide icons

The apps' line icons come from [Lucide](https://lucide.dev), ISC License. See
[licenses/Lucide-ISC.txt](licenses/Lucide-ISC.txt).

## Google code scanner (Android)

The phone app scans QR codes with Google's code scanner
(`com.google.android.gms:play-services-code-scanner`), which runs in Google Play
services under the [Google APIs Terms of Service](https://developers.google.com/terms).

## App icon

The beamr app icon was generated with OpenAI's Codex image generation for this
project and is released with it under the MIT license.
