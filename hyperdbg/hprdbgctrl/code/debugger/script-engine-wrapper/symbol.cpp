/**
 * @file symbol.cpp
 * @author Alee Amini (alee@hyperdbg.org)
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief symbol parser
 * @details
 * @version 0.1
 * @date 2021-05-20
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern PMODULE_SYMBOL_DETAIL                        g_SymbolTable;
extern UINT32                                       g_SymbolTableSize;
extern UINT32                                       g_SymbolTableCurrentIndex;
extern BOOLEAN                                      g_IsExecutingSymbolLoadingRoutines;
extern BOOLEAN                                      g_IsSerialConnectedToRemoteDebugger;
extern BOOLEAN                                      g_AddressConversion;
extern std::map<UINT64, LOCAL_FUNCTION_DESCRIPTION> g_DisassemblerSymbolMap;

using namespace std;

/**
 * @brief Initial load of symbols (for previously download symbols)
 * 
 * @return VOID 
 */
VOID
SymbolInitialReload()
{
    //
    // Load already downloaded symbol (won't download at this point)
    //
    ShowMessages("interpreting symbols and creating symbol maps\n");
    SymbolLoadOrDownloadSymbols(FALSE, TRUE);
}

/**
 * @brief Locally reload the symbol table
 * @param UserProcessId
 * 
 * @return BOOLEAN 
 */
BOOLEAN
SymbolLocalReload(UINT32 UserProcessId)
{
    ShowMessages("interpreting symbols and creating symbol maps\n");

    SymbolBuildSymbolTable(&g_SymbolTable, &g_SymbolTableSize, UserProcessId, FALSE);

    //
    // And also load the symbols
    //
    return SymbolLoadOrDownloadSymbols(FALSE, TRUE);
}

/**
 * @brief Initial and send the results of serial for the debugger
 * in the case of debugger mode
 * @param UserProcessId
 * 
 * @return VOID 
 */
VOID
SymbolPrepareDebuggerWithSymbolInfo(UINT32 UserProcessId)
{
    //
    // Load already downloaded symbol (won't download at this point)
    //
    SymbolBuildSymbolTable(&g_SymbolTable, &g_SymbolTableSize, UserProcessId, TRUE);
}

/**
 * @brief Callback for creating symbol map for disassembler
 *
 * @param Address
 * @param ModuleName
 * @param ObjectName
 * @param ObjectSize
 * 
 * @return VOID
 */
VOID
SymbolCreateDisassemblerMapCallback(UINT64       Address,
                                    char *       ModuleName,
                                    char *       ObjectName,
                                    unsigned int ObjectSize)
{
    //
    // It has a string, should not be initialized with zero
    //
    LOCAL_FUNCTION_DESCRIPTION LocalFunctionDescription = {};
    string                     FinalModuleName          = "";

    if (ObjectSize == 0)
    {
        ObjectSize = DISASSEMBLY_MAXIMUM_DISTANCE_FROM_OBJECT_NAME;
    }

    //
    // Convert module name to string
    //
    if (ModuleName != NULL)
    {
        FinalModuleName += std::string(ModuleName) + "!";
    }

    //
    // Convert object name to string
    //
    if (ObjectName != NULL)
    {
        FinalModuleName += std::string(ObjectName);
    }

    //
    // Create the structure
    //
    LocalFunctionDescription.ObjectName = std::move(FinalModuleName);
    LocalFunctionDescription.ObjectSize = ObjectSize;

    //
    // Add to disassembler map
    //
    g_DisassemblerSymbolMap[Address] = LocalFunctionDescription;
}

/**
 * @brief Update (or create) symbol map for the disassembler
 * 
 * @return BOOLEAN
 */
BOOLEAN
SymbolCreateDisassemblerSymbolMap()
{
    //
    // Clear the map table
    //
    g_DisassemblerSymbolMap.clear();

    //
    // Get all the symbols in the callback
    //
    ScriptEngineCreateSymbolTableForDisassemblerWrapper(SymbolCreateDisassemblerMapCallback);

    return TRUE;
}

