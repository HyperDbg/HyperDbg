/**
 * @file i.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
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
extern BOOLEAN                  g_IsSerialConnectedToRemoteDebuggee;
extern BOOLEAN                  g_IsInstrumentingInstructions;
extern ACTIVE_DEBUGGING_PROCESS g_ActiveProcessDebuggingState;

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

    ShowMessages("syntax : \ti\n");
    ShowMessages("syntax : \ti [Count (hex)]\n");
    ShowMessages("syntax : \tir\n");
    ShowMessages("syntax : \tir [Count (hex)]\n");

    ShowMessages("\n");
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
    // Check if we're in VMI mode
    //
    if (g_ActiveProcessDebuggingState.IsActive)
    {
        ShowMessages("the instrumentation step-in is only supported in Debugger Mode\n");
        return;
    }

    //
    // Set type of step
    //
    RequestFormat = DEBUGGER_REMOTE_STEPPING_REQUEST_INSTRUMENTATION_STEP_IN;

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
    // Check if the remote serial debuggee or user debugger are paused or not
    //
    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // Indicate that we're instrumenting
        //
        g_IsInstrumentingInstructions = TRUE;

        for (size_t i = 0; i < StepCount; i++)
        {
            //
            // For logging purpose
            //
            // ShowMessages("percentage : %f %% (%x)\n", 100.0 * (i /
            //   (float)StepCount), i);
            //

            //
            // It's stepping over serial connection in kernel debugger
            //
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

            //
            // Check if user pressed CTRL+C
            //
            if (!g_IsInstrumentingInstructions)
            {
                break;
            }
        }

        //
        // We're not instrumenting instructions anymore
        //
        g_IsInstrumentingInstructions = FALSE;
    }
    else
    {
        ShowMessages("err, stepping (i) is not valid in the current context, you "
                     "should connect to a debuggee\n");
    }
}
