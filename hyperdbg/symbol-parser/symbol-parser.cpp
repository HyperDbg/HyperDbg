/**
 * @file symbol-parser.cpp
 * @author Alee Amini (aleeaminiz@gmail.com)
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief symbol parser
 * @details
 * @version 0.1
 * @date 2021-05-29
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Convert function name to address
 *
 * @param BaseAddress
 * @param FileName
 * @param Guid
 * 
 * @return UINT64
 */
UINT64
SymLoadFileSymbol(UINT64 BaseAddress, const char * FileName, const char * Guid)
{
    // printf("hello from symbol loaded base address is : %llx !\n", BaseAddress);
    return 0x55;
}

/**
 * @brief Convert function name to address
 *
 * @param FunctionName
 * @param WasFound
 * 
 * @return UINT64
 */
UINT64
SymConvertNameToAddress(const char * FunctionName, PBOOLEAN WasFound)
{
    BOOLEAN Found   = FALSE;
    UINT64  Address = NULL;

    //
    // Not found by default
    //
    *WasFound = FALSE;

    if (strcmp(FunctionName, "test1") == 0)
    {
        Found   = TRUE;
        Address = 0x85;
    }
    else if (strcmp(FunctionName, "test2") == 0)
    {
        Found   = TRUE;
        Address = 0x185;
    }
    else
    {
        Found   = FALSE;
        Address = NULL;
    }

    *WasFound = Found;
    return Address;
}

/**
 * @brief Enumerate all the symbols in a file
 *
 * @param PdbFilePath
 * @param SearchMask
 * @param BaseAddr
 * @param FileSize
 * 
 * @return UINT32
 */
UINT32
SymSymbolsEnumerateAll(char * PdbFilePath, const char * SearchMask, DWORD64 & BaseAddr, DWORD & FileSize)
{
    BOOL  Ret     = FALSE;
    DWORD Options = SymGetOptions();

    //
    // SYMOPT_DEBUG option asks DbgHelp to print additional troubleshooting
    // messages to debug output - use the debugger's Debug Output window
    // to view the messages
    //
    Options |= SYMOPT_DEBUG;
    SymSetOptions(Options);

    //
    // Initialize DbgHelp and load symbols for all modules of the current process
    //
    Ret = SymInitialize(
        GetCurrentProcess(), // Process handle of the current process
        NULL,                // No user-defined search path -> use default
        FALSE                // Do not load symbols for modules in the current process
    );

    if (!Ret)
    {
        printf("err, symbol init failed (%u)\n",
               GetLastError());
        return 0;
    }

    do
    {
        //
        // Determine the base address and the file size
        //
        DWORD64 BaseAddr = 0;
        DWORD   FileSize = 0;

        if (!SymGetFileParams(PdbFilePath, BaseAddr, FileSize))
        {
            printf("err, cannot obtain file parameters (internal error)\n");
            break;
        }

        //
        // Load symbols for the module
        //
        printf("loading symbols for: %s\n", PdbFilePath);

        DWORD64 ModBase = SymLoadModule64(
            GetCurrentProcess(), // Process handle of the current process
            NULL,                // Handle to the module's image file (not needed)
            PdbFilePath,         // Path/name of the file
            NULL,                // User-defined short name of the module (it can be NULL)
            BaseAddr,            // Base address of the module (cannot be NULL if .PDB file is
                                 // used, otherwise it can be NULL)
            FileSize             // Size of the file (cannot be NULL if .PDB file is used,
                                 // otherwise it can be NULL)
        );

        if (ModBase == 0)
        {
            printf("err, loading symbols failed (%u)\n",
                   GetLastError());
            break;
        }

        printf("load address: %I64x\n", ModBase);

        //
        // Obtain and display information about loaded symbols
        //
        SymShowSymbolInfo(ModBase);

        //
        // Enumerate symbols and display information about them
        //
        if (SearchMask != NULL)
            printf("search mask: %s\n", SearchMask);

        printf("symbols:\n");

        Ret = SymEnumSymbols(
            GetCurrentProcess(),    // Process handle of the current process
            ModBase,                // Base address of the module
            SearchMask,             // Mask (NULL -> all symbols)
            SymEnumSymbolsCallback, // The callback function
            NULL                    // A used-defined context can be passed here, if necessary
        );

        if (!Ret)
        {
            printf("err, symbol enum failed (%u)\n",
                   GetLastError());
        }

        //
        // Unload symbols for the module
        //
        Ret = SymUnloadModule64(GetCurrentProcess(), ModBase);

        if (!Ret)
        {
            printf("err, unload symbol failed (%u)\n",
                   GetLastError());
        }

    } while (0);

    //
    // Uninitialize DbgHelp
    //
    Ret = SymCleanup(GetCurrentProcess());

    if (!Ret)
    {
        printf("err, symbol cleanup failed (%u)\n", GetLastError());
        return 0;
    }

    return 0;
}

