/**
 * @file sleep.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief sleep command
 * @details
 * @version 0.1
 * @date 2020-07-25
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

VOID CommandSleepHelp() {
  ShowMessages("sleep : sleep command is used in scripts, it doesn't breaks "
               "the debugger but the debugger still shows the buffers received "
               "from kernel.\n\n");
  ShowMessages("syntax : \sleep [time - milliseconds (hex value)]\n");
}

VOID CommandSleep(vector<string> SplittedCommand) {
  UINT32 MillisecondsTime = 0;

  if (SplittedCommand.size() != 2) {
    ShowMessages("incorrect use of 'sleep'\n\n");
    CommandSleepHelp();
    return;
  }

  if (!ConvertStringToUInt32(SplittedCommand.at(1), &MillisecondsTime)) {
    ShowMessages(
        "please specify a correct hex value for time (milliseconds)\n\n");
    CommandSleepHelp();
    return;
  }
  Sleep(MillisecondsTime);
}
