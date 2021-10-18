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
#include "..\symbol-parser\pch.h"

//
// Global Variables
//
std::vector<PSYMBOL_LOADED_MODULE_DETAILS> g_LoadedModules;
BOOLEAN                                    g_IsLoadedModulesInitialized = FALSE;
BOOLEAN                                    g_AbortLoadingExecution      = FALSE;
CHAR *                                     g_CurrentModuleName          = NULL;
CHAR                                       g_NtModuleName[_MAX_FNAME]   = {0};
Callback                                   g_MessageHandler             = NULL;

/**
 * @brief Set the function callback that will be called if any message
 * needs to be shown
 *
 * @param handler Function that handles the messages
 */
void
SymSetTextMessageCallback(PVOID handler)
{
    g_MessageHandler = (Callback)handler;
}

/**
 * @brief Show messages
 *
 * @param Fmt format string message
 */
VOID
ShowMessages(const char * Fmt, ...)
{
    va_list ArgList;
    va_list Args;
    char    TempMessage[COMMUNICATION_BUFFER_SIZE + TCP_END_OF_BUFFER_CHARS_COUNT] = {0};

    if (g_MessageHandler == NULL)
    {
        va_start(Args, Fmt);
        vprintf(Fmt, Args);
        va_end(Args);
    }
    else
    {
        va_start(ArgList, Fmt);
        int sprintfresult = vsprintf_s(TempMessage, Fmt, ArgList);
        va_end(ArgList);

        if (sprintfresult != -1)
        {
            //
            // There is another handler
            //
            g_MessageHandler(TempMessage);
        }
    }
}

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
        ShowMessages("err, symbol init failed (%u)\n",
                     GetLastError());
        return -1;
    }

    //
    // Determine the base address and the file size
    //
    if (!SymGetFileParams(PdbFileName, FileSize))
    {
        ShowMessages("err, cannot obtain file parameters (internal error)\n");
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
        ShowMessages("err, allocating buffer for storing symbol details (%u)\n",
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
        //ShowMessages("err, loading symbols failed (%u)\n",
        //       GetLastError());

        free(ModuleDetails);
        return -1;
    }

#ifndef DoNotShowDetailedResult

    //
    // Load symbols for the module
    //
    ShowMessages("loading symbols for: %s\n", PdbFilePath);

    ShowMessages("load address: %I64x\n", ModuleDetails.ModuleBase);

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
 * @brief Unload one module symbol
 * 
 * @param ModuleName 
 * 
 * @return UINT32
 */
UINT32
SymUnloadModuleSymbol(char * ModuleName)
{
    BOOLEAN OneModuleFound = FALSE;
    BOOL    Ret            = FALSE;
    UINT32  Index          = 0;

    for (auto item : g_LoadedModules)
    {
        Index++;
        if (strcmp(item->ModuleName, ModuleName) == 0)
        {
            //
            // Unload symbol for the module
            //
            Ret = SymUnloadModule64(GetCurrentProcess(), item->ModuleBase);

            if (!Ret)
            {
                ShowMessages("err, unload symbol failed (%u)\n",
                             GetLastError());
                return -1;
            }

            OneModuleFound = TRUE;

            free(item);

            break;
        }
    }

    if (!OneModuleFound)
    {
        //
        // Not found
        //
        return -1;
    }

    //
    // Remove it from the vector
    //
    std::vector<PSYMBOL_LOADED_MODULE_DETAILS>::iterator it = g_LoadedModules.begin();
    std::advance(it, --Index);
    g_LoadedModules.erase(it);

    //
    // Success
    //
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
            ShowMessages("err, unload symbol failed (%u)\n",
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
        ShowMessages("err, symbol cleanup failed (%u)\n", GetLastError());
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
        //ShowMessages("symbol not found (%u)\n", GetLastError());
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
 * @param UpdateSymbolTable
 * 
 * @return UINT32
 */
UINT32
SymSearchSymbolForMask(const char * SearchMask, BOOLEAN UpdateSymbolTable)
{
    BOOL    Ret        = FALSE;
    DWORD64 ModuleBase = NULL;

    if (!UpdateSymbolTable)
    {
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
            GetCurrentProcess(),           // Process handle of the current process
            ModuleBase,                    // Base address of the module
            SearchMask,                    // Mask (NULL -> all symbols)
            SymDisplayMaskSymbolsCallback, // The callback function
            NULL                           // A used-defined context can be passed here, if necessary
        );

        if (!Ret)
        {
            ShowMessages("err, symbol enum failed (%u)\n",
                         GetLastError());
        }
    }
    else
    {
        //
        // Create a symbol table from all modules
        //
        for (auto item : g_LoadedModules)
        {
            Ret = SymEnumSymbols(
                GetCurrentProcess(),    // Process handle of the current process
                item->BaseAddress,      // Base address of the module
                NULL,                   // Mask (NULL -> all symbols)
                SymEnumSymbolsCallback, // The callback function
                NULL                    // A used-defined context can be passed here, if necessary
            );

            if (!Ret)
            {
                //  ShowMessages("err, symbol enum failed (%u)\n", GetLastError());
            }
        }
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
        ShowMessages("err, unable to open symbol file (%u)\n", GetLastError());
        return FALSE;
    }

    //
    // Obtain the size of the file
    //
    FileSize = GetFileSize(hFile, NULL);

    if (FileSize == INVALID_FILE_SIZE)
    {
        ShowMessages("err, unable to get symbol file size (%u)\n", GetLastError());

        //
        // and continue ...
        //
    }

    //
    // Close the file
    //
    if (!CloseHandle(hFile))
    {
        ShowMessages("err, unable to close symbol file (%u)\n", GetLastError());

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
        ShowMessages("err, unable to get symbol file information (%u)\n",
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
        ShowMessages("no symbols available for the module\n");
        break;

    case SymExport:
        ShowMessages("loaded symbols: Exports\n");
        break;

    case SymCoff:
        ShowMessages("loaded symbols: COFF\n");
        break;

    case SymCv:
        ShowMessages("loaded symbols: CodeView\n");
        break;

    case SymSym:
        ShowMessages("loaded symbols: SYM\n");
        break;

    case SymVirtual:
        ShowMessages("loaded symbols: Virtual\n");
        break;

    case SymPdb:
        ShowMessages("loaded symbols: PDB\n");
        break;

    case SymDia:
        ShowMessages("loaded symbols: DIA\n");
        break;

    case SymDeferred:

        //
        // not actually loaded
        //
        ShowMessages("loaded symbols: Deferred\n");
        break;

    default:
        ShowMessages("loaded symbols: Unknown format\n");
        break;
    }

    //
    // Image name
    //
    if (strlen(ModuleInfo.ImageName) > 0)
    {
        ShowMessages("image name: %s\n", ModuleInfo.ImageName);
    }

    //
    // Loaded image name
    //
    if (strlen(ModuleInfo.LoadedImageName) > 0)
    {
        ShowMessages("loaded image name: %s\n", ModuleInfo.LoadedImageName);
    }

    //
    // Loaded PDB name
    //
    if (strlen(ModuleInfo.LoadedPdbName) > 0)
    {
        ShowMessages("PDB file name: %s\n", ModuleInfo.LoadedPdbName);
    }

    //
    // Is debug information unmatched?
    // (It can only happen if the debug information is contained
    // in a separate file (.DBG or .PDB)
    //
    if (ModuleInfo.PdbUnmatched || ModuleInfo.DbgUnmatched)
    {
        ShowMessages("warning, unmatched symbols\n");
    }

    //
    // *** Contents ***
    //

    //
    // Line numbers available?
    //
    ShowMessages("line numbers: %s\n",
                 ModuleInfo.LineNumbers ? "available" : "not available");

    //
    // Global symbols available?
    //
    ShowMessages("global symbols: %s\n",
                 ModuleInfo.GlobalSymbols ? "available" : "not available");

    //
    // Type information available?
    //
    ShowMessages("type information: %s\n",
                 ModuleInfo.TypeInfo ? ("Available") : ("Not available"));

    //
    // Source indexing available?
    //
    ShowMessages("source indexing: %s\n",
                 ModuleInfo.SourceIndexed ? "yes" : "no");

    //
    // Public symbols available?
    //
    ShowMessages("public symbols: %s\n",
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
SymDisplayMaskSymbolsCallback(SYMBOL_INFO * SymInfo, ULONG SymbolSize, PVOID UserContext)
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
    // SymInfo->Address
    // SymInfo->Name

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
        ShowMessages("%s ", SymSeparateTo64BitValue(SymInfo.Address).c_str());
    }
    else
    {
        //
        // Module!Name Address
        //
        ShowMessages("%s  %s!", SymSeparateTo64BitValue(SymInfo.Address).c_str(), g_CurrentModuleName);
    }

    //
    // Name
    //
    ShowMessages("%s\n", SymInfo.Name);

#ifndef DoNotShowDetailedResult

    //
    // Size
    //
    ShowMessages(" size: %u", SymInfo.Size);

    //
    // Kind of symbol (tag)
    //
    ShowMessages(" symbol: %s  ", SymTagStr(SymInfo.Tag));

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

/**
 * @brief Convert a DLL to a Microsoft Symbol path
 *
 * @param LocalFilePath
 * @param ResultPath
 * 
 * @return BOOLEAN
 */
BOOLEAN
SymConvertFileToPdbPath(const char * LocalFilePath, char * ResultPath)
{
    SYMSRV_INDEX_INFO SymInfo = {0};
    const char *      FormatStr =
        "%s/%08x%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x%x/%s";
    SymInfo.sizeofstruct = sizeof(SYMSRV_INDEX_INFO);

    BOOL Ret = SymSrvGetFileIndexInfo(LocalFilePath, &SymInfo, 0);

    if (Ret)
    {
        wsprintfA(ResultPath,
                  FormatStr,
                  SymInfo.pdbfile,
                  SymInfo.guid.Data1,
                  SymInfo.guid.Data2,
                  SymInfo.guid.Data3,
                  SymInfo.guid.Data4[0],
                  SymInfo.guid.Data4[1],
                  SymInfo.guid.Data4[2],
                  SymInfo.guid.Data4[3],
                  SymInfo.guid.Data4[4],
                  SymInfo.guid.Data4[5],
                  SymInfo.guid.Data4[6],
                  SymInfo.guid.Data4[7],
                  SymInfo.age,
                  SymInfo.pdbfile);

        return TRUE;
    }
    else
    {
        //
        // ShowMessages("err, unable to get symbol information for %s (%x)\n", LocalFilePath, GetLastError());
        //
        return FALSE;
    }

    //
    // By default, return false
    //
    return FALSE;
}

/**
 * @brief Convert a DLL to a Microsoft Symbol details
 * like pdb file path and GUID
 *
 * @param LocalFilePath
 * @param PdbFilePath
 * @param GuidAndAgeDetails
 * 
 * @return BOOLEAN
 */
BOOLEAN
SymConvertFileToPdbFileAndGuidAndAgeDetails(const char * LocalFilePath, char * PdbFilePath, char * GuidAndAgeDetails)
{
    SYMSRV_INDEX_INFO SymInfo              = {0};
    const char *      FormatStrPdbFilePath = "%s";
    const char *      FormatStrPdbFileGuidAndAgeDetails =
        "%08x%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x%x";
    SymInfo.sizeofstruct = sizeof(SYMSRV_INDEX_INFO);

    BOOL Ret = SymSrvGetFileIndexInfo(LocalFilePath, &SymInfo, 0);

    if (Ret)
    {
        wsprintfA(PdbFilePath, FormatStrPdbFilePath, SymInfo.pdbfile);

        wsprintfA(GuidAndAgeDetails,
                  FormatStrPdbFileGuidAndAgeDetails,
                  SymInfo.guid.Data1,
                  SymInfo.guid.Data2,
                  SymInfo.guid.Data3,
                  SymInfo.guid.Data4[0],
                  SymInfo.guid.Data4[1],
                  SymInfo.guid.Data4[2],
                  SymInfo.guid.Data4[3],
                  SymInfo.guid.Data4[4],
                  SymInfo.guid.Data4[5],
                  SymInfo.guid.Data4[6],
                  SymInfo.guid.Data4[7],
                  SymInfo.age);

        return TRUE;
    }
    else
    {
        //
        // ShowMessages("err, unable to get symbol information for %s (%x)\n", LocalFilePath, GetLastError());
        //
        return FALSE;
    }

    //
    // By default, return false
    //
    return FALSE;
}

/**
 * @brief check if the pdb files of loaded symbols are available or not
 * 
 * @param BufferToStoreDetails Pointer to a buffer to store the symbols details
 * this buffer will be allocated by this function and needs to be freed by caller
 * @param StoredLength The length that stored on the BufferToStoreDetails
 * @param DownloadIfAvailable Download the file if its available online
 * @param SymbolPath The path of symbols
 * @param IsSilentLoad
 * 
 * @return BOOLEAN
 */
BOOLEAN
SymbolInitLoad(PVOID        BufferToStoreDetails,
               UINT32       StoredLength,
               BOOLEAN      DownloadIfAvailable,
               const char * SymbolPath,
               BOOLEAN      IsSilentLoad)
{
    string                Tmp, SymDir;
    string                SymPath(SymbolPath);
    PMODULE_SYMBOL_DETAIL BufferToStoreDetailsConverted = (PMODULE_SYMBOL_DETAIL)BufferToStoreDetails;

    vector<string> SplitedsymPath = Split(SymPath, '*');
    if (SplitedsymPath.size() < 2)
        return FALSE;
    if (SplitedsymPath[1].find(":\\") == string::npos)
        return FALSE;

    SymDir = SplitedsymPath[1];

    //
    // Split each module and details
    //
    for (size_t i = 0; i < StoredLength / sizeof(MODULE_SYMBOL_DETAIL); i++)
    {
        //
        // Check for abort
        //
        if (g_AbortLoadingExecution)
        {
            g_AbortLoadingExecution = FALSE;
            return FALSE;
        }
        //
        // Check if symbol pdb detail is available in the module
        //
        if (!BufferToStoreDetailsConverted[i].IsSymbolDetailsFound)
        {
            //
            // Ignore the module
            //
            continue;
        }

        //
        // Check if it's a local path (a path) or a microsoft symbol
        //
        if (BufferToStoreDetailsConverted[i].IsLocalSymbolPath)
        {
            //
            // If this is a local driver, then load the pdb
            //
            if (IsFileExists(BufferToStoreDetailsConverted[i].ModuleSymbolPath))
            {
                BufferToStoreDetailsConverted[i].IsSymbolPDBAvaliable = TRUE;

                //
                // Load symbol locally
                //
                if (!IsSilentLoad)
                {
                    ShowMessages("loading symbol '%s'...", Tmp.c_str());
                }

                if (SymLoadFileSymbol(BufferToStoreDetailsConverted[i].BaseAddress,
                                      BufferToStoreDetailsConverted[i].ModuleSymbolPath) == 0)
                {
                    if (!IsSilentLoad)
                    {
                        ShowMessages("\tloaded\n");
                    }
                }
                else
                {
                    if (!IsSilentLoad)
                    {
                        ShowMessages("\tcould not be loaded\n");
                    }
                }
            }
        }
        else
        {
            //
            // It might be a Windows symbol
            //
            Tmp = SymDir +
                  "\\" +
                  BufferToStoreDetailsConverted[i].ModuleSymbolPath +
                  "\\" +
                  BufferToStoreDetailsConverted[i].ModuleSymbolGuidAndAge +
                  "\\" +
                  BufferToStoreDetailsConverted[i].ModuleSymbolPath;

            //
            // Check if the symbol already download or not
            //
            if (IsFileExists(Tmp))
            {
                BufferToStoreDetailsConverted[i].IsSymbolPDBAvaliable = TRUE;

                if (!IsSilentLoad)
                {
                    ShowMessages("loading symbol '%s'...", Tmp.c_str());
                }

                if (SymLoadFileSymbol(BufferToStoreDetailsConverted[i].BaseAddress, Tmp.c_str()) == 0)
                {
                    if (!IsSilentLoad)
                    {
                        ShowMessages("\tloaded\n");
                    }
                }
                else
                {
                    if (!IsSilentLoad)
                    {
                        ShowMessages("\tcould not be loaded\n");
                    }
                }
            }
            else
            {
                if (DownloadIfAvailable)
                {
                    //
                    // Download the symbol
                    //
                    SymbolPDBDownload(BufferToStoreDetailsConverted[i].ModuleSymbolPath,
                                      BufferToStoreDetailsConverted[i].ModuleSymbolGuidAndAge,
                                      SymPath,
                                      IsSilentLoad);
                }
            }
        }
    }

    return TRUE;
}

/**
 * @brief download pdb file 
 * 
 * @param BufferToStoreDetails Pointer to a buffer to store the symbols details
 * this buffer will be allocated by this function and needs to be freed by caller
 * @param StoredLength The length that stored on the BufferToStoreDetails
 * @param SymPath The path of symbols
 * @param IsSilentLoad Download without any message
 * 
 * return BOOLEAN
 */
BOOLEAN
SymbolPDBDownload(std::string SymName, std::string GUID, std::string SymPath, BOOLEAN IsSilentLoad)
{
    vector<string> SplitedsymPath = Split(SymPath, '*');
    if (SplitedsymPath.size() < 2)
        return FALSE;
    if (SplitedsymPath[1].find(":\\") == string::npos)
        return FALSE;
    if (SplitedsymPath[2].find("http:") == string::npos && SplitedsymPath[2].find("https:") == string::npos)
        return FALSE;

    string SymDir            = SplitedsymPath[1];
    string SymDownloadServer = SplitedsymPath[2];
    string DownloadURL       = SymDownloadServer + "/" + SymName + "/" + GUID + "/" + SymName;
    string SymFullDir        = SymDir + "\\" + SymName + "\\" + GUID + "\\";
    if (!CreateDirectoryRecursive(SymFullDir))
    {
        if (!IsSilentLoad)
        {
            ShowMessages("err, unable to create sympath directory '%s'\n", SymFullDir);
        }
        return FALSE;
    }

    if (!IsSilentLoad)
    {
        ShowMessages("downloading symbol '%s'...", SymName.c_str());
    }

    HRESULT Result = URLDownloadToFileA(NULL, DownloadURL.c_str(), (SymFullDir + "\\" + SymName).c_str(), 0, NULL);

    if (Result == S_OK)
    {
        if (!IsSilentLoad)
        {
            ShowMessages("\tdownloaded\n");
        }
        return TRUE;
    }
    else
    {
        if (!IsSilentLoad)
        {
            ShowMessages("\tcould not be downloaded (%x) \n", Result);
        }
    }

    return FALSE;
}

/**
 * @brief In the case of pressing CTRL+C, it sets a flag
 * to abort the execution of 'reload'ing and 'download'ing
 *  
 * return VOID
 */
VOID
SymbolAbortLoading()
{
    if (!g_AbortLoadingExecution)
    {
        g_AbortLoadingExecution = TRUE;
        ShowMessages("\naborting, please wait...\n");
    }
}
