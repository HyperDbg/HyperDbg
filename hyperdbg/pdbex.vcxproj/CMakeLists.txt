
find_path(DIA_SDK_INCLUDE_DIR
        NAMES dia2.h
        PATHS
        "$ENV{VSINSTALLDIR}/DIA SDK/include"
        "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/DIA SDK/include"
        "C:/Program Files (x86)/Microsoft Visual Studio/2017/Community/DIA SDK/include"
        DOC "Path to DIA SDK include directory"
)

if(DIA_SDK_INCLUDE_DIR)
    include_directories(${DIA_SDK_INCLUDE_DIR})
    message(STATUS "Found DIA SDK include directory: ${DIA_SDK_INCLUDE_DIR}")
else()
    message(FATAL_ERROR "Could not find DIA SDK include directory")
endif()

set(SourceFiles
        "HyperDbgIntegration.cpp"
        "main.cpp"
        "PDB.cpp"
        "PDBExtractor.cpp"
        "PDBHeaderReconstructor.cpp"
        "HyperDbgExport.h"
        "HyperDbgGlobals.h"
        "PDB.h"
        "PDBCallback.h"
        "PDBExtractor.h"
        "PDBHeaderReconstructor.h"
        "PDBReconstructorBase.h"
        "PDBSymbolSorterAlphabetical.h"
        "PDBSymbolSorterBase.h"
        "PDBSymbolVisitorBase.h"
        "PDBSymbolVisitor.h"
        "PDBSymbolSorter.h"
        "UdtFieldDefinition.h"
        "UdtFieldDefinitionBase.h"
)
add_library(pdbex SHARED ${SourceFiles})

