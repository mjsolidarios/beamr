# beamr

Low-latency screen casting from an Android phone to computers on the same Wi-Fi.

- **sender-android**: the phone app. It captures the screen with MediaProjection,
  encodes H.264 in hardware and streams it to up to 4 computers at once. It can
  also cast what apps play, as Opus audio.
- **receiver-desktop**: the computer app. It decodes and shows up to 4 phones
  side by side, each on its own screen. It plays one phone's sound at a time,
  and can also pause, record and take screenshots.
- **common**: the wire protocol and connect-link format both apps share.

Both apps are Qt 6 / QML.

## How it works

1. Open beamr on the computer. Each free screen shows a QR code.
2. On the phone, tap **Scan QR code** and point it at the screen you want. You
   can also pick a recent computer or type its address (port 47700 by default).
3. The computer asks you to allow the phone (or you can trust it once), then the
   phone asks for screen-capture consent and the cast starts.

Use **Add screen** on the computer to let another phone cast next to the first.
Double-click a screen, or use its focus button, to show only that one.

**Sound:** turn on **Cast sound** on the phone before casting. Android then asks
for permission to record audio. beamr only captures what apps play, never the
microphone, and calls and apps that block capture stay silent. On the computer,
only one phone's sound plays at a time: the first to cast. Use a screen's
speaker button, or press M, to switch the sound to that phone or mute it.

The QR code carries a link like:

```
beamr://connect?name=Office%20PC&port=47700&screen=k3f9x2&host=192.168.1.20&host=10.0.0.5
```

The phone picks the host on its own network, and `screen` sends the cast to the
screen whose code was scanned. See [`common/include/beamr/protocol.h`](common/include/beamr/protocol.h)
for the control protocol (newline-delimited JSON over TCP) and the video stream
framing.

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

Useful environment variables for the receiver:

- `BEAMR_PORT=47710` listens on another port, for example to run a second
  receiver on one computer.
- `BEAMR_DEMO=1` enables Ctrl+Shift+D, which simulates a phone asking to cast.
  Debug builds have it on already.

## Third-party code

- [`third_party/ffmpeg`](third_party/ffmpeg): FFmpeg public headers matching the
  FFmpeg that ships with Qt Multimedia (LGPL 2.1)
- [`third_party/qrcodegen`](third_party/qrcodegen): Project Nayuki's QR Code
  generator (MIT)
- Icons from [Lucide](https://lucide.dev) (ISC)

## License

MIT. See [LICENSE](LICENSE).
