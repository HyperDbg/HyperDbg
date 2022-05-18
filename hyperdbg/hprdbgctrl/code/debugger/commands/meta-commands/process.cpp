/**
 * @file process.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief show and change process
 * @details
 * @version 0.1
 * @date 2021-02-02
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;

/**
 * @brief help of .process command
 *
 * @return VOID
 */
VOID
CommandProcessHelp()
{
    ShowMessages(".process, .process2 : shows and changes the processes. "
                 "This command needs public symbols for ntoskrnl.exe if "
                 "you want to see the processes list. Please visit the "
                 "documentation to know about the difference between '.process' "
                 "and '.process2'.\n\n");

    ShowMessages("syntax : \t.process\n");
    ShowMessages("syntax : \t.process [list]\n");
    ShowMessages("syntax : \t.process [pid ProcessId (hex)]\n");
    ShowMessages("syntax : \t.process [process Eprocess (hex)]\n");
    ShowMessages("syntax : \t.process2 [pid ProcessId (hex)]\n");
    ShowMessages("syntax : \t.process2 [process Eprocess (hex)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : .process\n");
    ShowMessages("\t\te.g : .process list\n");
    ShowMessages("\t\te.g : .process pid 4\n");
    ShowMessages("\t\te.g : .process2 pid 4\n");
    ShowMessages("\t\te.g : .process process ffff948c`c2349280\n");
}

