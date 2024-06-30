wdk_add_driver(MinifilterCppDriver
    Main.cpp
    )
target_link_libraries(MinifilterCppDriver WDK::FLTMGR)