/**
 * @file p.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
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
extern BOOLEAN                  g_IsSerialConnectedToRemoteDebuggee;
extern BOOLEAN                  g_IsInstrumentingInstructions;
extern ACTIVE_DEBUGGING_PROCESS g_ActiveProcessDebuggingState;

/**
 * @brief help of the p command
 *
 * @return VOID
 */
VOID
CommandPHelp()
{
    ShowMessages(
        "p : executes a single instruction (step) and optionally displays the "
        "resulting values of all registers and flags.\n\n");

    ShowMessages("syntax : \tp\n");
    ShowMessages("syntax : \tp [Count (hex)]\n");
    ShowMessages("syntax : \tpr\n");
    ShowMessages("syntax : \tpr [Count (hex)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : p\n");
    ShowMessages("\t\te.g : pr\n");
    ShowMessages("\t\te.g : pr 1f\n");
}

/**
 * @brief handler of p command
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandP(vector<CommandToken> CommandTokens, string Command)
{
    UINT32                           StepCount;
    DEBUGGER_REMOTE_STEPPING_REQUEST RequestFormat;

    //
    // Validate the commands
    //
    if (CommandTokens.size() != 1 && CommandTokens.size() != 2)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
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
    if (CommandTokens.size() == 2)
    {
        if (!ConvertTokenToUInt32(CommandTokens.at(1), &StepCount))
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
    // Check if the remote serial debuggee or user debugger are paused or not
    //
    if (g_IsSerialConnectedToRemoteDebuggee || g_ActiveProcessDebuggingState.IsActive)
    {
        //
        // Check if the thread is paused or not
        //
        if (g_ActiveProcessDebuggingState.IsActive && !g_ActiveProcessDebuggingState.IsPaused)
        {
            ShowMessages("the target process is running, use the "
                         "'pause' command or press CTRL+C to pause the process\n");
            return;
        }

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

            if (g_IsSerialConnectedToRemoteDebuggee)
            {
                //
                // It's stepping over serial connection in kernel debugger
                //
                KdSendStepPacketToDebuggee(RequestFormat);
            }
            else
            {
                //
                // It's stepping over user debugger
                //
                UdSendStepPacketToDebuggee(g_ActiveProcessDebuggingState.ProcessDebuggingToken,
                                           g_ActiveProcessDebuggingState.ThreadId,
                                           RequestFormat);
            }

            if (CompareLowerCaseStrings(CommandTokens.at(0), "pr"))
            {
                //
                // Show registers
                //
                HyperDbgRegisterShowAll();

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
        ShowMessages("err, stepping (p) is not valid in the current context, you "
                     "should connect to a debuggee\n");
    }
}
