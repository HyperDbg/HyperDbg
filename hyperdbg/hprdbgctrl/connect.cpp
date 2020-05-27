/**
 * @file connect.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief connect command
 * @details
 * @version 0.1
 * @date 2020-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables Header
//
#include "globals.h"

//
// Global Variables
//
extern BOOLEAN g_IsConnectedToDebugger;
extern BOOLEAN g_IsDebuggerModulesLoaded;

VOID CommandConnectHelp() {
  ShowMessages(".connect : connects to a remote or local machine to start "
               "debugging.\n\n");
  ShowMessages("syntax : \t.connect [ip] [port]\n");
  ShowMessages("\t\te.g : .connect 192.168.1.5 50000\n");
  ShowMessages("\t\te.g : .connect local\n");
}

VOID CommandConnect(vector<string> SplittedCommand) {

  if (SplittedCommand.size() == 1) {
    //
    // Means that user entered just a connect so we have to
    // ask to connect to what ?
    //
    ShowMessages("incorrect use of '.connect'\n\n");
    CommandConnectHelp();
    return;
  } else if (SplittedCommand.at(1) == "local" && SplittedCommand.size() == 2) {
    //
    // connect to local debugger
    //
    ShowMessages("local debug current system\n");
    g_IsConnectedToDebugger = TRUE;

    return;
  } else if (SplittedCommand.size() == 3) {

    string ip = SplittedCommand.at(1);
    string port = SplittedCommand.at(2);

    //
    // means that probably wants to connect to a remote
    // system, let's first check the if the parameters are
    // valid
    //
    if (!ValidateIP(ip)) {
      ShowMessages("incorrect ip address\n");
      return;
    }
    if (!IsNumber(port) || stoi(port) > 65535 || stoi(port) < 0) {
      ShowMessages("incorrect port\n");
      return;
    }

    //
    // connect to remote debugger
    //
    ShowMessages("local debug remote system\n");
    g_IsConnectedToDebugger = TRUE;

    return;
  } else {
    ShowMessages("incorrect use of '.connect'\n\n");
    CommandConnectHelp();
    return;
  }
}
