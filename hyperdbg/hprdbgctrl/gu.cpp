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

    ShowMessages("syntax : \tg\n");
    ShowMessages("syntax : \tgur\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : gu\n");
    ShowMessages("\t\te.g : gur\n");
}

/**
 * @brief handler of gu command
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandGu(vector<string> SplittedCommand, string Command)
{
    UINT32                           CallInstructionSize;
    DEBUGGER_REMOTE_STEPPING_REQUEST RequestFormat;

    //
    // Validate the commands
    //
    if (SplittedCommand.size() != 1)
    {
        ShowMessages("incorrect use of the 'gu'\n\n");
        CommandGuHelp();
        return;
    }

    //
    // Set type of step
    //
    RequestFormat = DEBUGGER_REMOTE_STEPPING_REQUEST_STEP_OVER_FOR_GU;

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

        //
        // Send gu until the current instruction is ret
        //
        while (1)
        {
            if (HyperDbgCheckWhetherTheCurrentInstructionIsRet(
                    g_CurrentRunningInstruction,
                    MAXIMUM_INSTR_SIZE,
                    g_IsRunningInstruction32Bit ? FALSE : TRUE, // equals to !g_IsRunningInstruction32Bit
                    &CallInstructionSize))
            {
                break;
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
        }

        //
        // Send a step-in after the ret instruction if we are instrumenting instructions
        //
        if (g_IsInstrumentingInstructions)
        {
            RequestFormat = DEBUGGER_REMOTE_STEPPING_REQUEST_STEP_IN;

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

            if (!SplittedCommand.at(0).compare("gur"))
            {
                //
                // Show registers
                //
                ShowAllRegisters();
            }
        }

        //
        // We're not instrumenting instructions anymore
        //
        g_IsInstrumentingInstructions = FALSE;
    }
    else
    {
        ShowMessages("err, stepping (gu) is not valid in the current context, you "
                     "should connect to a debuggee\n");
    }
}
