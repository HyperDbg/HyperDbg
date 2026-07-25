/**
 * @file symbol-stub-linux.c
 * @author Max Raulea (max.raulea@hyperdbg.org)
 * @brief Linux stub implementations of the symbol-parser (Sym*) exports
 * @details The Windows implementation lives in the symbol-parser/ subproject and
 *          is built on DbgHelp + PDB files (via the DIA-SDK-based pdbex), none of
 *          which is available on Linux. The script-engine library calls these
 *          Sym* functions directly, so without definitions libscript-engine.so
 *          fails to link. These stubs satisfy the link and keep every call site
 *          intact; symbol resolution is simply unavailable until a real Linux
 *          backend (ELF/DWARF, or an LLVM DebugInfo/PDB port of symbol-parser)
 *          is implemented.
 *
 *          The signatures mirror include/SDK/imports/user/HyperDbgSymImports.h
 *          exactly. Return values indicate "nothing found / not supported"
 *          (0 / FALSE) and any out-parameters are cleared.
 *
 *          TODO: replace with a real symbol backend and drop this file from the
 *                UNIX branch of script-engine/CMakeLists.txt.
 *
 * @version 0.1
 * @date 2026-07-24
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#ifdef __linux__

VOID
SymSetTextMessageCallback(PVOID Handler)
{
    (void)Handler;
}

VOID
SymbolAbortLoading()
{
}

UINT64
SymConvertNameToAddress(const CHAR * FunctionOrVariableName, PBOOLEAN WasFound)
{
    (void)FunctionOrVariableName;

    if (WasFound != NULL)
    {
        *WasFound = FALSE;
    }

    return 0;
}

UINT32
SymLoadFileSymbol(UINT64 BaseAddress, const CHAR * PdbFileName, const CHAR * CustomModuleName)
{
    (void)BaseAddress;
    (void)PdbFileName;
    (void)CustomModuleName;

    return 0;
}

UINT32
SymUnloadAllSymbols()
{
    return 0;
}

UINT32
SymUnloadModuleSymbol(CHAR * ModuleName)
{
    (void)ModuleName;

    return 0;
}

UINT32
SymSearchSymbolForMask(const CHAR * SearchMask)
{
    (void)SearchMask;

    return 0;
}

BOOLEAN
SymGetFieldOffset(CHAR * TypeName, CHAR * FieldName, UINT32 * FieldOffset)
{
    (void)TypeName;
    (void)FieldName;

    if (FieldOffset != NULL)
    {
        *FieldOffset = 0;
    }

    return FALSE;
}

BOOLEAN
SymGetDataTypeSize(CHAR * TypeName, UINT64 * TypeSize)
{
    (void)TypeName;

    if (TypeSize != NULL)
    {
        *TypeSize = 0;
    }

    return FALSE;
}

BOOLEAN
SymCreateSymbolTableForDisassembler(PVOID CallbackFunction)
{
    (void)CallbackFunction;

    return FALSE;
}

BOOLEAN
SymConvertFileToPdbPath(const CHAR * LocalFilePath, CHAR * ResultPath, SIZE_T ResultPathSize)
{
    (void)LocalFilePath;

    if (ResultPath != NULL && ResultPathSize > 0)
    {
        ResultPath[0] = '\0';
    }

    return FALSE;
}

BOOLEAN
SymConvertFileToPdbFileAndGuidAndAgeDetails(const CHAR * LocalFilePath,
                                            CHAR *       PdbFilePath,
                                            CHAR *       GuidAndAgeDetails,
                                            BOOLEAN      Is32BitModule)
{
    (void)LocalFilePath;
    (void)PdbFilePath;
    (void)GuidAndAgeDetails;
    (void)Is32BitModule;

    return FALSE;
}

BOOLEAN
SymConvertLoadedModuleToPdbFileAndGuidAndAgeDetails(const BYTE * LoadedImageBytes,
                                                    SIZE_T       LoadedImageSize,
                                                    const CHAR * LocalFilePath,
                                                    CHAR *       PdbFilePath,
                                                    CHAR *       GuidAndAgeDetails,
                                                    BOOLEAN      Is32BitModule)
{
    (void)LoadedImageBytes;
    (void)LoadedImageSize;
    (void)LocalFilePath;
    (void)PdbFilePath;
    (void)GuidAndAgeDetails;
    (void)Is32BitModule;

    return FALSE;
}

BOOLEAN
SymbolInitLoad(PVOID        BufferToStoreDetails,
               UINT32       StoredLength,
               BOOLEAN      DownloadIfAvailable,
               const CHAR * SymbolPath,
               BOOLEAN      IsSilentLoad)
{
    (void)BufferToStoreDetails;
    (void)StoredLength;
    (void)DownloadIfAvailable;
    (void)SymbolPath;
    (void)IsSilentLoad;

    return FALSE;
}

BOOLEAN
SymShowDataBasedOnSymbolTypes(const CHAR * TypeName,
                              UINT64       Address,
                              BOOLEAN      IsStruct,
                              PVOID        BufferAddress,
                              const CHAR * AdditionalParameters)
{
    (void)TypeName;
    (void)Address;
    (void)IsStruct;
    (void)BufferAddress;
    (void)AdditionalParameters;

    return FALSE;
}

#endif // __linux__
