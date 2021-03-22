/**
 * @file i.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief i command
 * @details
 * @version 0.1
 * @date 2021-03-19
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
 * @brief help of i command
 *
 * @return VOID
 */
VOID
CommandIHelp()
{
    ShowMessages(
        "i : executes a single instruction (step-in) and guarantees that no "
        "other instruction is executed other than the displayed instruction "
        "including user to the kernel (syscalls) and kernel to the user "
        "(sysrets) and exceptions and page-faults and optionally displays all "
        "registers and flags' resulting values.\n\n");

    ShowMessages("syntax : \ti[r] [count]\n");
    ShowMessages("\t\te.g : i\n");
    ShowMessages("\t\te.g : ir\n");
    ShowMessages("\t\te.g : ir 1f\n");
}

/**
 * @brief handler of i command
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandI(vector<string> SplittedCommand, string Command)
{
    UINT32                           StepCount;
    DEBUGGER_REMOTE_STEPPING_REQUEST RequestFormat;

    //
    // Validate the commands
    //
    if (SplittedCommand.size() != 1 && SplittedCommand.size() != 2)
    {
        ShowMessages("incorrect use of 'i'\n\n");
        CommandIHelp();
        return;
    }

    //
    // Set type of step
    //
    RequestFormat = DEBUGGER_REMOTE_STEPPING_REQUEST_STEP_IN_INSTRUMENT;

    //
    // Check if the command has a counter parameter
    //
    if (SplittedCommand.size() == 2)
    {
        if (!ConvertStringToUInt32(SplittedCommand.at(1), &StepCount))
        {
            ShowMessages("please specify a correct hex value for [count]\n\n");
            CommandIHelp();
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

            if (!SplittedCommand.at(0).compare("ir"))
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
