
include(FetchContent)
FetchContent_Declare(
        com_sealpir
        GIT_REPOSITORY https://github.com/elkanatovey/sealpir-distribicom.git
        GIT_TAG        e01d189489b57ddaef72c8e055c32e83eb11af4a #distribicom sealpir version
        USES_TERMINAL_DOWNLOAD ON
)

if(NOT com_sealpir_POPULATED)
    FetchContent_MakeAvailable(com_sealpir)
endif()