/**
 * @brief Get symbol file parameters
 *
 * @param FileName
 * @param BaseAddr
 * @param FileSize
 * 
 * @return BOOL
 */
BOOL
SymGetFileParams(const char * FileName, DWORD64 & BaseAddr, DWORD & FileSize)
{
    //
    // Check parameters
    //
    if (FileName == 0)
    {
        return FALSE;
    }

    //
    // Determine the extension of the file
    //
    char FileExt[_MAX_EXT] = {0};

    _splitpath(FileName, NULL, NULL, NULL, FileExt);

    //
    // Is it .PDB file?
    //
    if (strcmp(FileExt, (".pdb")) == 0 || strcmp(FileExt, (".PDB")) == 0)
    {
        //
        // Yes, it is a .PDB file
        // Determine its size, and use a dummy base address
        //

        //
        // it can be any non-zero value, but if we load
        // symbols from more than one file, memory regions
        // specified for different files should not overlap
        // (region is "base address + file size")
        //
        BaseAddr = 0x40000000;

        if (!SymGetFileSize(FileName, FileSize))
        {
            return FALSE;
        }
    }
    else
    {
        //
        // It is not a .PDB file
        // Base address and file size can be 0
        //

        BaseAddr = 0;
        FileSize = 0;
    }

    return TRUE;
}

/**
 * @brief Get symbol file size
 *
 * @param FileName
 * @param FileSize
 * 
 * @return BOOL
 */
BOOL
SymGetFileSize(const char * FileName, DWORD & FileSize)
{
    //
    // Check parameters
    //
    if (FileName == 0)
    {
        return FALSE;
    }

    //
    // Open the file
    //
    HANDLE hFile = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        printf("err, unable to open symbol file (%u)\n", GetLastError());
        return FALSE;
    }

    //
    // Obtain the size of the file
    //
    FileSize = GetFileSize(hFile, NULL);

    if (FileSize == INVALID_FILE_SIZE)
    {
        printf("err, unable to get symbol file size (%u)\n", GetLastError());

        //
        // and continue ...
        //
    }

    //
    // Close the file
    //
    if (!CloseHandle(hFile))
    {
        printf("err, unable to close symbol file (%u)\n", GetLastError());

        //
        // and continue ...
        //
    }

    return (FileSize != INVALID_FILE_SIZE);
}

/**
 * @brief Show symbol info
 *
 * @param ModBase
 * 
 * @return VOID
 */
VOID
SymShowSymbolInfo(DWORD64 ModBase)
{
    //
    // Get module information
    //
    IMAGEHLP_MODULE64 ModuleInfo;

    memset(&ModuleInfo, 0, sizeof(ModuleInfo));

    ModuleInfo.SizeOfStruct = sizeof(ModuleInfo);

    BOOL Ret = SymGetModuleInfo64(GetCurrentProcess(), ModBase, &ModuleInfo);

    if (!Ret)
    {
        printf("err, unable to get symbol file information (%u)\n",
               GetLastError());
        return;
    }

    //
    // Display information about symbols
    // Kind of symbols
    //
    switch (ModuleInfo.SymType)
    {
    case SymNone:
        printf("no symbols available for the module\n");
        break;

    case SymExport:
        printf("loaded symbols: Exports\n");
        break;

    case SymCoff:
        printf("loaded symbols: COFF\n");
        break;

    case SymCv:
        printf("loaded symbols: CodeView\n");
        break;

    case SymSym:
        printf("loaded symbols: SYM\n");
        break;

    case SymVirtual:
        printf("loaded symbols: Virtual\n");
        break;

    case SymPdb:
        printf("loaded symbols: PDB\n");
        break;

    case SymDia:
        printf("loaded symbols: DIA\n");
        break;

    case SymDeferred:

        //
        // not actually loaded
        //
        printf("loaded symbols: Deferred\n");
        break;

    default:
        printf("loaded symbols: Unknown format\n");
        break;
    }

    //
    // Image name
    //
    if (strlen(ModuleInfo.ImageName) > 0)
    {
        printf("image name: %s\n", ModuleInfo.ImageName);
    }

    //
    // Loaded image name
    //
    if (strlen(ModuleInfo.LoadedImageName) > 0)
    {
        printf("loaded image name: %s\n", ModuleInfo.LoadedImageName);
    }

    //
    // Loaded PDB name
    //
    if (strlen(ModuleInfo.LoadedPdbName) > 0)
    {
        printf("PDB file name: %s\n", ModuleInfo.LoadedPdbName);
    }

    //
    // Is debug information unmatched?
    // (It can only happen if the debug information is contained
    // in a separate file (.DBG or .PDB)
    //
    if (ModuleInfo.PdbUnmatched || ModuleInfo.DbgUnmatched)
    {
        printf("warning, unmatched symbols\n");
    }

    //
    // *** Contents ***
    //

    //
    // Line numbers available?
    //
    printf("line numbers: %s\n",
           ModuleInfo.LineNumbers ? "available" : "not available");

    //
    // Global symbols available?
    //
    printf("global symbols: %s\n",
           ModuleInfo.GlobalSymbols ? "available" : "not available");

    //
    // Type information available?
    //
    printf("type information: %s\n",
           ModuleInfo.TypeInfo ? ("Available") : ("Not available"));

    //
    // Source indexing available?
    //
    printf("source indexing: %s\n",
           ModuleInfo.SourceIndexed ? "yes" : "no");

    //
    // Public symbols available?
    //
    printf("public symbols: %s\n",
           ModuleInfo.Publics ? "available" : "not available");
}

