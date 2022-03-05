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
#include "..\hprdbgctrl\pch.h"

/**
 * @brief help of k command
 *
 * @return VOID
 */
VOID
CommandKHelp()
{
    ShowMessages(
        "k : shows the callstack of the thread.\n\n");

    ShowMessages("syntax : \tk\n");
    ShowMessages("syntax : \tk [base StackAddress (hex)] [size Size (hex)] [mode Mode (string)\n");
    ShowMessages("\t\te.g : k\n");
    ShowMessages("\t\te.g : k size 100\n");
    ShowMessages("\t\te.g : k base fffff8077356f010\n");
    ShowMessages("\t\te.g : k base fffff8077356f010 size 100\n");
    ShowMessages("\t\te.g : k base 0x77356f010 mode 32\n");
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
    KdSendCallStackPacketToDebuggee(NULL, 0x180, FALSE);
}
