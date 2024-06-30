package main

/*
What dll source and header files does libhyperdbg reference in addition to hyperlog?
 We need to sub here to merge the compilation configurations of all dependent projects so that the compilation results in only one unique dll,
 which is the ideal solution. So we need to iterate through all the dependent source and header directories to add the configurations here and
 extract the respective macro definitions to merge them here, along with other compilation parameters.
*/