/**
 * @file pause.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief pause command
 * @details
 * @version 0.1
 * @date 2020-07-25
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern BOOLEAN g_BreakPrintingOutput;

VOID CommandPauseHelp() {
  ShowMessages("pause : pauses the kernel events.\n\n");
  ShowMessages("syntax : \tpause\n");
}

VOID CommandPause(vector<string> SplittedCommand) {

  if (SplittedCommand.size() != 1) {
    ShowMessages("incorrect use of 'pause'\n\n");
    CommandPauseHelp();
    return;
  }

  //
  // Set the g_BreakPrintingOutput to TRUE
  //
  g_BreakPrintingOutput = TRUE;

  //
  // Sleep because the other thread that shows must be stopped
  //
  Sleep(500);

  ShowMessages("\npause\npausing debugger...\n");
}
