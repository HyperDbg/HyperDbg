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

//
// Local (global) variables
//
UINT32           NumberOfCallsIdentation  = 0;
BOOLEAN          IsCallInstructionVisited = FALSE;
BOOLEAN          ShowRegs                 = FALSE;
volatile BOOLEAN RequestShowingRegs       = FALSE;

/**
 * @brief help of the !track command
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
    ShowMessages("syntax : \t!track [reg] [Count (hex)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !track tree 10000\n");
    ShowMessages("\t\te.g : !track reg 10000\n");
}

/**
 * @brief handler of !track command
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandTrack(vector<CommandToken> CommandTokens, string Command)
{
    UINT32 StepCount;
    string SymbolServer;

    //
    // Validate the commands
    //
    if (CommandTokens.size() >= 4)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
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
    // Show recommendation
    //
    if (!CommandSettingsGetValueFromConfigFile("SymbolServer", SymbolServer))
    {
        ShowMessages("it is recommended to configure the symbol path '.sympath' and load "
                     "symbols before using the '!track' command command to obtain results with function "
                     "names\n");
    }

    //
    // Reset the details of indentation and regs
    //
    NumberOfCallsIdentation  = 0;
    IsCallInstructionVisited = FALSE;
    ShowRegs                 = FALSE;
    RequestShowingRegs       = FALSE;

    //
    // Set default of stepping (tracking)
    //
    StepCount = DEBUGGER_REMOTE_TRACKING_DEFAULT_COUNT_OF_STEPPING;

    //
    // Check parameters
    //
    for (auto Section : CommandTokens)
    {
        if (CompareLowerCaseStrings(Section, "!track") || CompareLowerCaseStrings(Section, "track"))
        {
            continue;
        }

        //
        // check if the second param is a number
        //
        if (ConvertTokenToUInt32(Section, &StepCount))
        {
            continue;
        }
        else if (CompareLowerCaseStrings(Section, "tree"))
        {
            //
            // Default
            //
            ShowRegs = FALSE;
        }
        else if (CompareLowerCaseStrings(Section, "reg"))
        {
            ShowRegs = TRUE;
        }
        else
        {
            ShowMessages("err, couldn't resolve error at '%s'\n\n",
                         GetCaseSensitiveStringFromCommandToken(Section).c_str());
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

        for (SIZE_T i = 0; i < StepCount; i++)
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
            SteppingInstrumentationStepInForTracking();

            if (ShowRegs && RequestShowingRegs)
            {
                RequestShowingRegs = FALSE;

                //
                // Show registers
                //
                HyperDbgRegisterShowAll();

                ShowMessages("\n");
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
CommandTrackHandleReceivedInstructions(UCHAR * BufferToDisassemble,
                                       UINT32  BuffLength,
                                       BOOLEAN Isx86_64,
                                       UINT64  RipAddress)
{
    BOOLEAN IsRet = FALSE;

    //
    // By calling this function, it acts as a callback that in case of the 'call' and the 'ret' instructions
    // the callbacks will be called
    //
    HyperDbgCheckWhetherTheCurrentInstructionIsCallOrRet(BufferToDisassemble, RipAddress, BuffLength, Isx86_64, &IsRet);
}

/**
 * @brief Handle received 'call'
 *
 * @param NameOfFunctionFromSymbols
 * @param ComputedAbsoluteAddress
 *
 * @return VOID
 */
VOID
CommandTrackHandleReceivedCallInstructions(const CHAR * NameOfFunctionFromSymbols,
                                           UINT64       ComputedAbsoluteAddress)
{
    //
    // One 'call' instruction is visited
    //
    IsCallInstructionVisited = TRUE;

    CHAR Utf8String1[] = "\xE2\x94\x82\x20\x20";

    for (SIZE_T i = 0; i < NumberOfCallsIdentation; i++)
    {
        PlatformWriteConsole(Utf8String1, sizeof(Utf8String1) - 1);
    }

    //
    // Write the UTF-8 encoded character sequence to the console
    //
    // CHAR Utf8String[] = "\xE2\x94\x9C\xE2\x94\x80\xE2\x94\x80";
    CHAR Utf8String[] = "\xE2\x94\x8C\xE2\x94\x80\xE2\x94\x80";
    PlatformWriteConsole(Utf8String, sizeof(Utf8String) - 1);

    if (NameOfFunctionFromSymbols != NULL)
    {
        ShowMessages(" %s (%s)\n", NameOfFunctionFromSymbols, SeparateTo64BitValue(ComputedAbsoluteAddress).c_str());
    }
    else
    {
        ShowMessages(" %s\n", SeparateTo64BitValue(ComputedAbsoluteAddress).c_str());
    }

    if (ShowRegs)
    {
        RequestShowingRegs = TRUE;
    }

    NumberOfCallsIdentation++;
}

/**
 * @brief Handle received 'ret'
 *
 * @param CurrentRip
 *
 * @return VOID
 */
VOID
CommandTrackHandleReceivedRetInstructions(UINT64 CurrentRip)
{
    UINT64  UsedBaseAddress = NULL;
    BOOLEAN IsNameShowed    = FALSE;

    if (IsCallInstructionVisited)
    {
        if (NumberOfCallsIdentation != 0)
        {
            NumberOfCallsIdentation--;
        }
    }

    CHAR Utf8String1[] = "\xE2\x94\x82\x20\x20";

    for (SIZE_T i = 0; i < NumberOfCallsIdentation; i++)
    {
        PlatformWriteConsole(Utf8String1, sizeof(Utf8String1) - 1);
    }

    //
    // Write the UTF-8 encoded character sequence to the console
    //
    CHAR Utf8String[] = "\xE2\x94\x94\xE2\x94\x80\xE2\x94\x80\x20";
    PlatformWriteConsole(Utf8String, sizeof(Utf8String) - 1);

    //
    // Apply addressconversion of settings here
    //
    if (g_AddressConversion)
    {
        //
        // Showing function names here
        //
        if (SymbolShowFunctionNameBasedOnAddress(CurrentRip, &UsedBaseAddress))
        {
            //
            // The symbol address is showed
            //
            IsNameShowed = TRUE;
            ShowMessages(" (%s)\n", SeparateTo64BitValue(CurrentRip).c_str());
        }
    }

    if (!IsNameShowed)
    {
        ShowMessages("%s \n", SeparateTo64BitValue(CurrentRip).c_str());
    }
}
