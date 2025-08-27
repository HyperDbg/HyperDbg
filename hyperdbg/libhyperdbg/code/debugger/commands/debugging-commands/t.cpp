/**
 * @file t.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
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
extern BOOLEAN                  g_IsSerialConnectedToRemoteDebuggee;
extern BOOLEAN                  g_IsInstrumentingInstructions;
extern ACTIVE_DEBUGGING_PROCESS g_ActiveProcessDebuggingState;

/**
 * @brief help of the t command
 *
 * @return VOID
 */
VOID
CommandTHelp()
{
    ShowMessages(
        "t : executes a single instruction (step-in) and optionally displays the "
        "resulting values of all registers and flags.\n\n");

    ShowMessages("syntax : \tt\n");
    ShowMessages("syntax : \tt [Count (hex)]\n");
    ShowMessages("syntax : \ttr\n");
    ShowMessages("syntax : \ttr [Count (hex)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : t\n");
    ShowMessages("\t\te.g : tr\n");
    ShowMessages("\t\te.g : tr 1f\n");
}

/**
 * @brief handler of t command
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandT(vector<CommandToken> CommandTokens, string Command)
{
    UINT32 StepCount;

    //
    // Validate the commands
    //
    if (CommandTokens.size() != 1 && CommandTokens.size() != 2)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandTHelp();
        return;
    }

    //
    // Check if the command has a counter parameter
    //
    if (CommandTokens.size() == 2)
    {
        if (!ConvertTokenToUInt32(CommandTokens.at(1), &StepCount))
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
                         "'pause' command to pause the process\n");
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

            //
            // Instrument (regular) the instruction
            //
            SteppingRegularStepIn();

            if (CompareLowerCaseStrings(CommandTokens.at(0), "tr"))
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
        ShowMessages("err, stepping (t) is not valid in the current context, you "
                     "should connect to a debuggee\n");
    }
}
