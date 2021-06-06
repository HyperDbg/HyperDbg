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

//
// Global Variables
//
extern std::vector<PSYMBOL_LOADED_MODULE_DETAILS> g_LoadedModules;
extern BOOLEAN                                    g_IsLoadedModulesInitialized;
extern CHAR *                                     g_CurrentModuleName;
extern CHAR                                       g_NtModuleName[_MAX_FNAME];

/**
 * @brief Interpret and find module base , based on module name 
 * @param SearchMask
 * 
 * @return DWORD64 NULL means error or not found, otherwise the address
 */
DWORD64
SymGetModuleBaseFromSearchMask(const char * SearchMask, BOOLEAN SetModuleNameGlobally)
{
    string Token;
    char   ModuleName[_MAX_FNAME] = {0};
    string Delimiter              = "!";
    int    Index                  = 0;
    char   Ch                     = NULL;

    if (!g_IsLoadedModulesInitialized || SearchMask == NULL)
    {
        //
        // no module is loaded or search mask is invalid
        //
        return NULL;
    }

    //
    // Convert search mask to string
    //
    string SearchMaskString(SearchMask);

    //
    // Check if the string contains '!'
    //
    if (SearchMaskString.find('!') != std::string::npos)
    {
        //
        // Found
        //
        Token = SearchMaskString.substr(0, SearchMaskString.find(Delimiter));

        strcpy(ModuleName, Token.c_str());

        if (strlen(ModuleName) == 0)
        {
            //
            // Invalid name
            //
            return NULL;
        }

        //
        // Convert module name to lowercase
        //
        while (ModuleName[Index])
        {
            Ch = ModuleName[Index];

            //
            // convert ch to lowercase using toLower()
            //
            ModuleName[Index] = tolower(Ch);

            Index++;
        }

        if (strcmp(ModuleName, "ntkrnlmp") == 0 || strcmp(ModuleName, "ntoskrnl") == 0 ||
            strcmp(ModuleName, "ntkrpamp") == 0 || strcmp(ModuleName, "ntkrnlpa") == 0)
        {
            //
            // It's "nt"
            //
            RtlZeroMemory(ModuleName, _MAX_FNAME);

            //
            // Move nt as the name
            //
            ModuleName[0] = 'n';
            ModuleName[1] = 't';
        }
    }
    else
    {
        //
        // There is no '!' in the middle of the search mask so,
        // we assume that the module is nt
        //
        RtlZeroMemory(ModuleName, _MAX_FNAME);

        ModuleName[0] = 'n';
        ModuleName[1] = 't';
    }

    //
    // ************* Interpret based on remarks of the "X" command *************
    //
    for (auto item : g_LoadedModules)
    {
        if (strcmp((const char *)item->ModuleName, ModuleName) == 0)
        {
            if (SetModuleNameGlobally)
            {
                g_CurrentModuleName = (char *)item->ModuleName;
            }

            return item->ModuleBase;
        }
    }

    //
    // If the function continues until here then it means
    // that the module not found
    //
    return NULL;
}

/**
 * @brief load symbol based on a file name and GUID
 *
 * @param BaseAddress
 * @param FileName
 * @param Guid
 * 
 * @return UINT32
 */
