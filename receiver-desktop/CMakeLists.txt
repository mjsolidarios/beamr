find_package(Qt6 REQUIRED COMPONENTS Concurrent Multimedia Network Svg Widgets)

# H.264 decoding and MP4 recording use the FFmpeg that ships with Qt Multimedia, so the
# receiver deploys with nothing extra. Qt doesn't ship its headers; the
# matching ones are vendored in third_party/ffmpeg.
set(BEAMR_FFMPEG_LIB_DIR "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_LIBS}" CACHE PATH
    "Directory with the FFmpeg libraries to decode video with")
foreach(_lib avcodec avformat avutil)
    find_library(BEAMR_${_lib}_LIBRARY NAMES ${_lib} PATHS "${BEAMR_FFMPEG_LIB_DIR}" NO_DEFAULT_PATH)
    if(NOT BEAMR_${_lib}_LIBRARY)
        message(FATAL_ERROR "lib${_lib} not found in ${BEAMR_FFMPEG_LIB_DIR}. Install Qt Multimedia's "
                            "FFmpeg backend, or point BEAMR_FFMPEG_LIB_DIR at an FFmpeg 7.1 build.")
    endif()
endforeach()
add_library(beamr_ffmpeg INTERFACE)
target_include_directories(beamr_ffmpeg SYSTEM INTERFACE "${PROJECT_SOURCE_DIR}/third_party/ffmpeg/include")
target_link_libraries(beamr_ffmpeg INTERFACE ${BEAMR_avcodec_LIBRARY} ${BEAMR_avformat_LIBRARY} ${BEAMR_avutil_LIBRARY})
if(UNIX AND NOT APPLE)
    # Qt's FFmpeg needs Qt's stub libraries (libQt6FFmpegStub-*) next to it.
    # The stubs resolve OpenSSL and VA-API at run time, so their symbols
    # aren't there to link against; decoding H.264 never calls them.
    target_link_options(beamr_ffmpeg INTERFACE
        "LINKER:-rpath-link,${BEAMR_FFMPEG_LIB_DIR}"
        "LINKER:--allow-shlib-undefined")
endif()

# QR code generation for the connect code (Nayuki's, MIT), vendored.
add_library(beamr_qrcodegen STATIC "${PROJECT_SOURCE_DIR}/third_party/qrcodegen/qrcodegen.cpp")
target_include_directories(beamr_qrcodegen SYSTEM PUBLIC "${PROJECT_SOURCE_DIR}/third_party/qrcodegen")

# The UI and its C++ backend live in a static QML module so tools and tests
# can load it without the executable.
qt_add_library(beamr_receiver_ui STATIC)

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(beamr_receiver_ui
    URI Beamr.Receiver
    VERSION 1.0
    SOURCES
        src/audioplayer.h src/audioplayer.cpp
        src/castscreen.h src/castscreen.cpp
        src/connectionrequestmodel.h src/connectionrequestmodel.cpp
        src/controlserver.h src/controlserver.cpp
        src/desktopintegration.h src/desktopintegration.cpp
        src/discoveryresponder.h src/discoveryresponder.cpp
        src/qrcodeitem.h src/qrcodeitem.cpp
        src/recorder.h src/recorder.cpp
        src/receivercontroller.h src/receivercontroller.cpp
        src/recordingsmodel.h src/recordingsmodel.cpp
        src/videodecoder.h src/videodecoder.cpp
    QML_FILES
        qml/CastView.qml
        qml/ControlBar.qml
        qml/Icon.qml
        qml/IconButton.qml
        qml/IdleView.qml
        qml/Main.qml
        qml/PillButton.qml
        qml/RecordingBadge.qml
        qml/RecordingsDrawer.qml
        qml/RequestCard.qml
        qml/ScreenTile.qml
        qml/SettingsDialog.qml
        qml/StatusPill.qml
        qml/TestPattern.qml
        qml/Theme.qml
        qml/Toast.qml
        qml/TopBar.qml
        qml/WaitingTile.qml
    RESOURCES
        qml/icons/camera.svg
        qml/icons/circle-dot.svg
        qml/icons/copy.svg
        qml/icons/crop.svg
        qml/icons/download.svg
        qml/icons/film.svg
        qml/icons/focus.svg
        qml/icons/folder.svg
        qml/icons/layout-grid.svg
        qml/icons/maximize.svg
        qml/icons/minimize.svg
        qml/icons/monitor.svg
        qml/icons/pause.svg
        qml/icons/play.svg
        qml/icons/plus.svg
        qml/icons/pop-out.svg
        qml/icons/record.svg
        qml/icons/scan.svg
        qml/icons/settings.svg
        qml/icons/smartphone.svg
        qml/icons/trash.svg
        qml/icons/volume-2.svg
        qml/icons/volume-x.svg
        qml/icons/x.svg
)

target_include_directories(beamr_receiver_ui PUBLIC src)
target_link_libraries(beamr_receiver_ui PUBLIC
    beamr_common
    beamr_ffmpeg
    beamr_qrcodegen
    Qt6::Concurrent
    Qt6::Gui
    Qt6::Multimedia
    Qt6::Network
    Qt6::Qml
    Qt6::Quick
    Qt6::Widgets
)

qt_add_executable(beamr_receiver
    src/main.cpp
)

qt_add_resources(beamr_receiver "app_icon"
    PREFIX /beamr
    BASE platform
    FILES platform/beamr.png
)
if(WIN32)
    # The executable's icon in Explorer and the taskbar.
    target_sources(beamr_receiver PRIVATE platform/beamr.rc)
endif()

target_link_libraries(beamr_receiver PRIVATE
    beamr_receiver_ui
    beamr_receiver_uiplugin
    Qt6::Svg
)

set_target_properties(beamr_receiver PROPERTIES
    OUTPUT_NAME beamr-receiver
    WIN32_EXECUTABLE TRUE
)
