# beamr

Low-latency screen casting from an Android phone to computers on the same Wi-Fi.

- **sender-android**: the phone app. It captures the screen (or a single
  app) with MediaProjection, encodes H.264 in hardware and streams it, with
  what apps play as Opus sound, to up to 4 computers at once.
- **receiver-desktop**: the computer app, for Linux, Windows and macOS. It
  shows up to 4 phones side by side, plays one phone's sound at a time, and
  can pause, record to MP4, take screenshots and pop a phone out into its own
  window.
- **common**: the wire protocol and connect-link format both apps share.

Both apps are Qt 6 / QML.

## How it works

1. Open beamr on the computer. Each free screen shows a QR code.
2. On the phone, tap the computer under **Nearby** (beamr finds computers
   running it on the same Wi-Fi), or tap **Scan QR code** and point it at the
   screen you want. A recent computer or a typed address (port 47700 by
   default) works too.
3. Scanning a screen's code lets you straight in. Otherwise the computer asks
   you to allow the phone (or you can trust it once). The phone then asks for
   screen-capture consent: share the entire screen or just one app.

If the Wi-Fi drops for a moment, the phone reconnects on its own within 30
seconds and the cast carries on in the same screen, recording included.

Use **Add screen** on the computer to let another phone cast next to the first.
Double-click a screen, or use its focus button, to show only that one; its
pop-out button moves it into its own window, for example for a projector.

**Sound:** turn on **Cast sound** on the phone before casting. Android then asks
for permission to record audio. beamr only captures what apps play, never the
microphone, and calls and apps that block capture stay silent. On the computer,
only one phone's sound plays at a time: the first to cast. Use a screen's
speaker button, or press M, to switch the sound to that phone or mute it; its
slider sets the volume, and Settings picks the speakers.

**On the phone:** Settings has picture quality (Smooth 1080p60, Balanced
1080p30, Data saver 720p30), sound, and light or dark appearance. Add the beamr
tile to Quick Settings to cast to your last computer, or stop, from anywhere.

**On the computer:** closing the window keeps beamr in the tray so phones can
still connect, and it can start when you sign in (both in Settings).

The QR code carries a link like:

```
beamr://connect?name=Office%20PC&port=47700&screen=k3f9x2&pair=…&host=192.168.1.20&host=10.0.0.5
```

The phone picks the host on its own network, `screen` sends the cast to the
screen whose code was scanned, and `pair` is a one-time code that skips the
approval prompt. See [`common/include/beamr/protocol.h`](common/include/beamr/protocol.h)
for the control protocol (newline-delimited JSON over TCP), discovery, and the
media stream framing.

## Downloads

Every push builds, in [GitHub Actions](https://github.com/mjsolidarios/beamr/actions):

- Linux: an AppImage
- Windows: an installer (per user, no administrator rights) and a zip
- macOS (Apple silicon): a disk image. It isn't signed yet, so the first time,
  right-click the app and choose **Open**.
- Android: an APK. Test builds are signed with a throwaway key, so installing
  a newer one means uninstalling the old one first.

Signed releases go on the [Releases](https://github.com/mjsolidarios/beamr/releases)
page (see [Releasing](#releasing)).

## Building

Requirements:

- Qt 6.8+ (developed with 6.11), with **Qt Multimedia** (its FFmpeg backend
  decodes H.264 on the receiver) and **Qt Svg**
- CMake 3.21+ and Ninja
- For the phone app: Android SDK and NDK, JDK 17, and a Qt for Android kit.
  It targets Android 10 (API 29) or later.

Android kits build the sender and desktop kits build the receiver, so opening
the top-level `CMakeLists.txt` in Qt Creator works for both.

Receiver (desktop):

```sh
<qt>/gcc_64/bin/qt-cmake -S . -B build-desktop -G Ninja
cmake --build build-desktop
./build-desktop/receiver-desktop/beamr-receiver
```

Sender (Android):

```sh
<qt>/android_arm64_v8a/bin/qt-cmake -S . -B build-android -G Ninja \
    -DQT_HOST_PATH=<qt>/gcc_64 \
    -DANDROID_SDK_ROOT=<android-sdk> -DANDROID_NDK_ROOT=<android-sdk>/ndk/<version>
cmake --build build-android --target apk
adb install -r build-android/sender-android/android-build/build/outputs/apk/debug/android-build-debug.apk
```

QR scanning on the phone uses Google's code scanner (Play services). On phones
without Play services, enter the address instead.

macOS (receiver):

```sh
<qt>/macos/bin/qt-cmake -S . -B build-mac -G Ninja
cmake --build build-mac
open build-mac/receiver-desktop/beamr-receiver.app
```

Useful environment variables for the receiver:

- `BEAMR_PORT=47710` listens on another port, for example to run a second
  receiver on one computer.
- `BEAMR_DEMO=1` enables Ctrl+Shift+D, which simulates a phone asking to cast.
  Debug builds have it on already.

## Translating

The apps are ready for other languages. Configure with the languages you want
and generate the files translators fill in:

```sh
qt-cmake -S . -B build -G Ninja -DBEAMR_LANGUAGES="fil;es"
cmake --build build --target update_translations
```

That writes `sender-android/i18n/*.ts` and `receiver-desktop/i18n/*.ts`. Open
them in Qt Linguist, translate, and commit them; builds with the same
`BEAMR_LANGUAGES` include them, and each app picks the one matching the
phone's or computer's language.

## Releasing

1. Once: make the Android release key with
   `scripts/make-android-release-key.sh` and add the secrets it lists to the
   repository. Keep the key and its password somewhere safe: every future
   update must be signed with it.
2. Bump `VERSION` in `CMakeLists.txt` (and `QT_ANDROID_VERSION_CODE` in
   `sender-android/CMakeLists.txt`), then tag: `git tag v0.2.0 && git push origin v0.2.0`.
3. The workflow builds everything and publishes a release with the AppImage,
   Windows installer and zip, macOS disk image and signed APK attached.

## Built with Qt

beamr is built with [Qt 6](https://www.qt.io) and developed in
[Qt Creator](https://www.qt.io/product/development-tools). Qt is used under
its open-source license, the **GNU LGPL v3**. Qt is linked dynamically and
unmodified, both apps say so in their Settings, and the license texts are in
[licenses/](licenses). See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)
for details and where to get Qt's source.

## Third-party code

- [`third_party/ffmpeg`](third_party/ffmpeg): FFmpeg public headers matching the
  FFmpeg that ships with Qt Multimedia (LGPL 2.1)
- [`third_party/qrcodegen`](third_party/qrcodegen): Project Nayuki's QR Code
  generator (MIT)
- Icons from [Lucide](https://lucide.dev) (ISC)
- The app icon's masters are in [`assets/icon`](assets/icon); `generate.py`
  there makes every Android and desktop size from them

## License

MIT. See [LICENSE](LICENSE).
