wdk_add_driver(WdmDriver
    Main.c
    )
target_link_libraries(WdmDriver WdmLib)