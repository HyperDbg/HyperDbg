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
 * @brief handle lm command
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandLm(vector<string> SplittedCommand, string Command)
{
    NTSTATUS             Status = STATUS_UNSUCCESSFUL;
    PRTL_PROCESS_MODULES ModulesInfo;
    ULONG                SysModuleInfoBufferSize = 0;
    char *               Search;

    if (SplittedCommand.size() >= 3)
    {
        ShowMessages("incorrect use of 'lm'\n\n");
        CommandLmHelp();
        return;
    }

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
        return;
    }

    Status = NtQuerySystemInformation(SystemModuleInformation,
                                      ModulesInfo,
                                      SysModuleInfoBufferSize,
                                      NULL);
    if (!NT_SUCCESS(Status))
    {
        ShowMessages("\nError: Unable to query module list (%#x)\n", Status);

        VirtualFree(ModulesInfo, 0, MEM_RELEASE);
        return;
    }

    ShowMessages("start\t\t\tsize\tname\t\t\t\tpath\n\n");

    for (ULONG i = 0; i < ModulesInfo->NumberOfModules; i++)
    {
        RTL_PROCESS_MODULE_INFORMATION * CurrentModule = &ModulesInfo->Modules[i];
        //
        // Check if we need to search for the module or not
        //
        if (SplittedCommand.size() == 2)
        {
            Search = strstr((char *)CurrentModule->FullPathName, SplittedCommand.at(1).c_str());
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
}
