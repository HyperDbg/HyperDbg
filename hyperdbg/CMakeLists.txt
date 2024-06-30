cmake_minimum_required(VERSION 3.22)
project(EWDK_quickstart)

# Set the WDK target version to Windows 11
set(WDK_WINVER "0x0A00" CACHE STRING "Default WINVER for WDK targets")
add_definitions(-DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W1 /WX")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W1 /WX")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/FindWDK/cmake")
find_package(WDK REQUIRED)

#add_subdirectory("./user_helloworld")
#add_subdirectory("./kernel_helloworld")


#add_definitions(ZYAN_NO_LIBC)
add_subdirectory("./hyperdbg/dependencies/zydis")

add_subdirectory("./hyperdbg/libhyperdbg")


#add_subdirectory("./hyperdbg/hyperhv")
