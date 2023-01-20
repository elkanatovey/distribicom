
include(FetchContent)
FetchContent_Declare(
        com_sealpir
        GIT_REPOSITORY https://github.com/elkanatovey/sealpir-distribicom.git
        GIT_TAG        370be4d69ba242b28905f9da2f41ee667a13a456 #distribicom sealpir version
        USES_TERMINAL_DOWNLOAD ON
)

FetchContent_GetProperties(com_sealpir)

if(NOT com_sealpir_POPULATED)
    FetchContent_Populate(com_sealpir)
    add_subdirectory(
            ${com_sealpir_SOURCE_DIR}
            ${com_sealpir_BINARY_DIR}
            EXCLUDE_FROM_ALL)
endif()