/**
 * @brief shows the functions' name for the disassembler
 * @param Address
 * @param UsedBaseAddress
 * 
 * @return BOOLEAN
 */
BOOLEAN
SymbolShowFunctionNameBasedOnAddress(UINT64 Address, PUINT64 UsedBaseAddress)
{
    std::map<UINT64, LOCAL_FUNCTION_DESCRIPTION>::iterator Low, Prev;
    UINT64                                                 Pos = Address;

    //
    // Check if showing function (object) names is not prohibited
    // form settings command
    //
    if (!g_AddressConversion)
    {
        return FALSE;
    }

    //
    // Check if we already built the symbol map for disassembler or not
    //
    if (!g_DisassemblerSymbolMap.empty())
    {
        Low = g_DisassemblerSymbolMap.lower_bound(Pos);

        if (Low == g_DisassemblerSymbolMap.end())
        {
            //
            // Nothing found, maybe use rbegin()
            //
            return FALSE;
        }
        else if (Low == g_DisassemblerSymbolMap.begin() && Low->first > Address)
        {
            //
            // Nothing to do, address is below the lowest entry in symbol table
            //
            return FALSE;
        }
        else if (Low->first == Address)
        {
            if (*UsedBaseAddress != Address)
            {
                ShowMessages("%s", Low->second.ObjectName.c_str());
                *UsedBaseAddress = Address;
                return TRUE;
            }

            return FALSE;
        }
        else
        {
            Prev        = std::prev(Low);
            UINT64 Diff = Address - Prev->first;

            //
            // Check, so we have a threshold boundary to add +xx to the
            // symbols function name, in otherwords, the maximum number of
            // bytes that a function could contain (it's definitely not the
            // best option to find start and end of function, it's an approximate
            // and not always might be true)
            //
            if (Prev->second.ObjectSize >= Diff)
            {
                if (*UsedBaseAddress != Prev->first)
                {
                    ShowMessages("%s+0x%x", Prev->second.ObjectName.c_str(), Diff);
                    *UsedBaseAddress = Prev->first;
                    return TRUE;
                }

                return FALSE;
            }
            else if (DISASSEMBLY_MAXIMUM_DISTANCE_FROM_OBJECT_NAME >= Diff)
            {
                //
                // We add the logic of adding Name+X+X to show that a address is x bytes
                // after the Object Name and not within the size of the function but x
                // bytes from the above of the function
                //
                if (*UsedBaseAddress != Prev->first)
                {
                    ShowMessages("%s+0x%x+0x%x", Prev->second.ObjectName.c_str(), Diff, Diff - Prev->second.ObjectSize);
                    *UsedBaseAddress = Prev->first;
                    return TRUE;
                }

                return FALSE;
            }
        }
    }

    //
    // Nothing is showed
    //
    return FALSE;
}

/**
 * @brief Build and show symbol table details
 * @param BuildLocalSymTable Should this function call to build local symbol 
 * or the symbols are from a remote debuggee in debugger mode
 * 
 * @return VOID
 */
VOID
SymbolBuildAndShowSymbolTable()
{
    if (g_SymbolTable == NULL || g_SymbolTableSize == NULL)
    {
        ShowMessages("err, symbol table is empty. please use '.sym reload' "
                     "to build the symbol table\n");
        return;
    }

    //
    // show packet details
    //
    for (size_t i = 0; i < g_SymbolTableSize / sizeof(MODULE_SYMBOL_DETAIL); i++)
    {
        ShowMessages("is pdb details available? : %s\n", g_SymbolTable[i].IsSymbolDetailsFound ? "true" : "false");
        ShowMessages("is pdb a path instead of module name? : %s\n", g_SymbolTable[i].IsLocalSymbolPath ? "true" : "false");
        ShowMessages("base address : %llx\n", g_SymbolTable[i].BaseAddress);
        ShowMessages("file path : %s\n", g_SymbolTable[i].FilePath);
        ShowMessages("guid and age : %s\n", g_SymbolTable[i].ModuleSymbolGuidAndAge);
        ShowMessages("module symbol path/name : %s\n", g_SymbolTable[i].ModuleSymbolPath);
        ShowMessages("========================================================================\n");
    }
}

