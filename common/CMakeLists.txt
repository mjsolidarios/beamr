find_package(Qt6 REQUIRED COMPONENTS Network)

qt_add_library(beamr_common STATIC
    include/beamr/config.h
    include/beamr/connectlink.h
    include/beamr/device.h
    include/beamr/log.h
    include/beamr/notices.h
    include/beamr/protocol.h
    include/beamr/tlsidentity.h
    src/connectlink.cpp
    src/log.cpp
    src/notices.cpp
    src/p256cert.h
    src/p256cert.cpp
    src/protocol.cpp
    src/tlsidentity.cpp
)

# micro-ecc mints the receiver's TLS certificate. The asm backends are not
# vendored; the portable C implementation is selected with uECC_PLATFORM=0.
enable_language(C)
target_sources(beamr_common PRIVATE "${PROJECT_SOURCE_DIR}/third_party/micro-ecc/uECC.c")
target_include_directories(beamr_common PRIVATE "${PROJECT_SOURCE_DIR}/third_party/micro-ecc")
target_compile_definitions(beamr_common PRIVATE
    uECC_PLATFORM=0
    uECC_SUPPORTS_secp160r1=0
    uECC_SUPPORTS_secp192r1=0
    uECC_SUPPORTS_secp224r1=0
    uECC_SUPPORTS_secp256k1=0
    uECC_SUPPORT_COMPRESSED_POINT=0
    uECC_OPTIMIZATION_LEVEL=2
)
if(MSVC)
    set_source_files_properties("${PROJECT_SOURCE_DIR}/third_party/micro-ecc/uECC.c"
        PROPERTIES COMPILE_OPTIONS "/w")
else()
    set_source_files_properties("${PROJECT_SOURCE_DIR}/third_party/micro-ecc/uECC.c"
        PROPERTIES COMPILE_OPTIONS "-w")
endif()

# Notices and license texts, so both apps can show them offline. Static
# libraries drop an unreferenced resource initializer; notices.cpp calls
# Q_INIT_RESOURCE(beamr_notices).
qt_add_resources(beamr_common beamr_notices
    PREFIX "/beamr/notices"
    BASE "${PROJECT_SOURCE_DIR}"
    FILES
        "${PROJECT_SOURCE_DIR}/THIRD_PARTY_NOTICES.md"
        "${PROJECT_SOURCE_DIR}/LICENSE"
        "${PROJECT_SOURCE_DIR}/licenses/Apache-2.0.txt"
        "${PROJECT_SOURCE_DIR}/licenses/GPL-3.0.txt"
        "${PROJECT_SOURCE_DIR}/licenses/LGPL-3.0.txt"
        "${PROJECT_SOURCE_DIR}/licenses/LLVM-libc++.txt"
        "${PROJECT_SOURCE_DIR}/licenses/Lucide-ISC.txt"
        "${PROJECT_SOURCE_DIR}/third_party/ffmpeg/COPYING.LGPLv2.1"
        "${PROJECT_SOURCE_DIR}/third_party/micro-ecc/LICENSE.txt"
)

target_include_directories(beamr_common PUBLIC include)
target_link_libraries(beamr_common PUBLIC Qt6::Core Qt6::Network)
target_compile_definitions(beamr_common PUBLIC BEAMR_VERSION="${PROJECT_VERSION}")
set_target_properties(beamr_common PROPERTIES POSITION_INDEPENDENT_CODE ON)
