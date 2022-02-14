/**
 * @file lm.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief lm command
 * @details
 * @version 0.1
 * @date 2020-07-13
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "..\hprdbgctrl\pch.h"

using namespace std;

//
// Global Variables
//
extern ACTIVE_DEBUGGING_PROCESS g_ActiveProcessDebuggingState;

/**
 * @brief help of lm command
 *
 * @return VOID
 */
VOID
CommandLmHelp()
{
    ShowMessages(
        "lm : list kernel modules' base address, size, name and path.\n\n");
    ShowMessages("syntax : \tlm \n");
    ShowMessages("syntax : \tlm [m Name (string)] [pid ProcessId (hex)]\n");
    ShowMessages("\t\te.g : lm\n");
    ShowMessages("\t\t\tdescription : list all modules\n");
    ShowMessages("\t\te.g : lm nt\n");
    ShowMessages("\t\t\tdescription : search and show all modules that contain "
                 "'nt' in their path or name\n");
}

/**
 * @brief show modules for specified user mode process
 * @param ProcessId
 * 
 * @return BOOLEAN
 */
BOOLEAN
CommandLmShowUserModeModule(UINT32 ProcessId)
{
    BOOLEAN                         Status;
    ULONG                           ReturnedLength;
    UINT32                          ModuleDetailsSize    = 0;
    UINT32                          ModulesCount         = 0;
    PUSERMODE_LOADED_MODULE_DETAILS ModuleDetailsRequest = NULL;
    PUSERMODE_LOADED_MODULE_SYMBOLS Modules              = NULL;
    USERMODE_LOADED_MODULE_DETAILS  ModuleCountRequest   = {0};

    //
    // Check if debugger is loaded or not
    //
    if (!g_DeviceHandle)
    {
        return FALSE;
    }

    //
    // Set the module details to get the details
    //
    ModuleCountRequest.ProcessId        = ProcessId;
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
        return FALSE;
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
            return FALSE;
        }

        RtlZeroMemory(ModuleDetailsRequest, ModuleDetailsSize);

        //
        // Set the module details to get the modules (not count)
        //
        ModuleDetailsRequest->ProcessId        = ProcessId;
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
            return FALSE;
        }

        //
        // Show modules list
        //
        if (ModuleCountRequest.Result == DEBUGGER_OPERATION_WAS_SUCCESSFULL)
        {
            Modules = (PUSERMODE_LOADED_MODULE_SYMBOLS)((UINT64)ModuleDetailsRequest +
                                                        sizeof(USERMODE_LOADED_MODULE_DETAILS));
            ShowMessages("user mode\n");
            ShowMessages("start\t\t\tentrypoint\t\tpath\n\n");

            for (size_t i = 0; i < ModuleCountRequest.ModulesCount; i++)
            {
                ShowMessages("%016llx\t%016llx\t%ws\n",
                             Modules[i].BaseAddress,
                             Modules[i].Entrypoint,
                             Modules[i].FilePath);
            }
        }
        else
        {
            ShowErrorMessage(ModuleCountRequest.Result);
            return FALSE;
        }

        ShowMessages("\n==============================================================================\n\n");

        free(ModuleDetailsRequest);
        return TRUE;
    }
    else
    {
        ShowErrorMessage(ModuleCountRequest.Result);
        return FALSE;
    }
}

/**
 * @brief show modules for kernel mode 
 * 
 * @return BOOLEAN
 */
BOOLEAN
CommandLmShowKernelModeModule(const char * SearchModule)
{
    NTSTATUS             Status = STATUS_UNSUCCESSFUL;
    PRTL_PROCESS_MODULES ModulesInfo;
    ULONG                SysModuleInfoBufferSize = 0;
    char *               Search;

    //
    // Get required size of "RTL_PROCESS_MODULES" buffer
    //
    Status = NtQuerySystemInformation(SystemModuleInformation, NULL, NULL, &SysModuleInfoBufferSize);

    //
    // Allocate memory for the module list
    //
    ModulesInfo = (PRTL_PROCESS_MODULES)VirtualAlloc(
        NULL,
        SysModuleInfoBufferSize,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE);

    if (!ModulesInfo)
    {
        ShowMessages("\nUnable to allocate memory for module list (%x)\n",
                     GetLastError());
        return FALSE;
    }

    Status = NtQuerySystemInformation(SystemModuleInformation,
                                      ModulesInfo,
                                      SysModuleInfoBufferSize,
                                      NULL);
    if (!NT_SUCCESS(Status))
    {
        ShowMessages("\nError: Unable to query module list (%#x)\n", Status);

        VirtualFree(ModulesInfo, 0, MEM_RELEASE);
        return FALSE;
    }

    ShowMessages("kernel mode\n");
    ShowMessages("start\t\t\tsize\tname\t\t\t\tpath\n\n");

    for (ULONG i = 0; i < ModulesInfo->NumberOfModules; i++)
    {
        RTL_PROCESS_MODULE_INFORMATION * CurrentModule = &ModulesInfo->Modules[i];
        //
        // Check if we need to search for the module or not
        //
        if (SearchModule != NULL)
        {
            Search = strstr((char *)CurrentModule->FullPathName, SearchModule);
            if (Search == NULL)
            {
                //
                // not found
                //
                continue;
            }
        }

        ShowMessages("%s\t", SeparateTo64BitValue((UINT64)CurrentModule->ImageBase).c_str());
        ShowMessages("%x\t", CurrentModule->ImageSize);

        auto   PathName    = CurrentModule->FullPathName + CurrentModule->OffsetToFileName;
        UINT32 PathNameLen = strlen((const char *)PathName);

        ShowMessages("%s\t", PathName);

        if (PathNameLen >= 24)
        {
        }
        else if (PathNameLen >= 16)
        {
            ShowMessages("\t");
        }
        else if (PathNameLen >= 8)
        {
            ShowMessages("\t\t");
        }
        else
        {
            ShowMessages("\t\t\t");
        }

        ShowMessages("%s\n", CurrentModule->FullPathName);
    }

    VirtualFree(ModulesInfo, 0, MEM_RELEASE);

    return TRUE;
}

/**
 * @brief handle lm command
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandLm(vector<string> SplittedCommand, string Command)
{
    //
    // Show user mode modules
    //
    if (g_ActiveProcessDebuggingState.IsActive)
    {
        CommandLmShowUserModeModule(g_ActiveProcessDebuggingState.ProcessId);
    }
    else
    {
        CommandLmShowUserModeModule(GetCurrentProcessId());
    }

    //
    // Show kernel mode modules
    //
    if (SplittedCommand.size() == 2)
    {
        CommandLmShowKernelModeModule(SplittedCommand.at(1).c_str());
    }
    else
    {
        CommandLmShowKernelModeModule(NULL);
    }
}