UINT32
SymLoadFileSymbol(UINT64 BaseAddress, const char * PdbFileName)
{
    BOOL                          Ret                             = FALSE;
    DWORD                         Options                         = 0;
    DWORD                         FileSize                        = 0;
    int                           Index                           = 0;
    char                          Ch                              = NULL;
    char                          ModuleName[_MAX_FNAME]          = {0};
    char                          AlternateModuleName[_MAX_FNAME] = {0};
    PSYMBOL_LOADED_MODULE_DETAILS ModuleDetails                   = NULL;

    //
    // Get options
    //
    Options = SymGetOptions();

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
        return -1;
    }

    //
    // Determine the base address and the file size
    //
    if (!SymGetFileParams(PdbFileName, FileSize))
    {
        printf("err, cannot obtain file parameters (internal error)\n");
        return -1;
    }

    //
    // Determine the extension of the file
    //
    _splitpath(PdbFileName, NULL, NULL, ModuleName, NULL);

    //
    // Move to alternate list
    //
    strcpy(AlternateModuleName, ModuleName);

    //
    // Convert module name to lowercase
    //
    while (ModuleName[Index])
    {
        Ch = ModuleName[Index];

        //
        // convert ch to lowercase using toLower()
        //
        ModuleName[Index] = tolower(Ch);

        Index++;
    }

    //
    // Is it "nt" module or not
    //
    // Names of kernel
    //     NTOSKRNL.EXE : 1 CPU
    //     NTKRNLMP.EXE : N CPU, SMP
    //     NTKRNLPA.EXE : 1 CPU, PAE
    //     NTKRPAMP.EXE : N CPU SMP, PAE
    //
    if (strcmp(ModuleName, ("ntkrnlmp")) == 0 || strcmp(ModuleName, ("ntoskrnl")) == 0 ||
        strcmp(ModuleName, ("ntkrpamp")) == 0 || strcmp(ModuleName, ("ntkrnlpa")) == 0)
    {
        //
        // It's "nt"
        //
        RtlZeroMemory(ModuleName, _MAX_FNAME);

        //
        // Move nt as the name
        //
        ModuleName[0] = 'n';
        ModuleName[1] = 't';

        //
        // Describe it as main nt module
        //
        RtlZeroMemory(g_NtModuleName, _MAX_FNAME);
        strcpy(g_NtModuleName, AlternateModuleName);
    }

    //
    // Allocate buffer to store the details
    //
    ModuleDetails = (SYMBOL_LOADED_MODULE_DETAILS *)malloc(sizeof(SYMBOL_LOADED_MODULE_DETAILS));

    if (ModuleDetails == NULL)
    {
        printf("err, allocating buffer for storing symbol details (%u)\n",
               GetLastError());

        return -1;
    }

    RtlZeroMemory(ModuleDetails, sizeof(SYMBOL_LOADED_MODULE_DETAILS));

    ModuleDetails->ModuleBase = SymLoadModule64(
        GetCurrentProcess(), // Process handle of the current process
        NULL,                // Handle to the module's image file (not needed)
        PdbFileName,         // Path/name of the file
        NULL,                // User-defined short name of the module (it can be NULL)
        BaseAddress,         // Base address of the module (cannot be NULL if .PDB file is
                             // used, otherwise it can be NULL)
        FileSize             // Size of the file (cannot be NULL if .PDB file is used,
                             // otherwise it can be NULL)
    );

    if (ModuleDetails->ModuleBase == NULL)
    {
        printf("err, loading symbols failed (%u)\n",
               GetLastError());

        free(ModuleDetails);
        return -1;
    }

#ifndef DoNotShowDetailedResult

    //
    // Load symbols for the module
    //
    printf("loading symbols for: %s\n", PdbFilePath);

    printf("load address: %I64x\n", ModuleDetails.ModuleBase);

    //
    // Obtain and display information about loaded symbols
    //
    SymShowSymbolInfo(ModuleDetails.ModuleBase);

#endif // !DoNotShowDetailedResult

    //
    // Make the details (to save)
    //
    ModuleDetails->BaseAddress = BaseAddress;
    strcpy((char *)ModuleDetails->ModuleName, ModuleName);

    //
    // Save it
    //
    g_LoadedModules.push_back(ModuleDetails);

    if (!g_IsLoadedModulesInitialized)
    {
        //
        // Indicate that at least one module is loaded
        //
        g_IsLoadedModulesInitialized = TRUE;
    }

    return 0;
}

/**
 * @brief Unload all the symbols
 * 
 * @return UINT32
 */
