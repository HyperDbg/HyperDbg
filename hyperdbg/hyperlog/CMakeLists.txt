set(SourceFiles
    "../include/components/spinlock/code/Spinlock.c"
    "../include/platform/kernel/code/Mem.c"
    "code/Logging.c"
    "code/UnloadDll.c"
    "../include/components/spinlock/header/Spinlock.h"
    "../include/platform/kernel/header/Environment.h"
    "../include/platform/kernel/header/Mem.h"
    "header/Logging.h"
    "header/pch.h"
    "header/UnloadDll.h"
        hyperlog.def
)
include_directories(
    "../include"
    "header"
)
wdk_add_library(hyperlog SHARED
    KMDF 1.15
    ${SourceFiles}
)
