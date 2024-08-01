set(SourceFiles
    "../../../platform/user/header/Environment.h"
    "header/pch.h"
    "code/hyperdbg-app.cpp"
)
include_directories(
    "../../../HyperDbg/include"
    "header"
)
add_executable(hyperdbg_app ${SourceFiles})
