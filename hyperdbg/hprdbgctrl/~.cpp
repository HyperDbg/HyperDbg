/**
 * @file ~.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief show and change processor
 * @details
 * @version 0.1
 * @date 2021-01-30
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern ULONG g_CurrentRemoteCore;
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;

/**
 * @brief help of ~ command
 *
 * @return VOID
 */
VOID CommandTildeHelp() {
  ShowMessages("~ : show and change the operating processor.\n\n");
  ShowMessages("syntax : \t~\n");
}

/**
 * @brief ~ command handler
 *
 * @param SplittedCommand
 * @return VOID
 */
VOID CommandTilde(vector<string> SplittedCommand) {

  if (SplittedCommand.size() != 1) {
    ShowMessages("incorrect use of '~'\n\n");
    CommandTildeHelp();
    return;
  }

  //
  // Check if it's connected to a remote debuggee or not
  //
  if (!g_IsSerialConnectedToRemoteDebuggee) {
    ShowMessages("err, you're not connected to any debuggee.\n");
    return;
  }

  ShowMessages("current processor : 0x%x\n", g_CurrentRemoteCore);
}
