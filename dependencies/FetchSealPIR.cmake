
include(FetchContent)
FetchContent_Declare(
        com_sealpir
        GIT_REPOSITORY https://github.com/elkanatovey/sealpir-distribicom.git
        GIT_TAG        e7e3389b20aba29347d12ecb146b14af22db7a9d #distribicom sealpir version
        USES_TERMINAL_DOWNLOAD ON
)

if(NOT com_sealpir_POPULATED)
    FetchContent_MakeAvailable(com_sealpir)
endif()
