package main

/*
1. copy ../FindWdk.cmake into C:\Windows
2. build all projects ok
3. rewrite all projects build config into one here
4. get only one dll file with one .h file
5. use clang command to check ast dumper
6. start binding dll for golang

What dll source and header files does libhyperdbg reference in addition to hyperlog?
 We need to sub here to merge the compilation configurations of all dependent projects so that the compilation results in only one unique dll,
 which is the ideal solution. So we need to iterate through all the dependent source and header directories to add the configurations here and
 extract the respective macro definitions to merge them here, along with other compilation parameters.
 Finally just write everything to CMakeLists.txt .
*/