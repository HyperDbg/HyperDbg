set(SourceFiles
    "../../../include/platform/user/header/Environment.h"
    "header/pch.h"
    "code/hyperdbg-app.cpp"
)
include_directories(
    "../../../include"
    "header"
)
add_executable(hyperdbg_app ${SourceFiles})
