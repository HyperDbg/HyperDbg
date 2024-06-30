wdk_add_driver(KmdfCppDriver
    KMDF 1.15
    Main.cpp
    )
target_link_libraries(KmdfCppDriver KmdfCppLib)