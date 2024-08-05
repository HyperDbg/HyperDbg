set(SourceFiles
    "code/casting.cpp"
    "code/common-utils.cpp"
    "code/symbol-parser.cpp"
    "pch.cpp"
    "../include/platform/user/header/Environment.h"
    "header/common-utils.h"
    "header/symbol-parser.h"
    "pch.h"
)
include_directories(
    "../include"
    "../dependencies"
    "."
)


set(save_src_dir ${CMAKE_CURRENT_SOURCE_DIR}/pdbex/)
execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/pdbex/Source ${save_src_dir})

add_subdirectory("pdbex")

add_library(symbol-parser SHARED ${SourceFiles})

target_link_libraries(symbol-parser  pdbex)
