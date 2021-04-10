/**
 * @file t.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief t command
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
 * @brief help of t command
 *
 * @return VOID
 */
VOID
CommandTHelp()
{
    ShowMessages(
        "t : executes a single instruction (step-in) and optionally displays the "
        "resulting values of all registers and flags.\n\n");
    ShowMessages("syntax : \tt[r] [count]\n");
    ShowMessages("\t\te.g : t\n");
    ShowMessages("\t\te.g : tr\n");
    ShowMessages("\t\te.g : tr 1f\n");
}

/**
 * @brief handler of t command
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandT(vector<string> SplittedCommand, string Command)
{
    UINT32                           StepCount;
    DEBUGGER_REMOTE_STEPPING_REQUEST RequestFormat;

    //
    // Validate the commands
    //
    if (SplittedCommand.size() != 1 && SplittedCommand.size() != 2)
    {
        ShowMessages("incorrect use of 't'\n\n");
        CommandTHelp();
        return;
    }

    //
    // Set type of step
    //
    RequestFormat = DEBUGGER_REMOTE_STEPPING_REQUEST_STEP_IN;

    //
    // Check if the command has a counter parameter
    //
    if (SplittedCommand.size() == 2)
    {
        if (!ConvertStringToUInt32(SplittedCommand.at(1), &StepCount))
        {
            ShowMessages("please specify a correct hex value for [count]\n\n");
            CommandTHelp();
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
            KdSendStepPacketToDebuggee(RequestFormat);

            if (!SplittedCommand.at(0).compare("tr"))
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
        ShowMessages("err, stepping (t) is not valid in the current context, you "
                     "should connect to a debuggee\n");
    }
}
