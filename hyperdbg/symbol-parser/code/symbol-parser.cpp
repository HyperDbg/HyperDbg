/**
 * @file symbol-parser.cpp
 * @author Alee Amini (alee@hyperdbg.org)
 * @author Sina Karvandi (sina@hyperdbg.org)
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
std::vector<PSYMBOL_LOADED_MODULE_DETAILS> g_LoadedModules;
BOOLEAN                                    g_IsLoadedModulesInitialized = FALSE;
BOOLEAN                                    g_AbortLoadingExecution      = FALSE;
CHAR *                                     g_CurrentModuleName          = NULL;
CHAR                                       g_NtModuleName[_MAX_FNAME]   = {0};
Callback                                   g_MessageHandler             = NULL;
SymbolMapCallback                          g_SymbolMapForDisassembler   = NULL;

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

    //
    // Set the pdbex's message handler
    //
    pdbex_set_logging_method_export(handler);
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

    if (g_MessageHandler == NULL)
    {
        va_start(Args, Fmt);
        vprintf(Fmt, Args);
        va_end(Args);
    }
    else
    {
        char TempMessage[COMMUNICATION_BUFFER_SIZE + TCP_END_OF_BUFFER_CHARS_COUNT] = {0};
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
 * @brief Interpret and find module base, based on module name 
 * @param SearchMask
 * 
 * @return PSYMBOL_LOADED_MODULE_DETAILS NULL means error or not found, 
 * otherwise it returns the instance of loaded module based on search mask
 */
