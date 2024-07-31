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
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandSleep(vector<CommandToken> CommandTokens, string Command)
{
    UINT32 MillisecondsTime = 0;

    if (CommandTokens.size() != 2)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandSleepHelp();
        return;
    }

    if (!ConvertTokenToUInt32(CommandTokens.at(1), &MillisecondsTime))
    {
        ShowMessages(
            "please specify a correct hex value for time (milliseconds)\n\n");
        CommandSleepHelp();
        return;
    }
    Sleep(MillisecondsTime);
}