/**
 * @brief Load or download symbols
 * @param IsDownload Download from remote server if not available locally
 * @param SilentLoad Load without any message
 * 
 * @return BOOLEAN 
 */
BOOLEAN
SymbolLoadOrDownloadSymbols(BOOLEAN IsDownload, BOOLEAN SilentLoad)
{
    WCHAR            ConfigPath[MAX_PATH] = {0};
    inipp::Ini<char> Ini;
    string           SymbolServer = "";
    BOOLEAN          Result       = FALSE;

    //
    // *** Read symbol path/server from config file ***
    //

    //
    // Get config file path
    //
    GetConfigFilePath(ConfigPath);

    if (!IsFileExistW(ConfigPath))
    {
        ShowMessages("please configure the symbol path (use '.help .sympath' for more information)\n");
        return FALSE;
    }

    ifstream Is(ConfigPath);

    //
    // Read config file
    //
    Ini.parse(Is);

    //
    // Show config file
    //
    // Ini.generate(std::cout);

    inipp::get_value(Ini.sections["DEFAULT"], "SymbolServer", SymbolServer);

    Is.close();

    if (SymbolServer.empty())
    {
        ShowMessages("err, invalid config for symbol server/path\n");
        return FALSE;
    }

    //
    // Check if symbol table is empty
    //
    if (g_SymbolTable == NULL || g_SymbolTableSize == NULL)
    {
        ShowMessages("symbol table is empty, please use '.sym reload' to build a symbol table\n");
        return FALSE;
    }

    //
    // *** Load or download available symbols ***
    //

    //
    // Indicate that we're in loading routines
    //
    g_IsExecutingSymbolLoadingRoutines = TRUE;
    Result                             = ScriptEngineSymbolInitLoadWrapper(g_SymbolTable,
                                               g_SymbolTableSize,
                                               IsDownload,
                                               SymbolServer.c_str(),
                                               SilentLoad);

    //
    // Build symbol table for disassembler
    //
    SymbolCreateDisassemblerSymbolMap();

    //
    // Not in loading routines anymore
    //
    g_IsExecutingSymbolLoadingRoutines = FALSE;

    return Result;
}

/**
 * @brief check and convert string to a 64 bit unsigned interger and also
 *  check for symbol object names and evaluate expressions
 * 
 * @param TextToConvert the target string
 * @param Result result will be save to the pointer
 * 
 * @return BOOLEAN shows whether the conversion was successful or not
 */
BOOLEAN
SymbolConvertNameOrExprToAddress(const string & TextToConvert, PUINT64 Result)
{
    BOOLEAN IsFound  = FALSE;
    BOOLEAN HasError = NULL;
    UINT64  Address  = NULL;

    if (!ConvertStringToUInt64(TextToConvert, &Address))
    {
        //
        // Check for symbol object names
        //
        string ConstTextToConvert = TextToConvert;
        Address                   = ScriptEngineConvertNameToAddressWrapper(ConstTextToConvert.c_str(), &IsFound);

        if (!IsFound)
        {
            //
            // It's neither a number, nor a founded object name,
            // as the last resort, we have to test whether it's an expression or not
            // if we're in the Debugger Mode then we have to send it the kernel to get
            // the evaluation, if we're in VMI mode, then we evaluate it here with all
            // registers set to Zero
            //
            Address = ScriptEngineEvalSingleExpression(TextToConvert, &HasError);

            if (HasError)
            {
                //
                // Not found or has error
                //
                IsFound = FALSE;
            }
            else
            {
                //
                // Expression evaluated successfully
                //
                IsFound = TRUE;
            }
        }
        else
        {
            //
            // Object name is found
            //
            IsFound = TRUE;
        }
    }
    else
    {
        //
        // It's a hex number
        //
        IsFound = TRUE;
    }

    //
    // Set the number if the address is founded
    //
    if (IsFound)
    {
        *Result = Address;
    }

    return IsFound;
}

