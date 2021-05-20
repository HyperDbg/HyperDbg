/**
 * @file symbol.cpp
 * @author Alee Amini (aleeaminiz@gmail.com)
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief symbol parser
 * @details
 * @version 0.1
 * @date 2021-05-20
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

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
    Ret = ::SymInitialize(
        GetCurrentProcess(), // Process handle of the current process
        NULL,                // No user-defined search path -> use default
        FALSE                // Do not load symbols for modules in the current process
    );

    if (!Ret)
    {
        ShowMessages(("Error: SymInitialize() failed. Error code: %u \n"),
                     ::GetLastError());
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
            ShowMessages(("Error: Cannot obtain file parameters (internal error).\n"));
            break;
        }

        //
        // Load symbols for the module
        //
        ShowMessages(("Loading symbols for: %s ... \n"), PdbFilePath);

        DWORD64 ModBase = ::SymLoadModule64(
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
            ShowMessages(("Error: SymLoadModule64() failed. Error code: %u \n"),
                         ::GetLastError());
            break;
        }

        ShowMessages(("Load address: %I64x \n"), ModBase);

        //
        // Obtain and display information about loaded symbols
        //
        SymShowSymbolInfo(ModBase);

        //
        // Enumerate symbols and display information about them
        //
        if (SearchMask != NULL)
            ShowMessages(("Search mask: %s \n"), SearchMask);

        ShowMessages(("Symbols: \n"));

        Ret = SymEnumSymbols(
            GetCurrentProcess(),    // Process handle of the current process
            ModBase,                // Base address of the module
            SearchMask,             // Mask (NULL -> all symbols)
            SymEnumSymbolsCallback, // The callback function
            NULL                    // A used-defined context can be passed here, if necessary
        );

        if (!Ret)
        {
            ShowMessages(("Error: SymEnumSymbols() failed. Error code: %u \n"),
                         ::GetLastError());
        }

        //
        // Unload symbols for the module
        //
        Ret = ::SymUnloadModule64(GetCurrentProcess(), ModBase);

        if (!Ret)
        {
            ShowMessages(("Error: SymUnloadModule64() failed. Error code: %u \n"),
                         ::GetLastError());
        }

    } while (0);

    //
    // Uninitialize DbgHelp
    //
    Ret = SymCleanup(GetCurrentProcess());

    if (!Ret)
    {
        ShowMessages(("Error: SymCleanup() failed. Error code: %u \n"), ::GetLastError());
        return 0;
    }

    return 0;
}

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
    HANDLE hFile = ::CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        ShowMessages(("CreateFile() failed. Error: %u \n"), ::GetLastError());
        return FALSE;
    }

    //
    // Obtain the size of the file
    //
    FileSize = GetFileSize(hFile, NULL);

    if (FileSize == INVALID_FILE_SIZE)
    {
        ShowMessages(("GetFileSize() failed. Error: %u \n"), ::GetLastError());

        //
        // and continue ...
        //
    }

    //
    // Close the file
    //
    if (!::CloseHandle(hFile))
    {
        ShowMessages(("CloseHandle() failed. Error: %u \n"), ::GetLastError());

        //
        // and continue ...
        //
    }

    return (FileSize != INVALID_FILE_SIZE);
}

VOID
SymShowSymbolInfo(DWORD64 ModBase)
{
    //
    // Get module information
    //
    IMAGEHLP_MODULE64 ModuleInfo;

    memset(&ModuleInfo, 0, sizeof(ModuleInfo));

    ModuleInfo.SizeOfStruct = sizeof(ModuleInfo);

    BOOL Ret = ::SymGetModuleInfo64(GetCurrentProcess(), ModBase, &ModuleInfo);

    if (!Ret)
    {
        ShowMessages(("Error: SymGetModuleInfo64() failed. Error code: %u \n"),
                     ::GetLastError());
        return;
    }

    //
    // Display information about symbols
    // Kind of symbols
    //
    switch (ModuleInfo.SymType)
    {
    case SymNone:
        ShowMessages(("No symbols available for the module.\n"));
        break;

    case SymExport:
        ShowMessages(("Loaded symbols: Exports\n"));
        break;

    case SymCoff:
        ShowMessages(("Loaded symbols: COFF\n"));
        break;

    case SymCv:
        ShowMessages(("Loaded symbols: CodeView\n"));
        break;

    case SymSym:
        ShowMessages(("Loaded symbols: SYM\n"));
        break;

    case SymVirtual:
        ShowMessages(("Loaded symbols: Virtual\n"));
        break;

    case SymPdb:
        ShowMessages(("Loaded symbols: PDB\n"));
        break;

    case SymDia:
        ShowMessages(("Loaded symbols: DIA\n"));
        break;

    case SymDeferred:

        //
        // not actually loaded
        //
        ShowMessages(("Loaded symbols: Deferred\n"));
        break;

    default:
        ShowMessages(("Loaded symbols: Unknown format.\n"));
        break;
    }

    //
    // Image name
    //
    if (strlen(ModuleInfo.ImageName) > 0)
    {
        ShowMessages(("Image name: %s \n"), ModuleInfo.ImageName);
    }

    //
    // Loaded image name
    //
    if (strlen(ModuleInfo.LoadedImageName) > 0)
    {
        ShowMessages(("Loaded image name: %s \n"), ModuleInfo.LoadedImageName);
    }

    //
    // Loaded PDB name
    //
    if (strlen(ModuleInfo.LoadedPdbName) > 0)
    {
        ShowMessages(("PDB file name: %s \n"), ModuleInfo.LoadedPdbName);
    }

    //
    // Is debug information unmatched?
    // (It can only happen if the debug information is contained
    // in a separate file (.DBG or .PDB)
    //
    if (ModuleInfo.PdbUnmatched || ModuleInfo.DbgUnmatched)
    {
        ShowMessages(("Warning: Unmatched symbols. \n"));
    }

    //
    // *** Contents ***
    //

    //
    // Line numbers available?
    //
    ShowMessages(("Line numbers: %s \n"),
                 ModuleInfo.LineNumbers ? ("Available") : ("Not available"));

    //
    // Global symbols available?
    //
    ShowMessages(("Global symbols: %s \n"),
                 ModuleInfo.GlobalSymbols ? ("Available") : ("Not available"));

    //
    // Type information available?
    //
    ShowMessages(("Type information: %s \n"),
                 ModuleInfo.TypeInfo ? ("Available") : ("Not available"));

    //
    // Source indexing available?
    //
    ShowMessages(("Source indexing: %s \n"),
                 ModuleInfo.SourceIndexed ? ("Yes") : ("No"));

    //
    // Public symbols available?
    //
    ShowMessages(("Public symbols: %s \n"),
                 ModuleInfo.Publics ? ("Available") : ("Not available"));
}

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

VOID
SymShowSymbolDetails(SYMBOL_INFO & SymInfo)
{
    //
    // Kind of symbol (tag)
    //
    ShowMessages(("Symbol: %s  "), SymTagStr(SymInfo.Tag));

    //
    // Address
    //
    ShowMessages(("Address: %x  "), SymInfo.Address);

    //
    // Size
    //
    ShowMessages(("Size: %u  "), SymInfo.Size);

    //
    // Name
    //
    ShowMessages(("Name: %s"), SymInfo.Name);

    ShowMessages(("\n"));
}

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
