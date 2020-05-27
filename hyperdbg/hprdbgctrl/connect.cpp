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

bool g_IsConnectedToDebugger = false;
bool g_IsDebuggerModulesLoaded = false;

void CommandConnectHelp() {
  ShowMessages(".connect : connects to a remote or local machine to start "
               "debugging.\n\n");
  ShowMessages("syntax : \t.connect [ip] [port]\n");
  ShowMessages("\t\te.g : .connect 192.168.1.5 50000\n");
  ShowMessages("\t\te.g : .connect local\n");
}
void CommandConnect(vector<string> SplittedCommand) {

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
    g_IsConnectedToDebugger = true;

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
    g_IsConnectedToDebugger = true;

    return;
  } else {
    ShowMessages("incorrect use of '.connect'\n\n");
    CommandConnectHelp();
    return;
  }
}

void CommandLoadHelp() {
  ShowMessages("load : installs the driver and load the kernel modules.\n\n");
  ShowMessages("syntax : \tload\n");
}
void CommandLoad(vector<string> SplittedCommand) {

  if (SplittedCommand.size() != 1) {
    ShowMessages("incorrect use of 'load'\n\n");
    CommandLoadHelp();
    return;
  }
  if (!g_IsConnectedToDebugger) {
    ShowMessages("You're not connected to any instance of HyperDbg, did you "
                 "use '.connect'? \n");
    return;
  }
  ShowMessages("try to install driver...\n");
  if (HyperdbgInstallDriver()) {
    ShowMessages("Failed to install driver\n");
    return;
  }

  ShowMessages("try to install load kernel modules...\n");
  if (HyperdbgLoad()) {
    ShowMessages("Failed to load driver\n");
    return;
  }

  //
  // If we reach here so the module are loaded
  //
  g_IsDebuggerModulesLoaded = true;
}

void CommandUnloadHelp() {
  ShowMessages(
      "unload : unloads the kernel modules and uninstalls the drivers.\n\n");
  ShowMessages("syntax : \tunload\n");
}
void CommandUnload(vector<string> SplittedCommand) {

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

void CommandCpuHelp() {
  ShowMessages("cpu : collects a report from cpu features.\n\n");
  ShowMessages("syntax : \tcpu\n");
}
void CommandCpu(vector<string> SplittedCommand) {

  if (SplittedCommand.size() != 1) {
    ShowMessages("incorrect use of 'cpu'\n\n");
    CommandCpuHelp();
    return;
  }
  ReadCpuDetails();
}

void CommandExitHelp() {
  ShowMessages(
      "exit : unload and uninstalls the drivers and closes the debugger.\n\n");
  ShowMessages("syntax : \texit\n");
}
void CommandExit(vector<string> SplittedCommand) {

  if (SplittedCommand.size() != 1) {
    ShowMessages("incorrect use of 'exit'\n\n");
    CommandExitHelp();
    return;
  }

  //
  // unload and exit
  //
  if (g_IsDebuggerModulesLoaded) {
    HyperdbgUnload();

    //
    // Installing Driver
    //
    if (HyperdbgUninstallDriver()) {
      ShowMessages("Failed to uninstall driver\n");
    }
  }

  exit(0);
}

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
