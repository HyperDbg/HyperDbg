/**
 * @file disconnect.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief disconnect command
 * @details
 * @version 0.1
 * @date 2020-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern bool g_IsConnectedToDebugger;
extern bool g_IsDebuggerModulesLoaded;

void CommandDisconnectHelp() {
  ShowMessages(".disconnect : disconnect from a debugging session (it won't "
               "unload the modules).\n\n");
  ShowMessages("syntax : \.disconnect\n");
}

void CommandDisconnect(vector<string> SplittedCommand) {

  if (SplittedCommand.size() != 1) {
    ShowMessages("incorrect use of '.disconnect'\n\n");
    CommandDisconnectHelp();
    return;
  }
  if (!g_IsConnectedToDebugger) {
    ShowMessages("You're not connected to any instance of HyperDbg, did you "
                 "use '.connect'? \n");
    return;
  }
  //
  // Disconnect the session
  //
  g_IsConnectedToDebugger = false;
  ShowMessages("successfully disconnected\n");
}
