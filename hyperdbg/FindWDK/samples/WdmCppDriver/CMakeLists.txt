wdk_add_driver(WdmCppDriver
    Main.cpp
    )
target_link_libraries(WdmCppDriver WdmCppLib)