file(GLOB_RECURSE C_SOURCES "code/*.cpp")
file(GLOB_RECURSE ASM_SOURCES "code/*.asm")
file(GLOB_RECURSE HEADERS "header/*.h")
enable_language(ASM_MASM)
include_directories(
        "."
        "../include"
        "header"
        "../dependencies"
        "../dependencies/zydis/include"
        "../dependencies/zydis/dependencies/zycore/include"
)

add_library(libhyperdbg2   SHARED
        ${C_SOURCES}
        ${ASM_SOURCES}
        ${HEADERS}
)
