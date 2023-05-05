/**
 * @file track.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief !track command
 * @details
 * @version 0.3
 * @date 2023-05-05
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
extern BOOLEAN                  g_AddressConversion;

/**
 * @brief help of !track command
 *
 * @return VOID
 */
VOID
CommandTrackHelp()
{
    ShowMessages(
        "!track : tracks instructions from user-mode to kernel-mode or kernel-mode to user-mode "
        "to create call tree. Please note that it's highly recommended to configure symbols before "
        "using this command as it maps addresses to corresponding function names.\n\n");

    ShowMessages("syntax : \t!track [tree] [Count (hex)]\n");
    ShowMessages("syntax : \ttrack [reg] [Count (hex)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !track tree 10000\n");
    ShowMessages("\t\te.g : !track reg 10000\n");
}

/**
 * @brief handler of !track command
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandTrack(vector<string> SplittedCommand, string Command)
{
    UINT32                           StepCount;
    DEBUGGER_REMOTE_STEPPING_REQUEST RequestFormat;
    BOOLEAN                          ShowRegs = FALSE;

    //
    // Validate the commands
    //
    if (SplittedCommand.size() >= 4)
    {
        ShowMessages("incorrect use of '!track'\n\n");
        CommandTrackHelp();
        return;
    }

    //
    // Check if we're in VMI mode
    //
    if (g_ActiveProcessDebuggingState.IsActive)
    {
        ShowMessages("the tracking mechanism is only supported in Debugger Mode\n");
        return;
    }

    //
    // Set default of stepping (tracking)
    //
    RequestFormat = DEBUGGER_REMOTE_STEPPING_REQUEST_INSTRUMENTATION_STEP_IN_FOR_TRACKING;
    StepCount     = DEBUGGER_REMOTE_TRACKING_DEFAULT_COUNT_OF_STEPPING;

    //
    // Check parameters
    //
    for (auto Section : SplittedCommand)
    {
        if (!Section.compare("!track") || !Section.compare("track"))
        {
            continue;
        }

        //
        // check if the second param is a number
        //
        if (ConvertStringToUInt32(Section, &StepCount))
        {
            continue;
        }
        else if (Section.compare("tree"))
        {
            //
            // Default
            //
            ShowRegs = FALSE;
        }
        else if (Section.compare("reg"))
        {
            ShowRegs = TRUE;
        }
        else
        {
            ShowMessages("err, couldn't resolve error at '%s'\n\n", Section.c_str());
            return;
        }
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

            //
            // Check if it's a 'call' or 'ret' instructions
            //

            if (ShowRegs)
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
        ShowMessages("err, tracking is not valid in the current context, you "
                     "should connect to a debuggee\n");
    }
}

/**
 * @brief Handle received 'call' or 'ret'
 *
 * @param BufferToDisassemble
 * @param BuffLength
 * @param Isx86_64
 * @param RipAddress
 *
 * @return VOID
 */
VOID
CommandTrackHandleReceivedInstructions(unsigned char * BufferToDisassemble,
                                       UINT64          BuffLength,
                                       BOOLEAN         Isx86_64,
                                       UINT64          RipAddress)
{
    BOOLEAN IsRet           = FALSE;
    UINT64  UsedBaseAddress = NULL;

    if (HyperDbgCheckWhetherTheCurrentInstructionIsCallOrRet(BufferToDisassemble, BuffLength, Isx86_64, &IsRet))
    {
        if (!IsRet)
        {
            //
            // Apply addressconversion of settings here
            //
            if (g_AddressConversion)
            {
                //
                // Showing function names here
                //
                if (SymbolShowFunctionNameBasedOnAddress(RipAddress, &UsedBaseAddress))
                {
                    //
                    // The symbol address is showed
                    //
                    ShowMessages(":\n");
                }
            }

            // ZYAN_PRINTF("%016" PRIX64 "  ", runtime_address);
            ShowMessages("%s   ", SeparateTo64BitValue(RipAddress).c_str());
        }
        else
        {
            ShowMessages("ret detected at %llx\n", RipAddress);
        }
    }
}
