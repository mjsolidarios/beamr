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

## micro-ecc

The desktop app builds its TLS certificate with [micro-ecc](https://github.com/kmackay/micro-ecc)
v1.1, by Kenneth MacKay, BSD 2-clause license. The sources are in
[third_party/micro-ecc](third_party/micro-ecc). The license text:

Copyright (c) 2014, Kenneth MacKay
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

## Lucide icons

The apps' line icons come from [Lucide](https://lucide.dev), ISC License. See
[licenses/Lucide-ISC.txt](licenses/Lucide-ISC.txt).

## Google code scanner (Android)

The phone app scans QR codes with Google's code scanner
(`com.google.android.gms:play-services-code-scanner`), which runs in Google Play
services under the [Google APIs Terms of Service](https://developers.google.com/terms).

## AndroidX

The phone app ships [AndroidX Core](https://developer.android.com/jetpack/androidx)
(`androidx.core:core`), including `FileProvider`, under the **Apache License 2.0**.
The text is in [licenses/Apache-2.0.txt](licenses/Apache-2.0.txt).

## Kotlin coroutines

AndroidX Core brings in [Kotlin coroutines](https://github.com/Kotlin/kotlinx.coroutines)
(`kotlinx-coroutines-core`). They are Apache License 2.0, the same text as AndroidX.

## libc++

The phone app ships the NDK's `libc++_shared.so` under the **Apache License 2.0
with LLVM Exceptions**. The text is in [licenses/LLVM-libc++.txt](licenses/LLVM-libc++.txt).

## OpenSSL

The phone app ships [OpenSSL](https://www.openssl.org) 3.1.8 (`libssl` and
`libcrypto`) so Qt can encrypt the cast. The binaries are the Android builds
from [KDAB/android_openssl](https://github.com/KDAB/android_openssl) commit
`b71f147`. OpenSSL is Apache License 2.0. The text is in
[licenses/Apache-2.0.txt](licenses/Apache-2.0.txt).

## App icon

The beamr app icon was generated with OpenAI's Codex image generation for this
project and is released with it under the MIT license.
