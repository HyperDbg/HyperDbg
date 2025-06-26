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
#include "pch.h"

//
// Global Variables
//
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;
extern BOOLEAN g_IsSerialConnectedToRemoteDebugger;

/**
 * @brief help of the .attach command
 *
 * @return VOID
 */
VOID
CommandAttachHelp()
{
    ShowMessages(".attach : attaches to debug a thread in VMI Mode.\n\n");

    ShowMessages("syntax : \t.attach [pid ProcessId (hex)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : .attach pid b60 \n");
}

/**
 * @brief .attach command handler
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandAttach(vector<CommandToken> CommandTokens, string Command)
{
    UINT32  TargetPid = 0;
    BOOLEAN NextIsPid = FALSE;

    //
    // It's a attach to a target PID
    //
    if (CommandTokens.size() >= 4)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandAttachHelp();
        return;
    }

    for (auto Section = CommandTokens.begin() + 1; Section != CommandTokens.end(); Section++)
    {
        //
        // Find out whether the user enters pid or not
        //
        if (NextIsPid)
        {
            NextIsPid = FALSE;

            if (!ConvertTokenToUInt32(*Section, &TargetPid))
            {
                ShowMessages("please specify a correct hex value for process id\n\n");
                CommandAttachHelp();
                return;
            }
        }
        else if (CompareLowerCaseStrings(*Section, "pid"))
        {
            //
            // next item is a pid for the process
            //
            NextIsPid = TRUE;
        }
        else
        {
            ShowMessages("unknown parameter at '%s'\n\n",
                         GetCaseSensitiveStringFromCommandToken(*Section).c_str());
            CommandAttachHelp();
            return;
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
    ShowMessages("Attaching to process 0x%llx (%lld)...\n\n", TargetPid, TargetPid);
    UdAttachToProcess(TargetPid, NULL, NULL, FALSE);
}