/**
 * @brief Delete and free structures and variables related to the symbols
 * 
 * @return BOOLEAN shows whether the operation was successful or not
 */
BOOLEAN
SymbolDeleteSymTable()
{
    if (g_SymbolTable != NULL)
    {
        free(g_SymbolTable);

        g_SymbolTable             = NULL;
        g_SymbolTableSize         = NULL;
        g_SymbolTableCurrentIndex = 0;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**
 * @brief make the initial packet required for symbol server
 * or reload packet
 * 
 * @param BufferToStoreDetails Pointer to a buffer to store the symbols details
 * this buffer will be allocated by this function and needs to be freed by caller
 * @param StoredLength The length that stored on the BufferToStoreDetails
 * @param UserProcessId Which user mode process to get its modules
 * @param SendOverSerial Shows whether the packet should be sent to the debugger
 * over the serial or not
 * 
 * @return BOOLEAN shows whether the operation was successful or not
 */
BOOLEAN
SymbolBuildSymbolTable(PMODULE_SYMBOL_DETAIL * BufferToStoreDetails,
                       PUINT32                 StoredLength,
                       UINT32                  UserProcessId,
                       BOOLEAN                 SendOverSerial)
{
    PRTL_PROCESS_MODULES            ModuleInfo;
    NTSTATUS                        NtStatus;
    BOOLEAN                         Status;
    ULONG                           ReturnedLength;
    PMODULE_SYMBOL_DETAIL           ModuleSymDetailArray                              = NULL;
    char                            SystemRoot[MAX_PATH]                              = {0};
    char                            ModuleSymbolPath[MAX_PATH]                        = {0};
    char                            TempPath[MAX_PATH]                                = {0};
    char                            ModuleSymbolGuidAndAge[MAXIMUM_GUID_AND_AGE_SIZE] = {0};
    BOOLEAN                         IsSymbolPdbDetailAvailable                        = FALSE;
    BOOLEAN                         IsFreeUsermodeModulesBuffer                       = FALSE;
    ULONG                           SysModuleInfoBufferSize                           = 0;
    UINT32                          ModuleDetailsSize                                 = 0;
    UINT32                          ModulesCount                                      = 0;
    PUSERMODE_LOADED_MODULE_DETAILS ModuleDetailsRequest                              = NULL;
    PUSERMODE_LOADED_MODULE_SYMBOLS Modules                                           = NULL;
    USERMODE_LOADED_MODULE_DETAILS  ModuleCountRequest                                = {0};

    //
    // Check if we found an already built symbol table
    //
    SymbolDeleteSymTable();

    //
    // Get system root
    //
    if (GetSystemDirectoryA(SystemRoot, MAX_PATH) == NULL)
    {
        ShowMessages("err, unable to get system directory (%x)\n",
                     GetLastError());

        return FALSE;
    }

    string SystemRootString(SystemRoot);

    //
    // Convert root path to lower-case
    //
    transform(SystemRootString.begin(),
              SystemRootString.end(),
              SystemRootString.begin(),
              [](unsigned char c) { return std::tolower(c); });

    //
    // Remove system32 from the root
    //
    Replace(SystemRootString, "\\system32", "");

    //
    // *****************************************************************
    //              Get kernel-mode modules information
    // *****************************************************************
    //

    //
    // Get required size of "RTL_PROCESS_MODULES" buffer
    //
    NtStatus = NtQuerySystemInformation(SystemModuleInformation, NULL, NULL, &SysModuleInfoBufferSize);

    //
    // Allocate memory for the module list
    //
    ModuleInfo = (PRTL_PROCESS_MODULES)VirtualAlloc(
        NULL,
        SysModuleInfoBufferSize,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE);

    if (!ModuleInfo)
    {
        ShowMessages("err, unable to allocate memory for module list (%x)\n",
                     GetLastError());
        return FALSE;
    }

    if (!NT_SUCCESS(
            NtStatus = NtQuerySystemInformation(SystemModuleInformation,
                                                ModuleInfo,
                                                SysModuleInfoBufferSize,
                                                NULL)))
    {
        ShowMessages("err, unable to query module list (%#x)\n", NtStatus);

        VirtualFree(ModuleInfo, 0, MEM_RELEASE);
        return FALSE;
    }

    //
    // *****************************************************************
    //              Get user-mode modules information
    // *****************************************************************
    //

    //
    // We won't fail the entire process if the user-mode modules is failed
    //

    do
    {
        //
        // Check if debugger is loaded or not
        //
        if (!g_DeviceHandle)
        {
            break;
        }

        //
        // Set the module details to get the details
        //
        ModuleCountRequest.ProcessId        = UserProcessId;
        ModuleCountRequest.OnlyCountModules = TRUE;

        //
        // Send the request to the kernel
        //
        Status = DeviceIoControl(
            g_DeviceHandle,                         // Handle to device
            IOCTL_GET_USER_MODE_MODULE_DETAILS,     // IO Control
                                                    // code
            &ModuleCountRequest,                    // Input Buffer to driver.
            sizeof(USERMODE_LOADED_MODULE_DETAILS), // Input buffer length
            &ModuleCountRequest,                    // Output Buffer from driver.
            sizeof(USERMODE_LOADED_MODULE_DETAILS), // Length of output
                                                    // buffer in bytes.
            &ReturnedLength,                        // Bytes placed in buffer.
            NULL                                    // synchronous call
        );

        if (!Status)
        {
            break;
        }

        //
        // Check if counting modules was successful or not
        //
        if (ModuleCountRequest.Result == DEBUGGER_OPERATION_WAS_SUCCESSFULL)
        {
            ModulesCount = ModuleCountRequest.ModulesCount;

            // ShowMessages("Count of modules : 0x%x\n", ModuleCountRequest.ModulesCount);

            ModuleDetailsSize = sizeof(USERMODE_LOADED_MODULE_DETAILS) +
                                (ModuleCountRequest.ModulesCount * sizeof(USERMODE_LOADED_MODULE_SYMBOLS));

            ModuleDetailsRequest = (PUSERMODE_LOADED_MODULE_DETAILS)malloc(ModuleDetailsSize);

            if (ModuleDetailsRequest == NULL)
            {
                break;
            }

            RtlZeroMemory(ModuleDetailsRequest, ModuleDetailsSize);

            //
            // Set the module details to get the modules (not count)
            //
            ModuleDetailsRequest->ProcessId        = UserProcessId;
            ModuleDetailsRequest->OnlyCountModules = FALSE;

            //
            // Send the request to the kernel
            //
            Status = DeviceIoControl(
                g_DeviceHandle,                         // Handle to device
                IOCTL_GET_USER_MODE_MODULE_DETAILS,     // IO Control
                                                        // code
                ModuleDetailsRequest,                   // Input Buffer to driver.
                sizeof(USERMODE_LOADED_MODULE_DETAILS), // Input buffer length
                ModuleDetailsRequest,                   // Output Buffer from driver.
                ModuleDetailsSize,                      // Length of output
                                                        // buffer in bytes.
                &ReturnedLength,                        // Bytes placed in buffer.
                NULL                                    // synchronous call
            );

            if (!Status)
            {
                free(ModuleDetailsRequest);
                break;
            }

            //
            // Show modules list
            //
            if (ModuleCountRequest.Result == DEBUGGER_OPERATION_WAS_SUCCESSFULL)
            {
                //
                // Se the modules buffer
                //
                Modules = (PUSERMODE_LOADED_MODULE_SYMBOLS)((UINT64)ModuleDetailsRequest +
                                                            sizeof(USERMODE_LOADED_MODULE_DETAILS));

                //
                // Check if modules buffer is valid
                //
                if (Modules == NULL)
                {
                    ModulesCount = 0;
                    free(ModuleDetailsRequest);
                    break;
                }
            }
            else
            {
                ShowErrorMessage(ModuleCountRequest.Result);
                free(ModuleDetailsRequest);
                break;
            }

            //
            // Everything is okay when we reached here
            //
            IsFreeUsermodeModulesBuffer = TRUE;
        }
        else
        {
            ShowErrorMessage(ModuleCountRequest.Result);
            break;
        }

    } while (FALSE);

    //
    // *****************************************************************
    //

    //
    // Allocate Details buffer
    //
    ModuleSymDetailArray = (PMODULE_SYMBOL_DETAIL)malloc(
        (ModuleInfo->NumberOfModules + ModulesCount) * sizeof(MODULE_SYMBOL_DETAIL));

    if (ModuleSymDetailArray == NULL)
    {
        ShowMessages("err, unable to allocate memory for module list (%x)\n",
                     GetLastError());

        if (IsFreeUsermodeModulesBuffer)
        {
            free(ModuleDetailsRequest);
        }
        return FALSE;
    }

    //
    // Make sure buffer is zero
    //
    RtlZeroMemory(ModuleSymDetailArray, (ModuleInfo->NumberOfModules + ModulesCount) * sizeof(MODULE_SYMBOL_DETAIL));

    //
    // ----------------------------------------------------------------------------------
    // Add user-modules
    // ----------------------------------------------------------------------------------
    //

    if (ModulesCount != 0)
    {
        for (int i = 0; i < ModulesCount; i++)
        {
            //
            // For logging purpose
            //

            /* 
            ShowMessages("%016llx\t%016llx\t%ws\n",
                         Modules[i].BaseAddress,
                         Modules[i].Entrypoint,
                         Modules[i].FilePath);
                         */

            //
            // Read symbol signature details
            //
            RtlZeroMemory(ModuleSymbolPath, sizeof(ModuleSymbolPath));
            RtlZeroMemory(TempPath, sizeof(TempPath));
            RtlZeroMemory(ModuleSymbolGuidAndAge, sizeof(ModuleSymbolGuidAndAge));

            //
            // Convert symbol path from unicode to ascii
            //
            wcstombs(TempPath, Modules[i].FilePath, MAX_PATH);

            if (ScriptEngineConvertFileToPdbFileAndGuidAndAgeDetailsWrapper(
                    TempPath,
                    ModuleSymbolPath,
                    ModuleSymbolGuidAndAge))
            {
                IsSymbolPdbDetailAvailable = TRUE;

                // ShowMessages("Hash : %s , Symbol path : %s\n", ModuleSymbolGuidAndAge, ModuleSymbolPath);
            }
            else
            {
                IsSymbolPdbDetailAvailable = FALSE;

                // ShowMessages("err, unable to get module pdb details\n");
            }

            //
            // Build the structure for this module
            //
            ModuleSymDetailArray[i].BaseAddress = Modules[i].BaseAddress;
            ModuleSymDetailArray[i].IsUserMode  = TRUE;
            memcpy(ModuleSymDetailArray[i].FilePath, TempPath, strlen(TempPath));

            if (IsSymbolPdbDetailAvailable)
            {
                ModuleSymDetailArray[i].IsSymbolDetailsFound = TRUE;
                memcpy(ModuleSymDetailArray[i].ModuleSymbolGuidAndAge, ModuleSymbolGuidAndAge, MAXIMUM_GUID_AND_AGE_SIZE);
                memcpy(ModuleSymDetailArray[i].ModuleSymbolPath, ModuleSymbolPath, MAX_PATH);

                //
                // Check if pdb file name is a real path or a module name
                //
                string ModuleSymbolPathString(ModuleSymbolPath);
                if (ModuleSymbolPathString.find(":\\") != std::string::npos)
                    ModuleSymDetailArray[i].IsLocalSymbolPath = TRUE;
                else
                    ModuleSymDetailArray[i].IsLocalSymbolPath = FALSE;
            }
            else
            {
                ModuleSymDetailArray[i].IsSymbolDetailsFound = FALSE;
            }

            //
            // Check if it should be send to the remote debugger over serial
            // and also make sure that we're connected to the remote debugger
            // and this is a debuggee
            //
            if (SendOverSerial)
            {
                KdSendSymbolDetailPacket(&ModuleSymDetailArray[i], i, ModuleInfo->NumberOfModules + ModulesCount);
            }
        }
    }

    //
    // ----------------------------------------------------------------------------------
    // Add kernel-modules
    // ----------------------------------------------------------------------------------
    //

    for (int i = 0; i < ModuleInfo->NumberOfModules; i++)
    {
        UINT32 IndexInSymbolBuffer = ModulesCount + i;

        auto PathName = ModuleInfo->Modules[i].FullPathName + ModuleInfo->Modules[i].OffsetToFileName;

        //
        // Read symbol signature details
        //
        RtlZeroMemory(ModuleSymbolPath, sizeof(ModuleSymbolPath));
        RtlZeroMemory(ModuleSymbolGuidAndAge, sizeof(ModuleSymbolGuidAndAge));

        string ModuleFullPath((const char *)ModuleInfo->Modules[i].FullPathName);

        if (ModuleFullPath.rfind("\\SystemRoot\\", 0) == 0)
        {
            //
            // Path starts with \SystemRoot\
            // we should change it to the real system root
            // path
            //
            Replace(ModuleFullPath, "\\SystemRoot", SystemRootString);
        }

        if (ScriptEngineConvertFileToPdbFileAndGuidAndAgeDetailsWrapper(ModuleFullPath.c_str(), ModuleSymbolPath, ModuleSymbolGuidAndAge))
        {
            IsSymbolPdbDetailAvailable = TRUE;

            //
            // ShowMessages("Hash : %s , Symbol path : %s\n", ModuleSymbolGuidAndAge, ModuleSymbolPath);
            //
        }
        else
        {
            IsSymbolPdbDetailAvailable = FALSE;

            //
            // ShowMessages("err, unable to get module pdb details\n");
            //
        }

        //
        // Build the structure for this module
        //
        ModuleSymDetailArray[IndexInSymbolBuffer].BaseAddress = (UINT64)ModuleInfo->Modules[i].ImageBase;
        memcpy(ModuleSymDetailArray[IndexInSymbolBuffer].FilePath, ModuleFullPath.c_str(), ModuleFullPath.size());

        if (IsSymbolPdbDetailAvailable)
        {
            ModuleSymDetailArray[IndexInSymbolBuffer].IsSymbolDetailsFound = TRUE;
            memcpy(ModuleSymDetailArray[IndexInSymbolBuffer].ModuleSymbolGuidAndAge, ModuleSymbolGuidAndAge, MAXIMUM_GUID_AND_AGE_SIZE);
            memcpy(ModuleSymDetailArray[IndexInSymbolBuffer].ModuleSymbolPath, ModuleSymbolPath, MAX_PATH);

            //
            // Check if pdb file name is a real path or a module name
            //
            string ModuleSymbolPathString(ModuleSymbolPath);
            if (ModuleSymbolPathString.find(":\\") != std::string::npos)
                ModuleSymDetailArray[IndexInSymbolBuffer].IsLocalSymbolPath = TRUE;
            else
                ModuleSymDetailArray[IndexInSymbolBuffer].IsLocalSymbolPath = FALSE;
        }
        else
        {
            ModuleSymDetailArray[IndexInSymbolBuffer].IsSymbolDetailsFound = FALSE;
        }

        //
        // Check if it should be send to the remote debugger over serial
        // and also make sure that we're connected to the remote debugger
        // and this is a debuggee
        //
        if (SendOverSerial)
        {
            KdSendSymbolDetailPacket(&ModuleSymDetailArray[IndexInSymbolBuffer], i, ModuleInfo->NumberOfModules + ModulesCount);
        }
    }

    //
    // ----------------------------------------------------------------------------------
    //

    //
    // Store the buffer and length of module symbols details
    //
    *BufferToStoreDetails = ModuleSymDetailArray;
    *StoredLength         = (ModuleInfo->NumberOfModules + ModulesCount) * sizeof(MODULE_SYMBOL_DETAIL);

    VirtualFree(ModuleInfo, 0, MEM_RELEASE);

    if (IsFreeUsermodeModulesBuffer)
    {
        free(ModuleDetailsRequest);
    }

    return TRUE;
}

/**
 * @brief Allocate (build) and update the symbol table whenever a debuggee is attached
 * on the debugger mode
 * 
 * @param SymbolDetail Pointer to a buffer that was received as the single 
 * symbol info
 * 
 * @return BOOLEAN shows whether the operation was successful or not
 */
BOOLEAN
SymbolBuildAndUpdateSymbolTable(PMODULE_SYMBOL_DETAIL SymbolDetail)
{
    //
    // Check to avoid overflow in symbol table
    //
    if (g_SymbolTableCurrentIndex >= MAXIMUM_SUPPORTED_SYMBOLS)
    {
        ShowMessages("err, the symbol table buffer is full, unable to add new symbol\n");
        return FALSE;
    }

    //
    // Check if we found an already built symbol table
    //
    if (g_SymbolTable == NULL)
    {
        //
        // Allocate Details buffer
        //
        g_SymbolTable = (PMODULE_SYMBOL_DETAIL)malloc(MAXIMUM_SUPPORTED_SYMBOLS * sizeof(MODULE_SYMBOL_DETAIL));

        if (g_SymbolTable == NULL)
        {
            ShowMessages("err, unable to allocate memory for module list (%x)\n",
                         GetLastError());
            return FALSE;
        }

        //
        // Reset the index
        //
        g_SymbolTableCurrentIndex = 0;

        //
        // Make sure buffer is zero
        //
        RtlZeroMemory(g_SymbolTable, MAXIMUM_SUPPORTED_SYMBOLS * sizeof(MODULE_SYMBOL_DETAIL));
    }

    //
    // Move it to the new buffer
    //
    memcpy(&g_SymbolTable[g_SymbolTableCurrentIndex], SymbolDetail, sizeof(MODULE_SYMBOL_DETAIL));

    //
    // Add to index for future symbols
    //
    g_SymbolTableCurrentIndex++;

    //
    // Compute the (new) current size
    //
    g_SymbolTableSize = g_SymbolTableCurrentIndex * sizeof(MODULE_SYMBOL_DETAIL);

    return TRUE;
}

/**
 * @brief Update the symbol table from remote debuggee in debugger mode
 * @param ProcessId
 * 
 * @return BOOLEAN shows whether the operation was successful or not
 */
BOOLEAN
SymbolReloadSymbolTableInDebuggerMode(UINT32 ProcessId)
{
    //
    // Check if we found an already built symbol table
    //
    SymbolDeleteSymTable();

    //
    // Request to send new symbol details
    //
    if (KdSendSymbolReloadPacketToDebuggee(ProcessId))
    {
        ShowMessages("symbol table updated successfully\n");
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