UINT32
SymUnloadAllSymbols()
{
    BOOL Ret = FALSE;

    for (auto item : g_LoadedModules)
    {
        //
        // Unload symbols for the module
        //
        Ret = SymUnloadModule64(GetCurrentProcess(), item->ModuleBase);

        if (!Ret)
        {
            printf("err, unload symbol failed (%u)\n",
                   GetLastError());
        }

        free(item);
    }

    //
    // Clear the list
    //
    g_LoadedModules.clear();

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
 * @brief Convert function name to address
 *
 * @param FunctionName
 * @param WasFound
 * 
 * @return UINT64
 */
UINT64
SymConvertNameToAddress(const char * FunctionOrVariableName, PBOOLEAN WasFound)
{
    BOOLEAN      Found   = FALSE;
    UINT64       Address = NULL;
    ULONG64      Buffer[(sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(CHAR) + sizeof(ULONG64) - 1) / sizeof(ULONG64)];
    PSYMBOL_INFO Symbol = (PSYMBOL_INFO)Buffer;

    //
    // Not found by default
    //
    *WasFound = FALSE;

    //
    // Retrieve the address from name
    //
    Symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    Symbol->MaxNameLen   = MAX_SYM_NAME;

    //
    // Check if it starts with 'nt!' and if starts then
    // we'll remove it, because 'nt!' is not a real module
    // name
    //
    if (strlen(FunctionOrVariableName) >= 4 &&
        tolower(FunctionOrVariableName[0]) == 'n' &&
        tolower(FunctionOrVariableName[1]) == 't' &&
        tolower(FunctionOrVariableName[2]) == '!')
    {
        //
        // No need to use nt!
        //
        memmove((char *)FunctionOrVariableName, FunctionOrVariableName + 3, strlen(FunctionOrVariableName));
    }

    if (SymFromName(GetCurrentProcess(), FunctionOrVariableName, Symbol))
    {
        //
        // SymFromName returned success
        //
        Found   = TRUE;
        Address = Symbol->Address;
    }
    else
    {
        //
        // SymFromName failed
        //
        Found = FALSE;

        //
        //printf("symbol not found (%u)\n", GetLastError());
        //
    }

    ////////

    *WasFound = Found;
    return Address;
}

/**
 * @brief Search and show symbols 
 * @details mainly used by the 'x' command
 *
 * @param SearchMask
 * 
 * @return UINT32
 */
UINT32
SymSearchSymbolForMask(const char * SearchMask)
{
    BOOL    Ret        = FALSE;
    DWORD64 ModuleBase = NULL;

    //
    // Find module base
    //
    ModuleBase = SymGetModuleBaseFromSearchMask(SearchMask, TRUE);

    //
    // Find the module name
    //
    if (ModuleBase == NULL)
    {
        //
        // Module not found or there was an error
        //
        return -1;
    }

    Ret = SymEnumSymbols(
        GetCurrentProcess(),    // Process handle of the current process
        ModuleBase,             // Base address of the module
        SearchMask,             // Mask (NULL -> all symbols)
        SymEnumSymbolsCallback, // The callback function
        NULL                    // A used-defined context can be passed here, if necessary
    );

    if (!Ret)
    {
        printf("err, symbol enum failed (%u)\n",
               GetLastError());
    }

    return 0;
}

/**
 * @brief add ` between 64 bit values and convert them to string
 *
 * @param Value
 * @return string
 */
string
SymSeparateTo64BitValue(UINT64 Value)
{
    ostringstream OstringStream;
    string        Temp;

    OstringStream << setw(16) << setfill('0') << hex << Value;
    Temp = OstringStream.str();

    Temp.insert(8, 1, '`');
    return Temp;
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
SymGetFileParams(const char * FileName, DWORD & FileSize)
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
        FileSize = 0;
        return FALSE;
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
 * @param ModuleBase
 * 
 * @return VOID
 */
VOID
SymShowSymbolInfo(DWORD64 ModuleBase)
{
    //
    // Get module information
    //
    IMAGEHLP_MODULE64 ModuleInfo;

    memset(&ModuleInfo, 0, sizeof(ModuleInfo));

    ModuleInfo.SizeOfStruct = sizeof(ModuleInfo);

    BOOL Ret = SymGetModuleInfo64(GetCurrentProcess(), ModuleBase, &ModuleInfo);

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
    if (g_CurrentModuleName == NULL)
    {
        //
        // Name Address
        //
        printf("%s ", SymSeparateTo64BitValue(SymInfo.Address).c_str());
    }
    else
    {
        //
        // Module!Name Address
        //
        printf("%s  %s!", SymSeparateTo64BitValue(SymInfo.Address).c_str(), g_CurrentModuleName);
    }

    //
    // Name
    //
    printf("%s\n", SymInfo.Name);

#ifndef DoNotShowDetailedResult

    //
    // Size
    //
    printf(" size: %u", SymInfo.Size);

    //
    // Kind of symbol (tag)
    //
    printf(" symbol: %s  ", SymTagStr(SymInfo.Tag));

#endif // !DoNotShowDetailedResult
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
