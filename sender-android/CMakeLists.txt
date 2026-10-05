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
        qml/LicensesPage.qml
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
    # Pinned so a Qt upgrade can't quietly lower the Play target.
    QT_ANDROID_TARGET_SDK_VERSION 36
    QT_ANDROID_VERSION_NAME ${PROJECT_VERSION}
    QT_ANDROID_VERSION_CODE 6
    # Manifest and Java sources for screen capture.
    QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android
)

# Qt loads OpenSSL itself and does not put it in the APK. Without these
# libraries the phone logs "TLS initialization failed" and every cast fails
# before the handshake. Pinned so a rebuild keeps the same OpenSSL 3.1.8.
if(ANDROID)
    include(FetchContent)
    FetchContent_Declare(
        android_openssl
        URL https://github.com/KDAB/android_openssl/archive/b71f1470962019bd89534a2919f5925f93bc5779.zip
        URL_HASH SHA256=9277d62ecdb4809801e2c369e0a639c154e0d9137e8d60863b44bf07d16ed5b3
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(android_openssl)
    include(${android_openssl_SOURCE_DIR}/android_openssl.cmake)
    add_android_openssl_libraries(beamr_sender)
endif()

# Qt Core asks for shared storage, which beamr never uses. The manifest
# merger is also told to drop it. Filtering here keeps it out of the
# deployment JSON that androiddeployqt copies into the manifest.
if(ANDROID)
    get_target_property(_beamr_core_permissions Qt6::Core INTERFACE_QT_ANDROID_PERMISSIONS)
    if(_beamr_core_permissions)
        set(_beamr_kept_permissions "")
        foreach(_beamr_permission IN LISTS _beamr_core_permissions)
            if(_beamr_permission MATCHES "WRITE_EXTERNAL_STORAGE")
                continue()
            endif()
            string(REPLACE ";" "\\;" _beamr_permission "${_beamr_permission}")
            list(APPEND _beamr_kept_permissions "${_beamr_permission}")
        endforeach()
        set_property(TARGET Qt6::Core PROPERTY INTERFACE_QT_ANDROID_PERMISSIONS "${_beamr_kept_permissions}")
    endif()
endif()

# Translations (see BEAMR_LANGUAGES in the top-level CMakeLists.txt).
if(BEAMR_LANGUAGES)
    qt_add_translations(beamr_sender
        TS_FILE_BASE beamr_sender
        TS_FILE_DIR i18n
        SOURCE_TARGETS beamr_sender
        RESOURCE_PREFIX /i18n)
endif()
