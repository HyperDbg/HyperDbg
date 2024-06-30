wdk_add_library(WdmLib STATIC 
    WdmLib.h 
    WdmLib.c
    )
target_include_directories(WdmLib INTERFACE .)