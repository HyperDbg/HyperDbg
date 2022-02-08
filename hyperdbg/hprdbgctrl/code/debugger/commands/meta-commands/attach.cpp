/**
 * @file attach.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief .attach command
 * @details
 * @version 0.1
 * @date 2020-08-27
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
 * @brief help of .attach command
 *
 * @return VOID
 */
VOID
CommandAttachHelp()
{
    ShowMessages(".attach : attach to debug a thread in VMI Mode.\n\n");
    ShowMessages("syntax : \t.attach [pid ProcessId (hex)]\n");
    ShowMessages("\t\te.g : .attach pid b60 \n");
}

/**
 * @brief .attach command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandAttach(vector<string> SplittedCommand, string Command)
{
    UINT64  TargetPid = 0;
    BOOLEAN NextIsPid = FALSE;

    //
    // Show a message that the user debugger is still in the experimental version
    //
    ShowMessages("in contrast with the kernel debugger, the user debugger is still very basic "
                 "and needs a lot of tests and improvements. It's highly recommended not to run the "
                 "user debugger in your bare metal system. Instead, run it on a supported virtual "
                 "machine to won't end up with a Blue Screen of Death (BSOD) in your primary device. "
                 "Please keep reporting the issues to improve the user debugger\n\n");

    //
    // It's a attach to a target PID
    //
    if (SplittedCommand.size() >= 4)
    {
        ShowMessages("incorrect use of '.attach'\n\n");
        CommandAttachHelp();
        return;
    }

    //
    // .attach and .detach commands are only supported in VMI Mode
    //
    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        ShowMessages("err, '.attach', and '.detach' commands are only usable "
                     "in VMI Mode, you can use the '.process', or the '.thread' "
                     "in Debugger Mode\n");
        return;
    }

    for (auto item : SplittedCommand)
    {
        //
        // Find out whether the user enters pid or not
        //
        if (NextIsPid)
        {
            NextIsPid = FALSE;

            if (!ConvertStringToUInt64(item, &TargetPid))
            {
                ShowMessages("please specify a correct hex value for process id\n\n");
                CommandAttachHelp();
                return;
            }
        }
        else if (!item.compare("pid"))
        {
            //
            // next item is a pid for the process
            //
            NextIsPid = TRUE;
        }
    }

    //
    // Check if the process id is empty or not
    //
    if (TargetPid == 0)
    {
        ShowMessages("please specify a hex value for process id\n\n");
        CommandAttachHelp();
        return;
    }

    //
    // Perform attach to target process
    //
    UdAttachToProcess(TargetPid, NULL, NULL);
}
