/**
 * @file attach.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
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
    ShowMessages("syntax : \t.attach [pid (hex)] [tid (hex)]\n");
    ShowMessages("note : if you don't specify the thread id (id), then it shows "
                 "the list of active threads on the target process (it won't "
                 "attach to the target thread).\n");
    ShowMessages("\t\te.g : .attach pid b60 \n");
    ShowMessages("\t\te.g : .attach pid b60 tid 220 \n");
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
    UINT64  TargetTid = 0;
    BOOLEAN NextIsPid = FALSE;
    BOOLEAN NextIsTid = FALSE;

    //
    // It's a attach to a target PID
    //
    if (SplittedCommand.size() >= 6)
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
        else if (NextIsTid)
        {
            NextIsTid = FALSE;

            if (!ConvertStringToUInt64(item, &TargetTid))
            {
                ShowMessages("please specify a correct hex value for thread id\n\n");
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
        else if (!item.compare("tid"))
        {
            //
            // next item is a tid for the thread
            //
            NextIsTid = TRUE;
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
    // Check if the thread id is specified or not, if not then
    // we should just show the thread of the target process
    //
    if (TargetTid == 0)
    {
        UsermodeDebuggingListProcessThreads(TargetPid);
        return;
    }
    else
    {
        //
        // Check if the process id and thread id is correct or not
        //
        if (!UsermodeDebuggingCheckThreadByProcessId(TargetPid, TargetTid))
        {
            ShowMessages("err, the thread or the process not found, or the thread not "
                         "belongs to the process, or the thread is terminated\n");
            return;
        }
    }

    //
    // Perform attach to target process
    //
    UsermodeDebuggingAttachToProcess(TargetPid, TargetTid, NULL);
}
