set(SourceFiles
    "../../../platform/user/header/Environment.h"
    "header/pch.h"
    "code/hyperdbg-app.cpp"
)
include_directories(
    "../../.."
    "header"
)
add_executable(hyperrev ${SourceFiles})