PSYMBOL_LOADED_MODULE_DETAILS
SymGetModuleBaseFromSearchMask(const char * SearchMask, BOOLEAN SetModuleNameGlobally)
{
    string Token;
    char   ModuleName[_MAX_FNAME] = {0};
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
    char Delimiter = '!';
    if (SearchMaskString.find(Delimiter) != std::string::npos)
    {
        //
        // Found
        //
        Token = SearchMaskString.substr(0, SearchMaskString.find(Delimiter));

        strcpy(ModuleName, Token.c_str());

        if (ModuleName[0] == '\0')
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
    // ************* Interpret based on remarks type name *************
    //
    for (auto item : g_LoadedModules)
    {
        if (strcmp((const char *)item->ModuleName, ModuleName) == 0)
        {
            if (SetModuleNameGlobally)
            {
                g_CurrentModuleName = (char *)item->ModuleName;
            }

            return item;
        }
    }

    //
    // If the function continues until here then it means
    // that the module not found
    //
    return NULL;
}

/**
 * @brief Get the offset of a field from the top of a structure 
 * @param Base
 * @param TypeName
 * @param FieldName
 * @param FieldOffset
 * @details This function is derived from: https://github.com/0vercl0k/sic/blob/master/src/sic/sym.cc
 * 
 * @return BOOLEAN Whether the module is found successfully or not
 */
BOOLEAN
SymGetFieldOffsetFromModule(UINT64 Base, WCHAR * TypeName, WCHAR * FieldName, UINT32 * FieldOffset)
{
    BOOLEAN Found = FALSE;

    //
    // Allocate a buffer to back the SYMBOL_INFO structure
    //
    const DWORD SizeOfStruct =
        sizeof(SYMBOL_INFOW) + ((MAX_SYM_NAME - 1) * sizeof(wchar_t));
    uint8_t SymbolInfoBuffer[SizeOfStruct];
    auto    SymbolInfo = PSYMBOL_INFOW(SymbolInfoBuffer);

    //
    // Initialize the fields that need initialization
    //
    SymbolInfo->SizeOfStruct = sizeof(SYMBOL_INFOW);
    SymbolInfo->MaxNameLen   = MAX_SYM_NAME;

    //
    // Retrieve a type index for the type we're after
    //
    if (!SymGetTypeFromNameW(GetCurrentProcess(), Base, TypeName, SymbolInfo))
    {
        // ShowMessages("err, SymGetTypeFromName failed (%x)\n",
        //              GetLastError());
        return FALSE;
    }

    //
    // Now that we have a type, we need to enumerate its children to find the
    // field we're after. First step is to get the number of children
    //
    const ULONG TypeIndex     = SymbolInfo->TypeIndex;
    DWORD       ChildrenCount = 0;
    if (!SymGetTypeInfo(GetCurrentProcess(), Base, TypeIndex, TI_GET_CHILDRENCOUNT, &ChildrenCount))
    {
        // ShowMessages("err, SymGetTypeInfo failed (%x)\n",
        //              GetLastError());
        return FALSE;
    }

    //
    // Allocate enough memory to receive the children ids
    //
    auto FindChildrenParamsBacking = std::make_unique<uint8_t[]>(
        sizeof(_TI_FINDCHILDREN_PARAMS) + ((ChildrenCount - 1) * sizeof(ULONG)));
    auto FindChildrenParams =
        (_TI_FINDCHILDREN_PARAMS *)FindChildrenParamsBacking.get();

    //
    // Initialize the structure with the children count
    //
    FindChildrenParams->Count = ChildrenCount;

    //
    // Get all the children ids
    //
    if (!SymGetTypeInfo(GetCurrentProcess(), Base, TypeIndex, TI_FINDCHILDREN, FindChildrenParams))
    {
        // ShowMessages("err, SymGetTypeInfo failed (%x)\n",
        //             GetLastError());
        return FALSE;
    }

    //
    // Now that we have all the ids, we can walk them and find the one that
    // matches the field we're looking for
    //

    for (DWORD ChildIdx = 0; ChildIdx < ChildrenCount; ChildIdx++)
    {
        //
        // Grab the child name
        //
        const ULONG ChildId   = FindChildrenParams->ChildId[ChildIdx];
        WCHAR *     ChildName = nullptr;
        SymGetTypeInfo(GetCurrentProcess(), Base, ChildId, TI_GET_SYMNAME, &ChildName);

        //
        // Grab the child size - this is useful to know if a field is a bit or a
        // normal field
        //
        UINT64 ChildSize = 0;
        SymGetTypeInfo(GetCurrentProcess(), Base, ChildId, TI_GET_LENGTH, &ChildSize);

        //
        // Does this child's name match the field we're looking for?
        //
        Found = FALSE;
        if (wcscmp(ChildName, FieldName) == 0)
        {
            //
            // If we have found the field, now we need to find its offset if
            // it's a normal field, or its bit position if it is a bit
            //
            const IMAGEHLP_SYMBOL_TYPE_INFO Info =
                (ChildSize == 1) ? TI_GET_BITPOSITION : TI_GET_OFFSET;
            SymGetTypeInfo(GetCurrentProcess(), Base, ChildId, Info, FieldOffset);

            Found = TRUE;
        }

        //
        // Even if we have found a match, we need to clean up the memory
        //
        LocalFree(ChildName);
        ChildName = nullptr;

        //
        // We can now break out of the loop if we have what we came looking for
        //
        if (Found)
        {
            break;
        }
    }

    return Found;
}

/**
 * @brief Get the size of a data type (structure)
 * @param Base
 * @param TypeName
 * @param TypeSize
 * 
 * @return BOOLEAN Whether the module is found successfully or not
 */
BOOLEAN
SymGetDataTypeSizeFromModule(UINT64 Base, WCHAR * TypeName, UINT64 * TypeSize)
{
    //
    // Allocate a buffer to back the SYMBOL_INFO structure
    //
    const DWORD SizeOfStruct =
        sizeof(SYMBOL_INFOW) + ((MAX_SYM_NAME - 1) * sizeof(wchar_t));
    uint8_t SymbolInfoBuffer[SizeOfStruct];
    auto    SymbolInfo = PSYMBOL_INFOW(SymbolInfoBuffer);

    //
    // Initialize the fields that need initialization
    //
    SymbolInfo->SizeOfStruct = sizeof(SYMBOL_INFOW);
    SymbolInfo->MaxNameLen   = MAX_SYM_NAME;

    //
    // Retrieve a type index for the type we're after
    //
    if (!SymGetTypeFromNameW(GetCurrentProcess(), Base, TypeName, SymbolInfo))
    {
        // ShowMessages("err, SymGetTypeFromName failed (%x)\n",
        //              GetLastError());
        return FALSE;
    }

    if (!SymGetTypeInfo(GetCurrentProcess(), Base, SymbolInfo->TypeIndex, TI_GET_LENGTH, TypeSize))
    {
        // ShowMessages("err, SymGetTypeInfo failed (%x)\n",
        //              GetLastError());
        return FALSE;
    }

    // ShowMessages("type size : %llx\n", TypeSize);

    return TRUE;
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
        ShowMessages("err, symbol init failed (%x)\n",
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
        ShowMessages("err, allocating buffer for storing symbol details (%x)\n",
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
        //
        //ShowMessages("err, loading symbols failed (%x)\n",
        //       GetLastError());
        //

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
    strcpy((char *)ModuleDetails->PdbFilePath, PdbFileName);

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
                ShowMessages("err, unload symbol failed (%x)\n",
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
            ShowMessages("err, unload symbol failed (%x)\n",
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
        ShowMessages("err, symbol cleanup failed (%x)\n", GetLastError());
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
    UINT64       Buffer[(sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(CHAR) + sizeof(UINT64) - 1) / sizeof(UINT64)];
    PSYMBOL_INFO Symbol = (PSYMBOL_INFO)Buffer;
    string       name   = FunctionOrVariableName;

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
    // we'll change it to nt the kernel module 'ntkrnlmp' 
    // because 'nt!' is not a real module name
    //
    if (strlen(FunctionOrVariableName) >= 4 &&
        tolower(FunctionOrVariableName[0]) == 'n' &&
        tolower(FunctionOrVariableName[1]) == 't' &&
        tolower(FunctionOrVariableName[2]) == '!')
    {
        FunctionOrVariableName += 3;
        name = "ntkrnlmp!";
        name += FunctionOrVariableName;
    }

    if (SymFromName(GetCurrentProcess(), name.c_str(), Symbol))
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
        //ShowMessages("symbol not found (%x)\n", GetLastError());
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
 * @param TypeName
 * @param FieldName
 * @param FieldOffset
 * 
 * @return BOOLEAN Whether the module is found successfully or not
 */
BOOLEAN
SymGetFieldOffset(CHAR * TypeName, CHAR * FieldName, UINT32 * FieldOffset)
{
    BOOL                          Ret        = FALSE;
    UINT32                        Index      = 0;
    PSYMBOL_LOADED_MODULE_DETAILS SymbolInfo = NULL;
    BOOLEAN                       Result     = FALSE;

    //
    // Find module info
    //
    SymbolInfo = SymGetModuleBaseFromSearchMask(TypeName, TRUE);

    //
    // Check if module is found
    //
    if (SymbolInfo == NULL)
    {
        //
        // Module not found or there was an error
        //
        return FALSE;
    }

    //
    // Remove the *!Name from TypeName as it not supports module name
    // at the beginning of a type name
    //
    while (TypeName[Index] != '\0')
    {
        if (TypeName[Index] == '!')
        {
            TypeName = (CHAR *)(TypeName + Index + 1);
            break;
        }

        Index++;
    }

    //
    // Convert TypeName to wide-char, it's because SymGetTypeInfo supports
    // wide-char
    //
    const size_t TypeNameSize = strlen(TypeName) + 1;
    WCHAR *      TypeNameW    = (WCHAR *)malloc(sizeof(wchar_t) * TypeNameSize);
    RtlZeroMemory(TypeNameW, sizeof(wchar_t) * TypeNameSize);
    mbstowcs(TypeNameW, TypeName, TypeNameSize);

    //
    // Convert FieldName to wide-char, it's because SymGetTypeInfo supports
    // wide-char
    //
    const size_t FieldNameSize = strlen(FieldName) + 1;
    WCHAR *      FieldNameW    = (WCHAR *)malloc(sizeof(wchar_t) * FieldNameSize);
    RtlZeroMemory(FieldNameW, sizeof(wchar_t) * FieldNameSize);
    mbstowcs(FieldNameW, FieldName, FieldNameSize);

    Result = SymGetFieldOffsetFromModule(SymbolInfo->ModuleBase, TypeNameW, FieldNameW, FieldOffset);

    free(TypeNameW);
    free(FieldNameW);

    return Result;
}

/**
 * @brief Get the size of structures from the symbols 
 *
 * @param TypeName
 * @param FieldName
 * @param FieldOffset
 * 
 * @return BOOLEAN Whether the module is found successfully or not
 */
BOOLEAN
SymGetDataTypeSize(CHAR * TypeName, UINT64 * TypeSize)
{
    BOOL                          Ret        = FALSE;
    UINT32                        Index      = 0;
    PSYMBOL_LOADED_MODULE_DETAILS SymbolInfo = NULL;
    BOOLEAN                       Result     = FALSE;

    //
    // Find module info
    //
    SymbolInfo = SymGetModuleBaseFromSearchMask(TypeName, TRUE);

    //
    // Check if module is found
    //
    if (SymbolInfo == NULL)
    {
        //
        // Module not found or there was an error
        //
        return FALSE;
    }

    //
    // Remove the *!Name from TypeName as it not supports module name
    // at the beginning of a type name
    //
    while (TypeName[Index] != '\0')
    {
        if (TypeName[Index] == '!')
        {
            TypeName = (CHAR *)(TypeName + Index + 1);
            break;
        }

        Index++;
    }

    //
    // Convert FieldName to wide-char, it's because SymGetTypeInfo supports
    // wide-char
    //
    const size_t TypeNameSize = strlen(TypeName) + 1;
    WCHAR *      TypeNameW    = (WCHAR *)malloc(sizeof(wchar_t) * TypeNameSize);
    RtlZeroMemory(TypeNameW, sizeof(wchar_t) * TypeNameSize);
    mbstowcs(TypeNameW, TypeName, TypeNameSize);

    Result = SymGetDataTypeSizeFromModule(SymbolInfo->ModuleBase, TypeNameW, TypeSize);

    free(TypeNameW);

    return Result;
}

/**
 * @brief Gets the offset from the symbol 
 *
 * @param SearchMask
 * 
 * @return UINT32
 */
UINT32
SymSearchSymbolForMask(const char * SearchMask)
{
    BOOL                          Ret        = FALSE;
    PSYMBOL_LOADED_MODULE_DETAILS SymbolInfo = NULL;

    //
    // Get the module info
    //
    SymbolInfo = SymGetModuleBaseFromSearchMask(SearchMask, TRUE);

    //
    // Check to see if module info is found
    //
    if (SymbolInfo == NULL)
    {
        //
        // Module not found or there was an error
        //
        return -1;
    }

    Ret = SymEnumSymbols(
        GetCurrentProcess(),           // Process handle of the current process
        SymbolInfo->ModuleBase,        // Base address of the module
        SearchMask,                    // Mask (NULL -> all symbols)
        SymDisplayMaskSymbolsCallback, // The callback function
        NULL                           // A used-defined context can be passed here, if necessary
    );

    if (!Ret)
    {
        ShowMessages("err, symbol enum failed (%x)\n",
                     GetLastError());
    }

    return 0;
}

/**
 * @brief Create symbol table for disassembler 
 * @details mainly used by disassembler for 'u' command
 *
 * @param CallbackFunction
 * 
 * @return BOOLEAN
 */
BOOLEAN
SymCreateSymbolTableForDisassembler(void * CallbackFunction)
{
    BOOL    Ret    = FALSE;
    BOOLEAN Result = TRUE;

    //
    // Set the callback function to deliver the name of module!ObjectName
    //
    g_SymbolMapForDisassembler = (SymbolMapCallback)CallbackFunction;

    //
    // Create a symbol table from all modules
    //
    for (auto item : g_LoadedModules)
    {
        //
        // Set module name
        //
        g_CurrentModuleName = (char *)item->ModuleName;

        //
        // Call the callback for the current module
        //
        Ret = SymEnumSymbols(
            GetCurrentProcess(),                     // Process handle of the current process
            item->BaseAddress,                       // Base address of the module
            NULL,                                    // Mask (NULL -> all symbols)
            SymDeliverDisassemblerSymbolMapCallback, // The callback function
            NULL                                     // A used-defined context can be passed here, if necessary
        );

        if (!Ret)
        {
            //
            // A module did not added correctly
            //
            //  ShowMessages("err, symbol enum failed (%x)\n", GetLastError());
            Result = FALSE;
        }
    }

    return Result;
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
        ShowMessages("err, unable to open symbol file (%x)\n", GetLastError());
        return FALSE;
    }

    //
    // Obtain the size of the file
    //
    FileSize = GetFileSize(hFile, NULL);

    if (FileSize == INVALID_FILE_SIZE)
    {
        ShowMessages("err, unable to get symbol file size (%x)\n", GetLastError());

        //
        // and continue ...
        //
    }

    //
    // Close the file
    //
    if (!CloseHandle(hFile))
    {
        ShowMessages("err, unable to close symbol file (%x)\n", GetLastError());

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
SymShowSymbolInfo(UINT64 ModuleBase)
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
        ShowMessages("err, unable to get symbol file information (%x)\n",
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
    if (ModuleInfo.ImageName[0] != '\0')
    {
        ShowMessages("image name: %s\n", ModuleInfo.ImageName);
    }

    //
    // Loaded image name
    //
    if (ModuleInfo.LoadedImageName[0] != '\0')
    {
        ShowMessages("loaded image name: %s\n", ModuleInfo.LoadedImageName);
    }

    //
    // Loaded PDB name
    //
    if (ModuleInfo.LoadedPdbName[0] != '\0')
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
 * @brief Callback for delivering module!ObjectName to disassembler symbol map
 *
 * @param SymInfo
 * @param SymbolSize
 * @param UserContext
 * 
 * @return BOOL
 */
BOOL CALLBACK
SymDeliverDisassemblerSymbolMapCallback(SYMBOL_INFO * SymInfo, ULONG SymbolSize, PVOID UserContext)
{
    if (SymInfo != 0 && g_SymbolMapForDisassembler != NULL)
    {
        //
        // Call the remote callback
        //
        g_SymbolMapForDisassembler(SymInfo->Address, g_CurrentModuleName, SymInfo->Name, SymInfo->Size);
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

    Tmp = SymDir = SplitedsymPath[1];

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
SymbolPDBDownload(std::string SymName, const std::string & GUID, const std::string & SymPath, BOOLEAN IsSilentLoad)
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
            ShowMessages("err, unable to create sympath directory '%s'\n", SymFullDir.c_str());
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

/**
 * @brief Perform task for showing structures and data
 * @details used by dt command
 *
 * @param TypeName
 * @param Address
 * @param IsStruct
 * @param BufferAddress
 * @param AdditionalParameters
 * 
 * @return BOOLEAN
 */
BOOLEAN
SymShowDataBasedOnSymbolTypes(const char * TypeName,
                              UINT64       Address,
                              BOOLEAN      IsStruct,
                              PVOID        BufferAddress,
                              const char * AdditionalParameters)
{
    vector<string>                SplitedsymPath;
    char **                       ArgvArray     = NULL;
    PSYMBOL_LOADED_MODULE_DETAILS SymbolInfo    = NULL;
    UINT32                        SizeOfArgv    = 0;
    UINT32                        TypeNameIndex = 0;

    //
    // Find the symbol info (to get the PDB address)
    //
    SymbolInfo = SymGetModuleBaseFromSearchMask(TypeName, FALSE);

    if (!SymbolInfo)
    {
        //
        // Symbol not found
        //
        ShowMessages("err, couldn't resolve error at '%s'\n", TypeName);

        return FALSE;
    }

    //
    // Convert char* to string
    //
    std::string AdditionalParametersString(AdditionalParameters);

    //
    // Split the arguments by space
    //
    SplitedsymPath = Split(AdditionalParametersString, ' ');

    //
    // Allocate buffer to convert it to the char*
    // + 3 is because of
    //      1. file name
    //      2. type (structure) name
    //      3. PDB file location
    //
    SizeOfArgv = SplitedsymPath.size() + 3;
    ArgvArray  = (char **)malloc(SizeOfArgv * sizeof(char *));

    if (ArgvArray == NULL)
    {
        return FALSE;
    }

    RtlZeroMemory(ArgvArray, SizeOfArgv * sizeof(char *));

    //
    // First argument is the file name, we let it blank
    //
    ArgvArray[0] = (char *)NULL;

    //
    // Remove the module name (if any)
    //
    while (TypeName[TypeNameIndex] != NULL)
    {
        if (TypeName[TypeNameIndex] == '!')
        {
            TypeName = &TypeName[++TypeNameIndex];
            break;
        }

        TypeNameIndex++;
    }

    //
    // Second argument is the type (structure) name
    //
    ArgvArray[1] = (char *)TypeName;

    //
    // Third argument is the PDB file location
    //
    ArgvArray[2] = SymbolInfo->PdbFilePath;

    //
    // Fill the parameter with char array
    //
    for (size_t i = 3; i < SizeOfArgv; i++)
    {
        ArgvArray[i] = (char *)SplitedsymPath.at(i - 3).c_str();
    }

    //
    // Call the pdbex wrapper
    //
    if (IsStruct)
    {
        pdbex_export(SizeOfArgv, ArgvArray, true, BufferAddress);
    }
    else
    {
        pdbex_export(SizeOfArgv, ArgvArray, false, BufferAddress);
    }

    //
    // Free the buffer allocated for argv
    //
    free(ArgvArray);

    return TRUE;
}
