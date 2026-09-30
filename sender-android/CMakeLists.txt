find_package(Qt6 REQUIRED COMPONENTS Network Svg)

qt_add_executable(beamr_sender
    src/main.cpp
)

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(beamr_sender
    URI Beamr.Sender
    VERSION 1.0
    SOURCES
        src/receiversession.h src/receiversession.cpp
        src/sendercontroller.h src/sendercontroller.cpp
        src/sessionmodel.h src/sessionmodel.cpp
        src/streamsender.h src/streamsender.cpp
    QML_FILES
        qml/CastCard.qml
        qml/HomePage.qml
        qml/Icon.qml
        qml/Main.qml
        qml/MessageBanner.qml
        qml/PillButton.qml
        qml/SectionLabel.qml
        qml/SessionRow.qml
        qml/SettingsSheet.qml
        qml/SoundSwitch.qml
        qml/Theme.qml
    RESOURCES
        qml/icons/alert.svg
        qml/icons/cast.svg
        qml/icons/check.svg
        qml/icons/info.svg
        qml/icons/monitor.svg
        qml/icons/moon.svg
        qml/icons/pencil.svg
        qml/icons/scan-qr-code.svg
        qml/icons/settings.svg
        qml/icons/smartphone.svg
        qml/icons/sun.svg
        qml/icons/volume-2.svg
        qml/icons/volume-x.svg
        qml/icons/x.svg
)

target_include_directories(beamr_sender PRIVATE src)

target_link_libraries(beamr_sender PRIVATE
    beamr_common
    Qt6::Gui
    Qt6::Network
    Qt6::Qml
    Qt6::Quick
    Qt6::Svg
)

# API 29 (Android 10) is the floor for the features we need: foreground
# service type mediaProjection, AudioPlaybackCapture and
# KEY_PREPEND_HEADER_TO_SYNC_FRAMES.
set_target_properties(beamr_sender PROPERTIES
    QT_ANDROID_MIN_SDK_VERSION 29
    QT_ANDROID_VERSION_NAME ${PROJECT_VERSION}
    QT_ANDROID_VERSION_CODE 1
    # Manifest and Java sources for screen capture.
    QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android
)
