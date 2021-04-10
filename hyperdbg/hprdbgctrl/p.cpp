/**
 * @file p.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief p command
 * @details
 * @version 0.1
 * @date 2020-12-29
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
 * @brief help of p command
 *
 * @return VOID
 */
VOID
CommandPHelp()
{
    ShowMessages(
        "p : executes a single instruction (step) and optionally displays the "
        "resulting values of all registers and flags.\n\n");
    ShowMessages("syntax : \tp[r] [count]\n");
    ShowMessages("\t\te.g : p\n");
    ShowMessages("\t\te.g : pr\n");
    ShowMessages("\t\te.g : pr 1f\n");
}

/**
 * @brief handler of p command
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandP(vector<string> SplittedCommand, string Command)
{
    UINT32                           StepCount;
    DEBUGGER_REMOTE_STEPPING_REQUEST RequestFormat;

    //
    // Validate the commands
    //
    if (SplittedCommand.size() != 1 && SplittedCommand.size() != 2)
    {
        ShowMessages("incorrect use of 'p'\n\n");
        CommandPHelp();
        return;
    }

    //
    // Set type of request
    //
    RequestFormat = DEBUGGER_REMOTE_STEPPING_REQUEST_STEP_OVER;

    //
    // Check if the command has a counter parameter
    //
    if (SplittedCommand.size() == 2)
    {
        if (!ConvertStringToUInt32(SplittedCommand.at(1), &StepCount))
        {
            ShowMessages("please specify a correct hex value for [count]\n\n");
            CommandPHelp();
            return;
        }
    }
    else
    {
        StepCount = 1;
    }

    //
    // Check if the remote serial debuggee is paused or not
    //
    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        for (size_t i = 0; i < StepCount; i++)
        {
            //
            // For logging purpose
            //
            // ShowMessages("percentage : %f %% (%x)\n", 100.0 * (i /
            //   (float)StepCount), i);
            //

            KdSendStepPacketToDebuggee(RequestFormat);

            if (!SplittedCommand.at(0).compare("pr"))
            {
                //
                // Show registers
                //
                ShowAllRegisters();
                if (i != StepCount - 1)
                {
                    ShowMessages("\n");
                }
            }
        }
    }
    else
    {
        ShowMessages("err, stepping (p) is not valid in the current context, you "
                     "should connect to a debuggee\n");
    }
}
