file(GLOB_RECURSE C_SOURCES "code/*.c")
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

# hyperhv need zydis kernel library ?
#wdk_add_library()

wdk_add_driver(hyperhv
        ${C_SOURCES}
        ${ASM_SOURCES}
        ${HEADERS}
)