/**
 * @brief .process command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandProcess(vector<string> SplittedCommand, string Command)
{
    UINT32                               TargetProcessId            = 0;
    UINT64                               TargetProcess              = 0;
    UINT64                               AddressOfActiveProcessHead = 0; // nt!PsActiveProcessHead
    UINT32                               OffsetOfImageFileName      = 0; // nt!_EPROCESS.ImageFileName
    UINT32                               OffsetOfUniqueProcessId    = 0; // nt!_EPROCESS.UniqueProcessId
    UINT32                               OffsetOfActiveProcessLinks = 0; // nt!_EPROCESS.ActiveProcessLinks
    BOOLEAN                              ResultOfGettingOffsets     = FALSE;
    BOOLEAN                              IsSetByClkIntr             = FALSE;
    DEBUGGEE_PROCESS_LIST_NEEDED_DETAILS ProcessListNeededItems     = {0};

    if (SplittedCommand.size() >= 4)
    {
        ShowMessages("incorrect use of '.process'\n\n");
        CommandProcessHelp();
        return;
    }

    if (SplittedCommand.size() == 1)
    {
        //
        // Check if it's connected to a remote debuggee or not
        //
        if (!g_IsSerialConnectedToRemoteDebuggee)
        {
            //
            // Get the process details in VMI mode
            //
            ObjectShowProcessesOrThreadDetails(TRUE);
        }
        else
        {
            //
            // Send the packet to get current process
            //
            KdSendSwitchProcessPacketToDebuggee(DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_GET_PROCESS_DETAILS,
                                                NULL,
                                                NULL,
                                                FALSE,
                                                NULL);
        }
    }
    else if (SplittedCommand.size() == 2)
    {
        if (!SplittedCommand.at(1).compare("list"))
        {
            //
            // Query for nt!_EPROCESS.ImageFileName, nt!_EPROCESS.UniqueProcessId,
            // nt!_EPROCESS.UniqueProcessId offset from the top of nt!_EPROCESS,
            // and nt!PsActiveProcessHead address and check if we find them or not,
            // otherwise, it means that the PDB for ntoskrnl.exe is not available
            //
            if (ScriptEngineGetFieldOffsetWrapper((CHAR *)"nt!_EPROCESS", (CHAR *)"ActiveProcessLinks", &OffsetOfActiveProcessLinks) &&
                ScriptEngineGetFieldOffsetWrapper((CHAR *)"nt!_EPROCESS", (CHAR *)"ImageFileName", &OffsetOfImageFileName) &&
                ScriptEngineGetFieldOffsetWrapper((CHAR *)"nt!_EPROCESS", (CHAR *)"UniqueProcessId", &OffsetOfUniqueProcessId) &&
                SymbolConvertNameOrExprToAddress("nt!PsActiveProcessHead", &AddressOfActiveProcessHead))
            {
                //
                // For test offsets and addresses
                //

                /*
                ShowMessages("Address of ActiveProcessHead : %llx\n", AddressOfActiveProcessHead);
                ShowMessages("Offset Of ActiveProcessLinks : 0x%x\n", OffsetOfActiveProcessLinks);
                ShowMessages("Offset Of ImageFileName : 0x%x\n", OffsetOfImageFileName);
                ShowMessages("Offset Of UniqueProcessId : 0x%x\n", OffsetOfUniqueProcessId);
                */

                ProcessListNeededItems.PsActiveProcessHead      = AddressOfActiveProcessHead;
                ProcessListNeededItems.ActiveProcessLinksOffset = OffsetOfActiveProcessLinks;
                ProcessListNeededItems.ImageFileNameOffset      = OffsetOfImageFileName;
                ProcessListNeededItems.UniquePidOffset          = OffsetOfUniqueProcessId;

                if (!g_IsSerialConnectedToRemoteDebuggee)
                {
                    //
                    // Get list of processes in VMI mode
                    //
                    ObjectShowProcessesOrThreadList(TRUE,
                                                    &ProcessListNeededItems,
                                                    NULL,
                                                    NULL);
                }
                else
                {
                    //
                    // Send the packet to show list of process
                    //
                    KdSendSwitchProcessPacketToDebuggee(DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_GET_PROCESS_LIST,
                                                        NULL,
                                                        NULL,
                                                        FALSE,
                                                        &ProcessListNeededItems);
                }
            }
            else
            {
                ShowMessages("err, the need offset to iterate over processes not found, "
                             "make sure to load ntoskrnl.exe's PDB file. use '.help .sym' for "
                             "more information\n");
                return;
            }
        }
        else
        {
            ShowMessages(
                "err, unknown parameter at '%s'\n\n",
                SplittedCommand.at(1).c_str());
            CommandProcessHelp();
            return;
        }
    }
    else if (SplittedCommand.size() == 3)
    {
        //
        // Check if it's connected to a remote debuggee or not
        //
        if (!g_IsSerialConnectedToRemoteDebuggee)
        {
            ShowMessages("err, you're not connected to any debuggee in Debugger Mode, "
                         "you can use the '.attach', or the '.detach' commands if you're "
                         "operating in VMI Mode\n");
            return;
        }

        if (!SplittedCommand.at(1).compare("pid"))
        {
            if (!ConvertStringToUInt32(SplittedCommand.at(2), &TargetProcessId))
            {
                ShowMessages(
                    "please specify a correct hex value for the process id that you "
                    "want to operate on it\n\n");
                CommandProcessHelp();
                return;
            }
        }
        else if (!SplittedCommand.at(1).compare("process"))
        {
            if (!SymbolConvertNameOrExprToAddress(SplittedCommand.at(2), &TargetProcess))
            {
                ShowMessages(
                    "please specify a correct hex value for the process (nt!_EPROCESS) that you "
                    "want to operate on it\n\n");
                CommandProcessHelp();
                return;
            }
        }
        else
        {
            ShowMessages(
                "err, unknown parameter at '%s'\n\n",
                SplittedCommand.at(2).c_str());
            CommandProcessHelp();
            return;
        }

        //
        // Check for switching method
        //
        if (!SplittedCommand.at(0).compare(".process2"))
        {
            IsSetByClkIntr = FALSE;
        }
        else
        {
            IsSetByClkIntr = TRUE;
        }

        //
        // Send the packet to change process
        //
        KdSendSwitchProcessPacketToDebuggee(DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PERFORM_SWITCH,
                                            TargetProcessId,
                                            TargetProcess,
                                            IsSetByClkIntr,
                                            NULL);
    }
    else
    {
        ShowMessages("invalid parameter\n\n");
        CommandProcessHelp();
        return;
    }
}
