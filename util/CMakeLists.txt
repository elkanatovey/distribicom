
# In Debug mode, define DISTRIBICOM_DEBUG
if (DISTRIBICOM_DEBUG OR CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(DISTRIBICOM_DEBUG ON)
    message(STATUS "Distribicom debug mode: ${DISTRIBICOM_DEBUG}")
else ()
    message(STATUS "Distribicom debug mode: OFF")
endif ()

option(FREIVALDS "Enable Freivalds' algorithm" ON)
message(STATUS "FREIVALDS mode: ${FREIVALDS}")

configure_file(config.h.in ${PROJECT_BINARY_DIR}/util/config.h)
