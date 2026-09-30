Public headers of FFmpeg 7.1.1 (libavcodec 61.19.101, libavformat 61.7.100,
libavutil 59.39.100),
copied unmodified from https://ffmpeg.org/releases/ffmpeg-7.1.1.tar.xz except
for `libavutil/avconfig.h`, which FFmpeg's configure normally generates.

The receiver links against the FFmpeg shared libraries that ship with Qt
Multimedia, so these headers must match that build. When Qt is upgraded,
compare `strings <qt>/lib/libavcodec.so.* ` against `libavcodec/version.h`
and replace the headers if they differ. FFmpeg is LGPL 2.1; see COPYING.LGPLv2.1.
