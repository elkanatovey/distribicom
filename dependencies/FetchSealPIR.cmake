
include(FetchContent)
FetchContent_Declare(
        com_sealpir
        GIT_REPOSITORY https://github.com/elkanatovey/sealpir-distribicom.git
        GIT_TAG        190ebbd2fe9f127840bc658e174944f4d225b238 #distribicom sealpir version
        USES_TERMINAL_DOWNLOAD ON
)

if(NOT com_sealpir_POPULATED)
    FetchContent_MakeAvailable(com_sealpir)
endif()
