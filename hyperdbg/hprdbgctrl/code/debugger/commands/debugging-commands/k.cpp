/**
 * @file k.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief k command
 * @details
 * @version 0.1
 * @date 2021-12-13
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;
extern BOOLEAN g_IsRunningInstruction32Bit;

/**
 * @brief help of k command
 *
 * @return VOID
 */
VOID
CommandKHelp()
{
    ShowMessages(
        "k, kd, kq : shows the callstack of the thread.\n\n");

    ShowMessages("syntax : \tk\n");
    ShowMessages("syntax : \tkd\n");
    ShowMessages("syntax : \tkq\n");
    ShowMessages("syntax : \tk [base StackAddress (hex)] [l Length (hex)]\n");
    ShowMessages("syntax : \tkd [base StackAddress (hex)] [l Length (hex)]\n");
    ShowMessages("syntax : \tkq [base StackAddress (hex)] [l Length (hex)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : k\n");
    ShowMessages("\t\te.g : k l 100\n");
    ShowMessages("\t\te.g : kd base 0x77356f010\n");
    ShowMessages("\t\te.g : kq base fffff8077356f010\n");
    ShowMessages("\t\te.g : kq base @rbx-10\n");
    ShowMessages("\t\te.g : kq base fffff8077356f010 l 100\n");
}

/**
 * @brief k command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandK(vector<string> SplittedCommand, string Command)
{
    UINT64         BaseAddress = NULL;  // Null base address means current RSP register
    UINT32         Length      = 0x100; // Default length
    vector<string> SplittedCommandCaseSensitive {Split(Command, ' ')};
    UINT32         IndexInCommandCaseSensitive = 0;
    BOOLEAN        IsFirstCommand              = TRUE;
    BOOLEAN        IsNextBase                  = FALSE;
    BOOLEAN        IsNextLength                = FALSE;

    string FirstCommand = SplittedCommand.front();

    if (SplittedCommand.size() >= 6)
    {
        ShowMessages("incorrect use of '%s'\n\n", FirstCommand.c_str());
        CommandKHelp();
        return;
    }

    if (!g_IsSerialConnectedToRemoteDebuggee)
    {
        ShowMessages("err, tracing callstack is not possible when you're not "
                     "connected to a debuggee\n");
        return;
    }

    //
    // Set the default length
    //
    if (g_IsRunningInstruction32Bit)
    {
        Length = 0x100;
    }
    else
    {
        Length = 0x200;
    }

    for (auto Section : SplittedCommand)
    {
        IndexInCommandCaseSensitive++;

        if (IsFirstCommand)
        {
            IsFirstCommand = FALSE;
            continue;
        }
        if (IsNextBase == TRUE)
        {
            if (!SymbolConvertNameOrExprToAddress(SplittedCommandCaseSensitive.at(IndexInCommandCaseSensitive - 1),
                                                  &BaseAddress))
            {
                //
                // Couldn't resolve or unkonwn parameter
                //
                ShowMessages("err, couldn't resolve error at '%s'\n",
                             SplittedCommandCaseSensitive.at(IndexInCommandCaseSensitive - 1).c_str());
                return;
            }

            IsNextBase = FALSE;
            continue;
        }

        if (IsNextLength == TRUE)
        {
            if (!ConvertStringToUInt32(Section, &Length))
            {
                ShowMessages("err, you should enter a valid length\n\n");
                return;
            }
            IsNextLength = FALSE;
            continue;
        }

        if (!Section.compare("l"))
        {
            IsNextLength = TRUE;
            continue;
        }

        if (!Section.compare("base"))
        {
            IsNextBase = TRUE;
            continue;
        }

        //
        // User inserts unexpected input
        //
        ShowMessages("err, incorrect use of '%s' command\n\n",
                     FirstCommand.c_str());
        CommandKHelp();

        return;
    }

    if (IsNextLength || IsNextBase)
    {
        ShowMessages("incorrect use of '%s' command\n\n", FirstCommand.c_str());
        CommandKHelp();
        return;
    }

    //
    // Send callstack request
    //

    if (!FirstCommand.compare("k"))
    {
        KdSendCallStackPacketToDebuggee(BaseAddress,
                                        Length,
                                        DEBUGGER_CALLSTACK_DISPLAY_METHOD_WITHOUT_PARAMS,
                                        g_IsRunningInstruction32Bit);
    }
    else if (!FirstCommand.compare("kq"))
    {
        KdSendCallStackPacketToDebuggee(BaseAddress,
                                        Length,
                                        DEBUGGER_CALLSTACK_DISPLAY_METHOD_WITH_PARAMS,
                                        FALSE);
    }
    else if (!FirstCommand.compare("kd"))
    {
        KdSendCallStackPacketToDebuggee(BaseAddress,
                                        Length,
                                        DEBUGGER_CALLSTACK_DISPLAY_METHOD_WITH_PARAMS,
                                        TRUE);
    }
}
