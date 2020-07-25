/**
 * @file g.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief g command
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

VOID CommandGHelp() {
  ShowMessages("g : continue a breaked debugger (by pause) or a remote breaked "
               "debugger.\n\n");
  ShowMessages("syntax : \tg\n");
}

VOID CommandG(vector<string> SplittedCommand) {

  if (SplittedCommand.size() != 1) {
    ShowMessages("incorrect use of 'g'\n\n");
    CommandGHelp();
    return;
  }

  //
  // Set the g_BreakPrintingOutput to TRUE
  //
  g_BreakPrintingOutput = FALSE;
}
