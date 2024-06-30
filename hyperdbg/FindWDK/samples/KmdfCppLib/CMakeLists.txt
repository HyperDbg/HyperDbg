wdk_add_library(KmdfCppLib STATIC 
    KMDF 1.15
    KmdfCppLib.h 
    KmdfCppLib.cpp
    )
target_include_directories(KmdfCppLib INTERFACE .)