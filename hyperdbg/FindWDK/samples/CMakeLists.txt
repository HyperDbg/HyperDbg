cmake_minimum_required(VERSION 3.0)

project(FindWdk)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4 /WX")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../cmake")
find_package(WDK REQUIRED)

add_subdirectory(KmdfCppDriver)
add_subdirectory(KmdfCppLib)
add_subdirectory(MinifilterCppDriver)
add_subdirectory(WdmCppDriver)
add_subdirectory(WdmCppLib)
add_subdirectory(WdmDriver)
add_subdirectory(WdmLib)