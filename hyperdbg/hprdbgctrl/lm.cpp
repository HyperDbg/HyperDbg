/**
 * @file lm.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief lm command
 * @details
 * @version 0.1
 * @date 2020-07-13
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

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
    ShowMessages("syntax : \tlm [name]\n");
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
    PRTL_PROCESS_MODULES ModuleInfo;
    NTSTATUS             status;
    ULONG                i;
    char *               Search;

    if (SplittedCommand.size() >= 3)
    {
        ShowMessages("incorrect use of 'lm'\n\n");
        CommandLmHelp();
        return;
    }

    //
    // Allocate memory for the module list
    //
    ModuleInfo = (PRTL_PROCESS_MODULES)VirtualAlloc(
        NULL,
        1024 * 1024,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE);

    if (!ModuleInfo)
    {
        ShowMessages("\nUnable to allocate memory for module list (%d)\n",
                     GetLastError());
        return;
    }

    //
    // 11 = SystemModuleInformation
    //
    if (!NT_SUCCESS(
            status = NtQuerySystemInformation((SYSTEM_INFORMATION_CLASS)11,
                                              ModuleInfo,
                                              1024 * 1024,
                                              NULL)))
    {
        ShowMessages("\nError: Unable to query module list (%#x)\n", status);

        VirtualFree(ModuleInfo, 0, MEM_RELEASE);
        return;
    }

    ShowMessages("start\t\t\tsize\tname\t\t\t\tpath\n\n");

    for (i = 0; i < ModuleInfo->NumberOfModules; i++)
    {
        //
        // Check if we need to search for the module or not
        //
        if (SplittedCommand.size() == 2)
        {
            Search = strstr((char *)ModuleInfo->Modules[i].FullPathName,
                            SplittedCommand.at(1).c_str());
            if (Search == NULL)
            {
                //
                // not found
                //
                continue;
            }
        }

        ShowMessages("%s\t", SeparateTo64BitValue((UINT64)ModuleInfo->Modules[i].ImageBase).c_str());
        ShowMessages("%x\t", ModuleInfo->Modules[i].ImageSize);

        auto   PathName    = ModuleInfo->Modules[i].FullPathName + ModuleInfo->Modules[i].OffsetToFileName;
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

        ShowMessages("%s\n", ModuleInfo->Modules[i].FullPathName);
    }

    VirtualFree(ModuleInfo, 0, MEM_RELEASE);
    return;
}