/**
 * @brief Callback for showing and enumerating symbols
 *
 * @param SymInfo
 * @param SymbolSize
 * @param UserContext
 * 
 * @return BOOL
 */
BOOL CALLBACK
SymEnumSymbolsCallback(SYMBOL_INFO * SymInfo, ULONG SymbolSize, PVOID UserContext)
{
    if (SymInfo != 0)
    {
        SymShowSymbolDetails(*SymInfo);
    }

    //
    // Continue enumeration
    //
    return TRUE;
}

/**
 * @brief Show symbols details
 *
 * @param SymInfo
 * 
 * @return VOID
 */
VOID
SymShowSymbolDetails(SYMBOL_INFO & SymInfo)
{
    //
    // Kind of symbol (tag)
    //
    printf("symbol: %s  ", SymTagStr(SymInfo.Tag));

    //
    // Address
    //
    printf("address: %x  ", SymInfo.Address);

    //
    // Size
    //
    printf("size: %u  ", SymInfo.Size);

    //
    // Name
    //
    printf("name: %s\n", SymInfo.Name);
}

/**
 * @brief Interpret different tags for pdbs
 *
 * @param Tag
 * 
 * @return const char *
 */
const char *
SymTagStr(ULONG Tag)
{
    switch (Tag)
    {
    case SymTagNull:
        return ("Null");

    case SymTagExe:
        return ("Exe");

    case SymTagCompiland:
        return ("Compiland");

    case SymTagCompilandDetails:
        return ("CompilandDetails");

    case SymTagCompilandEnv:
        return ("CompilandEnv");

    case SymTagFunction:
        return ("Function");

    case SymTagBlock:
        return ("Block");

    case SymTagData:
        return ("Data");

    case SymTagAnnotation:
        return ("Annotation");

    case SymTagLabel:
        return ("Label");

    case SymTagPublicSymbol:
        return ("PublicSymbol");

    case SymTagUDT:
        return ("UDT");

    case SymTagEnum:
        return ("Enum");

    case SymTagFunctionType:
        return ("FunctionType");

    case SymTagPointerType:
        return ("PointerType");

    case SymTagArrayType:
        return ("ArrayType");

    case SymTagBaseType:
        return ("BaseType");

    case SymTagTypedef:
        return ("Typedef");

    case SymTagBaseClass:
        return ("BaseClass");

    case SymTagFriend:
        return ("Friend");

    case SymTagFunctionArgType:
        return ("FunctionArgType");

    case SymTagFuncDebugStart:
        return ("FuncDebugStart");

    case SymTagFuncDebugEnd:
        return ("FuncDebugEnd");

    case SymTagUsingNamespace:
        return ("UsingNamespace");

    case SymTagVTableShape:
        return ("VTableShape");

    case SymTagVTable:
        return ("VTable");

    case SymTagCustom:
        return ("Custom");

    case SymTagThunk:
        return ("Thunk");

    case SymTagCustomType:
        return ("CustomType");

    case SymTagManagedType:
        return ("ManagedType");

    case SymTagDimension:
        return ("Dimension");

    default:
        return ("Unknown");
    }

    return ("");
}
