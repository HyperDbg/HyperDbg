/**
 * @file thread.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief show and change threads
 * @details
 * @version 0.1
 * @date 2021-11-23
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "..\hprdbgctrl\pch.h"

//
// Global Variables
//
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;

/**
 * @brief help of .thread command
 *
 * @return VOID
 */
VOID
CommandThreadHelp()
{
    ShowMessages(".thread : show and change the threads. "
                 "This command needs public symbols for ntoskrnl.exe if "
                 "you want to see the threads list.\n\n");
    ShowMessages("syntax : \t.thread [type (tid | thread | list)] [new thread id (hex) | new nt!_ETHREAD address]\n");
    ShowMessages("\t\te.g : .thread\n");
    ShowMessages("\t\te.g : .thread list\n");
    ShowMessages("\t\te.g : .thread tid 48a4\n");
    ShowMessages("\t\te.g : .thread thread ffff948c`c8970200\n");
}

/**
 * @brief .thread command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandThread(vector<string> SplittedCommand, string Command)
{
    UINT32                              TargetThreadId        = 0;
    UINT64                              TargetThread          = 0;
    DEBUGGEE_THREAD_LIST_NEEDED_DETAILS ThreadListNeededItems = {0};

    if (SplittedCommand.size() >= 4)
    {
        ShowMessages("incorrect use of '.thread'\n\n");
        CommandThreadHelp();
        return;
    }

    //
    // Check if it's connected to a remote debuggee or not
    //
    if (!g_IsSerialConnectedToRemoteDebuggee)
    {
        ShowMessages("err, you're not connected to any debuggee\n");
        return;
    }

    if (SplittedCommand.size() == 1)
    {
        //
        // Send the packet to get current thread
        //
        KdSendSwitchThreadPacketToDebuggee(DEBUGGEE_DETAILS_AND_SWITCH_THREAD_GET_THREAD_DETAILS,
                                           NULL,
                                           NULL,
                                           NULL);
    }
    else if (SplittedCommand.size() == 2)
    {
        if (!SplittedCommand.at(1).compare("list"))
        {
            //
            // Not implemented yet !
            //
        }
        else
        {
            ShowMessages(
                "err, unknown parameter at '%s'\n\n",
                SplittedCommand.at(1).c_str());
            CommandThreadHelp();
            return;
        }
    }
    else if (SplittedCommand.size() == 3)
    {
        if (!SplittedCommand.at(1).compare("tid"))
        {
            if (!ConvertStringToUInt32(SplittedCommand.at(2), &TargetThreadId))
            {
                ShowMessages(
                    "please specify a correct hex value for the thread id that you "
                    "want to operate on it\n\n");
                CommandThreadHelp();
                return;
            }
        }
        else if (!SplittedCommand.at(1).compare("thread"))
        {
            if (!SymbolConvertNameOrExprToAddress(SplittedCommand.at(2), &TargetThread))
            {
                ShowMessages(
                    "please specify a correct hex value for the thread (nt!_ETHREAD) that you "
                    "want to operate on it\n\n");
                CommandThreadHelp();
                return;
            }
        }
        else
        {
            ShowMessages(
                "err, unknown parameter at '%s'\n\n",
                SplittedCommand.at(2).c_str());
            CommandThreadHelp();
            return;
        }

        //
        // Send the packet to change the thread
        //
        KdSendSwitchThreadPacketToDebuggee(DEBUGGEE_DETAILS_AND_SWITCH_THREAD_PERFORM_SWITCH,
                                           TargetThreadId,
                                           TargetThread,
                                           NULL);
    }
    else
    {
        ShowMessages("invalid parameter\n\n");
        CommandThreadHelp();
        return;
    }
}
