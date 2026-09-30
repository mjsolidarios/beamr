qt_add_library(beamr_common STATIC
    include/beamr/config.h
    include/beamr/connectlink.h
    include/beamr/device.h
    include/beamr/log.h
    include/beamr/protocol.h
    src/connectlink.cpp
    src/log.cpp
    src/protocol.cpp
)

target_include_directories(beamr_common PUBLIC include)
target_link_libraries(beamr_common PUBLIC Qt6::Core)
target_compile_definitions(beamr_common PUBLIC BEAMR_VERSION="${PROJECT_VERSION}")
set_target_properties(beamr_common PROPERTIES POSITION_INDEPENDENT_CODE ON)
