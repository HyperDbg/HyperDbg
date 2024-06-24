/**
 * @file gu.cpp
 * @author xmaple555
 * @brief gu command
 * @details
 * @version 0.6
 * @date 2023-09-06
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
extern BYTE                     g_CurrentRunningInstruction[MAXIMUM_INSTR_SIZE];
extern BOOLEAN                  g_IsRunningInstruction32Bit;

/**
 * @brief help of the gu command
 *
 * @return VOID
 */
VOID
CommandGuHelp()
{
    ShowMessages(
        "gu : executes a single instruction (step-out) and optionally displays the "
        "resulting values of all registers and flags.\n\n");

    ShowMessages("syntax : \tgu\n");
    ShowMessages("syntax : \tgu [Count (hex)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : gu\n");
    ShowMessages("\t\te.g : gu 10000\n");
}

/**
 * @brief handler of gu command
 *
 * @param SplitCommand
 * @param Command
 * @return VOID
 */
VOID
CommandGu(vector<string> SplitCommand, string Command)
{
    UINT32                           StepCount;
    DEBUGGER_REMOTE_STEPPING_REQUEST RequestFormat;
    BOOLEAN                          BreakOnNextInstruction = FALSE;

    //
    // Validate the commands
    //
    if (SplitCommand.size() != 1 && SplitCommand.size() != 2)
    {
        ShowMessages("incorrect use of the 'gu'\n\n");
        CommandGuHelp();
        return;
    }

    //
    // Set type of request
    //
    RequestFormat = DEBUGGER_REMOTE_STEPPING_REQUEST_STEP_OVER_FOR_GU;

    //
    // Check if the command has a counter parameter
    //
    if (SplitCommand.size() == 2)
    {
        if (!ConvertStringToUInt32(SplitCommand.at(1), &StepCount))
        {
            ShowMessages("please specify a correct hex value for [count]\n\n");
            CommandGuHelp();
            return;
        }
    }
    else
    {
        StepCount = DEBUGGER_REMOTE_TRACKING_DEFAULT_COUNT_OF_STEPPING;
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

            //
            // Check if the current instruction is 'ret' or not
            //
            if (HyperDbgCheckWhetherTheCurrentInstructionIsRet(
                    g_CurrentRunningInstruction,
                    MAXIMUM_INSTR_SIZE,
                    g_IsRunningInstruction32Bit ? FALSE : TRUE // equals to !g_IsRunningInstruction32Bit
                    ))
            {
                BreakOnNextInstruction = TRUE;

                //
                // It's the last instruction, so we gonna show the instruction
                //
                RequestFormat = DEBUGGER_REMOTE_STEPPING_REQUEST_STEP_OVER_FOR_GU_LAST_INSTRUCTION;
            }

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

            //
            // Check if user pressed CTRL+C
            //
            if (!g_IsInstrumentingInstructions)
            {
                break;
            }

            //
            // Check if we see 'ret' in the previous instruction or not
            //
            if (BreakOnNextInstruction)
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
        ShowMessages("err, going up (gu) is not valid in the current context, you "
                     "should connect to a debuggee\n");
    }
}
