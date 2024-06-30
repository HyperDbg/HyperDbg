wdk_add_library(WdmCppLib STATIC 
    WdmCppLib.h 
    WdmCppLib.cpp
    )
target_include_directories(WdmCppLib INTERFACE .)