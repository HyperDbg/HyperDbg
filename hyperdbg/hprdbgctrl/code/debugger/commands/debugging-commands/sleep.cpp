/**
 * @file sleep.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief sleep command
 * @details
 * @version 0.1
 * @date 2020-07-25
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of the sleep command
 *
 * @return VOID
 */
VOID
CommandSleepHelp()
{
    ShowMessages("sleep : sleeps the debugger; this command is used in scripts, it doesn't breaks "
                 "the debugger but the debugger still shows the buffers received "
                 "from kernel.\n\n");

    ShowMessages("syntax : \tsleep [MillisecondsTime (hex)]\n");
}

/**
 * @brief sleep command help
 *
 * @param SplitCommand
 * @param Command
 * @return VOID
 */
VOID
CommandSleep(vector<string> SplitCommand, string Command)
{
    UINT32 MillisecondsTime = 0;

    if (SplitCommand.size() != 2)
    {
        ShowMessages("incorrect use of the 'sleep'\n\n");
        CommandSleepHelp();
        return;
    }

    if (!ConvertStringToUInt32(SplitCommand.at(1), &MillisecondsTime))
    {
        ShowMessages(
            "please specify a correct hex value for time (milliseconds)\n\n");
        CommandSleepHelp();
        return;
    }
    Sleep(MillisecondsTime);
}
