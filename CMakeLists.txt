cmake_minimum_required(VERSION 3.21)

project(beamr
    VERSION 0.2.5
    DESCRIPTION "Low-latency Wi-Fi screen casting"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Qml Quick)
# Languages the apps are translated into, e.g. -DBEAMR_LANGUAGES="fil;es".
# `cmake --build <dir> --target update_translations` then writes the .ts
# files in each app's i18n/ folder for translators (Qt Linguist opens them);
# the build turns them into the apps' translations.
set(BEAMR_LANGUAGES "" CACHE STRING "Languages to translate the apps into (e.g. fil;es)")
if(BEAMR_LANGUAGES)
    find_package(Qt6 REQUIRED COMPONENTS LinguistTools)
endif()
qt_standard_project_setup(REQUIRES 6.5
    I18N_SOURCE_LANGUAGE en
    I18N_TRANSLATED_LANGUAGES ${BEAMR_LANGUAGES})
# Our QML lives in a qml/ subfolder of each module; let Qt generate the
# extra qmldir it needs (policy introduced in Qt 6.8).
if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()
# Android paths set as target properties (the sender's package source dir)
# are passed through as JSON.
if(QT_KNOWN_POLICY_QTP0002)
    qt_policy(SET QTP0002 NEW)
endif()
# QML modules land in <build>/qml/<URI path>, where qmllint and qmlls expect them.
set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml)

# Android kits build the sender, desktop kits build the receiver.
# The sender can also be built on desktop to iterate on its QML quickly.
if(ANDROID)
    set(_beamr_default_sender ON)
    set(_beamr_default_receiver OFF)
else()
    set(_beamr_default_sender OFF)
    set(_beamr_default_receiver ON)
endif()
option(BEAMR_BUILD_SENDER   "Build the sender app"   ${_beamr_default_sender})
option(BEAMR_BUILD_RECEIVER "Build the receiver app" ${_beamr_default_receiver})

if(ANDROID AND BEAMR_BUILD_RECEIVER)
    message(FATAL_ERROR "The receiver is desktop-only; turn off BEAMR_BUILD_RECEIVER for Android kits.")
endif()

add_subdirectory(common)

if(BEAMR_BUILD_RECEIVER)
    add_subdirectory(receiver-desktop)

    # Console test (not qt_add_executable, which marks Windows apps as GUI).
    add_executable(beamr_tls_roundtrip tests/tls_roundtrip.cpp)
    target_link_libraries(beamr_tls_roundtrip PRIVATE beamr_common Qt6::Network)
    enable_testing()
    add_test(NAME tls_roundtrip COMMAND beamr_tls_roundtrip)
endif()

if(BEAMR_BUILD_SENDER)
    add_subdirectory(sender-android)
endif()
