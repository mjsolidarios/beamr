find_package(Qt6 REQUIRED COMPONENTS Network Svg)

qt_add_executable(beamr_sender
    src/main.cpp
)

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(beamr_sender
    URI Beamr.Sender
    VERSION 1.0
    SOURCES
        src/discoveryclient.h src/discoveryclient.cpp
        src/receiversession.h src/receiversession.cpp
        src/sendercontroller.h src/sendercontroller.cpp
        src/sessionmodel.h src/sessionmodel.cpp
        src/streamsender.h src/streamsender.cpp
    QML_FILES
        qml/AboutPage.qml
        qml/CastCard.qml
        qml/HomePage.qml
        qml/Icon.qml
        qml/Main.qml
        qml/MessageBanner.qml
        qml/OnboardingArt.qml
        qml/OnboardingView.qml
        qml/PillButton.qml
        qml/ReceiverRow.qml
        qml/SectionLabel.qml
        qml/SessionRow.qml
        qml/SegmentedControl.qml
        qml/SettingsSheet.qml
        qml/SoundSwitch.qml
        qml/Theme.qml
    RESOURCES
        qml/icons/alert.svg
        qml/icons/arrow-left.svg
        qml/icons/book-open.svg
        qml/icons/bug.svg
        qml/icons/cast.svg
        qml/icons/check.svg
        qml/icons/chevron-right.svg
        qml/icons/code.svg
        qml/icons/external-link.svg
        qml/icons/info.svg
        qml/icons/monitor.svg
        qml/icons/moon.svg
        qml/icons/pencil.svg
        qml/icons/scale.svg
        qml/icons/scan-qr-code.svg
        qml/icons/settings.svg
        qml/icons/smartphone.svg
        qml/icons/sun.svg
        qml/icons/volume-2.svg
        qml/icons/volume-x.svg
        qml/icons/x.svg
        qml/images/beamr-icon.png
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
    # The app's permanent identity on Android: changing it later makes a
    # different app that can't update this one.
    QT_ANDROID_PACKAGE_NAME io.github.mjsolidarios.beamr
    QT_ANDROID_APP_NAME beamr
    QT_ANDROID_MIN_SDK_VERSION 29
    QT_ANDROID_VERSION_NAME ${PROJECT_VERSION}
    QT_ANDROID_VERSION_CODE 1
    # Manifest and Java sources for screen capture.
    QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android
)

# Translations (see BEAMR_LANGUAGES in the top-level CMakeLists.txt).
if(BEAMR_LANGUAGES)
    qt_add_translations(beamr_sender
        TS_FILE_BASE beamr_sender
        TS_FILE_DIR i18n
        SOURCE_TARGETS beamr_sender
        RESOURCE_PREFIX /i18n)
endif()
