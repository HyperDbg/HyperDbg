wdk_add_driver(kernel_helloworld main.cpp main.h)
#target_link_libraries(kernel_helloworld "$ENV{WDKContentRoot}\\Lib\\10.0.22000.0\\km\\x64\\wdmsec.lib")
