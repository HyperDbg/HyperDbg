/**
 * @file unload.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief unload command
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
extern BOOLEAN g_IsConnectedToDebugger;
extern BOOLEAN g_IsDebuggerModulesLoaded;

VOID CommandUnloadHelp() {
  ShowMessages(
      "unload : unloads the kernel modules and uninstalls the drivers.\n\n");
  ShowMessages("syntax : \tunload\n");
}

VOID CommandUnload(vector<string> SplittedCommand) {

  if (SplittedCommand.size() != 1) {
    ShowMessages("incorrect use of 'unload'\n\n");
    CommandLoadHelp();
    return;
  }
  if (!g_IsConnectedToDebugger) {
    ShowMessages("You're not connected to any instance of HyperDbg, did you "
                 "use '.connect'? \n");
    return;
  }

  if (g_IsDebuggerModulesLoaded) {
    HyperdbgUnload();

    //
    // Installing Driver
    //
    if (HyperdbgUninstallDriver()) {
      ShowMessages("Failed to uninstall driver\n");
    }
  } else {
    ShowMessages("there is nothing to unload\n");
  }
}
