/**
 * @file interpreter.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief The hyperdbg command interpreter and driver connector
 * @details
 * @version 0.1
 * @date 2020-04-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

using namespace std;

//
// Global Variables
//
extern DEBUGGING_STATE g_DebuggingState;
extern BOOLEAN g_IsCommandListInitialized;
extern CommandType g_CommandList;
extern BOOLEAN g_LogOpened;
extern BOOLEAN g_ExecutingScript;
extern BOOLEAN g_IsConnectedToHyperDbgLocally;
extern BOOLEAN g_IsConnectedToRemoteDebuggee;
extern BOOLEAN g_IsRemoteDebuggerMessageReceived;
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;
extern BOOLEAN g_IsDebuggeeRunning;
extern string g_ServerPort;
extern string g_ServerIp;

/**
 * @brief Interpret commands
 *
 * @param Command The text of command
 * @return int returns return zero if it was successful or non-zero if there was
 * error
 */
int HyperdbgInterpreter(const char *Command) {

  string CommandString(Command);
  BOOLEAN HelpCommand = FALSE;
  CommandType::iterator Iterator;

  //
  // Check if it's the first command and whether the mapping of command is
  // initialized or not
  //
  if (!g_IsCommandListInitialized) {

    //
    // Initialize the mapping of functions
    //
    InitializeCommandsDictionary();

    g_IsCommandListInitialized = TRUE;
  }
  //
  // Save the command into log open file
  //
  if (g_LogOpened && !g_ExecutingScript) {
    LogopenSaveToFile("HyperDbg> ");
    LogopenSaveToFile(Command);
    LogopenSaveToFile("\n");
  }

  //
  // Convert to lower case
  //
  transform(CommandString.begin(), CommandString.end(), CommandString.begin(),
            [](unsigned char c) { return std::tolower(c); });

  vector<string> SplittedCommand{Split(CommandString, ' ')};

  //
  // Check if user entered an empty imput
  //
  if (SplittedCommand.empty()) {
    ShowMessages("\n");
    return 0;
  }

  string FirstCommand = SplittedCommand.front();

  //
  // Check and send remote command and also we check whether this
  // is a command that should be handled in this command or we can
  // send it to the remote computer, it is because in a remote connection
  // still some of the commands should be handled in the local HyperDbg
  //
  if ((g_IsConnectedToRemoteDebuggee || g_IsSerialConnectedToRemoteDebuggee) &&
      !IsItALocalCommand(FirstCommand)) {

    if (g_IsConnectedToRemoteDebuggee) {

      //
      // It's a connection over network (VMI-Mode)
      //
      RemoteConnectionSendCommand(Command, strlen(Command) + 1);

    } else if (g_IsSerialConnectedToRemoteDebuggee) {

      //
      // It's a connection over serial (Debugger-Mode)
      //
      KdSendUserInputPacketToDebuggee(Command, strlen(Command) + 1);
    }

    //
    // Indicate that we sent the command to the target system
    //
    return 2;
  }

  //
  // Detect whether it's a .help command or not
  //
  if (!FirstCommand.compare(".help") || !FirstCommand.compare("help") ||
      !FirstCommand.compare(".hh")) {
    if (SplittedCommand.size() == 2) {
      //
      // Show that it's a help command
      //
      HelpCommand = TRUE;
      FirstCommand = SplittedCommand.at(1);
    } else {
      ShowMessages("Incorrect use of '%s'\n", FirstCommand.c_str());
      CommandHelpHelp();
      return 0;
    }
  }

  //
  // Start parsing commands
  //
  Iterator = g_CommandList.find(FirstCommand);

  if (Iterator == g_CommandList.end()) {

    //
    //  Command doesn't exist
    //
    ShowMessages("Couldn't resolve error at '%s'\n", FirstCommand.c_str());
  } else {
    if (HelpCommand) {
      Iterator->second.CommandHelpFunction();
    } else {
      Iterator->second.CommandFunction(SplittedCommand, CommandString);
    }
  }

  //
  // Save the command into log open file
  //
  if (g_LogOpened && !g_ExecutingScript) {
    LogopenSaveToFile("\n");
  }

  return 0;
}

VOID HyperdbgShowSignature() {

  if (g_IsConnectedToRemoteDebuggee) {
    if (g_IsRemoteDebuggerMessageReceived) {
      ShowMessages("HyperDbg [%s:%s] >", g_ServerIp.c_str(),
                   g_ServerPort.c_str());
    }

  } else if (g_DebuggingState.IsAttachedToUsermodeProcess) {
    ShowMessages("HyperDbg (%x:%x) >", g_DebuggingState.ConnectedProcessId,
                 g_DebuggingState.ConnectedThreadId);
  } else {
    ShowMessages("HyperDbg> ");
  }
}

/**
 * @brief This function is used to mark some commands as local
 * so we won't send them in local connections and instead handle
 * them in local instance of HyperDbg
 *
 * @param Command just the first word of command (without other parameters)
 * @return BOOLEAN if true then it shows that the command is a local
 * command and if false then it shows that the command can be used in
 * a remote machine
 */
BOOLEAN IsItALocalCommand(string Command) {

  //
  // Some commands should not be passed to the remote system
  // and instead should be handled in the current debugger
  //
  if (!Command.compare(".status") || !Command.compare("cls") ||
      !Command.compare(".cls") || !Command.compare("clear") ||
      !Command.compare("sleep") || !Command.compare("connect") ||
      !Command.compare(".connect") || !Command.compare("disconnect") ||
      !Command.compare(".disconnect") || !Command.compare(".listen") ||
      !Command.compare("listen") || !Command.compare(".logopen") ||
      !Command.compare(".logclose") || !Command.compare(".script") ||
      !Command.compare("g") || !Command.compare("go")) {

    return TRUE;
  }

  return FALSE;
}

VOID InitializeCommandsDictionary() {

  g_CommandList["clear"] = {&CommandClearScreen, &CommandClearScreenHelp, NULL};
  g_CommandList[".cls"] = {&CommandClearScreen, &CommandClearScreenHelp, NULL};
  g_CommandList["cls"] = {&CommandClearScreen, &CommandClearScreenHelp, NULL};

  g_CommandList[".connect"] = {&CommandConnect, &CommandConnectHelp, NULL};
  g_CommandList["connect"] = {&CommandConnect, &CommandConnectHelp, NULL};

  g_CommandList[".listen"] = {&CommandListen, &CommandListenHelp, NULL};
  g_CommandList["listen"] = {&CommandListen, &CommandListenHelp, NULL};

  g_CommandList["g"] = {&CommandG, &CommandGHelp, NULL};
  g_CommandList["go"] = {&CommandG, &CommandGHelp, NULL};

  g_CommandList[".attach"] = {&CommandAttach, &CommandAttachHelp, NULL};
  g_CommandList["attach"] = {&CommandAttach, &CommandAttachHelp, NULL};

  g_CommandList[".detach"] = {&CommandDetach, &CommandDetachHelp, NULL};
  g_CommandList["detach"] = {&CommandDetach, &CommandDetachHelp, NULL};

  g_CommandList[".process"] = {&CommandProcess, &CommandProcessHelp, NULL};

  g_CommandList["sleep"] = {&CommandSleep, &CommandSleepHelp, NULL};

  g_CommandList["event"] = {&CommandEvents, &CommandEventsHelp, NULL};
  g_CommandList["events"] = {&CommandEvents, &CommandEventsHelp, NULL};

  g_CommandList["setting"] = {&CommandSettings, &CommandSettingsHelp, NULL};
  g_CommandList["settings"] = {&CommandSettings, &CommandSettingsHelp, NULL};

  g_CommandList[".disconnect"] = {&CommandDisconnect, &CommandDisconnectHelp,
                                  NULL};
  g_CommandList["disconnect"] = {&CommandDisconnect, &CommandDisconnectHelp,
                                 NULL};

  g_CommandList[".debug"] = {&CommandDebug, &CommandDebugHelp, NULL};
  g_CommandList["debug"] = {&CommandDebug, &CommandDebugHelp, NULL};

  g_CommandList[".status"] = {&CommandStatus, &CommandStatusHelp, NULL};
  g_CommandList["status"] = {&CommandStatus, &CommandStatusHelp, NULL};

  g_CommandList["load"] = {&CommandLoad, &CommandLoadHelp, NULL};

  g_CommandList["exit"] = {&CommandExit, &CommandExitHelp, NULL};
  g_CommandList[".exit"] = {&CommandExit, &CommandExitHelp, NULL};

  g_CommandList["flush"] = {&CommandFlush, &CommandFlushHelp, NULL};

  g_CommandList["pause"] = {&CommandPause, &CommandPauseHelp, NULL};

  g_CommandList["unload"] = {&CommandUnload, &CommandUnloadHelp, NULL};

  g_CommandList[".script"] = {&CommandScript, &CommandScriptHelp, NULL};

  g_CommandList["output"] = {&CommandOutput, &CommandOutputHelp, NULL};

  g_CommandList["print"] = {&CommandPrint, &CommandPrintHelp, NULL};

  g_CommandList["?"] = {&CommandEval, &CommandEvalHelp, NULL};
  g_CommandList["eval"] = {&CommandEval, &CommandEvalHelp, NULL};
  g_CommandList["evaluate"] = {&CommandEval, &CommandEvalHelp, NULL};

  g_CommandList[".logopen"] = {&CommandLogopen, &CommandLogopenHelp, NULL};

  g_CommandList[".logclose"] = {&CommandLogclose, &CommandLogcloseHelp, NULL};

  g_CommandList["test"] = {&CommandTest, &CommandTestHelp, NULL};

  g_CommandList["cpu"] = {&CommandCpu, &CommandCpuHelp, NULL};

  g_CommandList["wrmsr"] = {&CommandWrmsr, &CommandWrmsrHelp, NULL};

  g_CommandList["rdmsr"] = {&CommandRdmsr, &CommandRdmsrHelp, NULL};

  g_CommandList["!va2pa"] = {&CommandVa2pa, &CommandVa2paHelp, NULL};

  g_CommandList["!pa2va"] = {&CommandPa2va, &CommandPa2vaHelp, NULL};

  g_CommandList[".formats"] = {&CommandFormats, &CommandFormatsHelp, NULL};
  g_CommandList[".format"] = {&CommandFormats, &CommandFormatsHelp, NULL};

  g_CommandList["!pte"] = {&CommandPte, &CommandPteHelp, NULL};

  g_CommandList["!monitor"] = {&CommandMonitor, &CommandMonitorHelp, NULL};

  g_CommandList["~"] = {&CommandCore, &CommandCoreHelp, NULL};
  g_CommandList["core"] = {&CommandCore, &CommandCoreHelp, NULL};

  g_CommandList["!vmcall"] = {&CommandVmcall, &CommandVmcallHelp, NULL};

  g_CommandList["!epthook"] = {&CommandEptHook, &CommandEptHookHelp, NULL};
  g_CommandList["bh"] = {&CommandEptHook, &CommandEptHookHelp, NULL};
  g_CommandList["bp"] = {&CommandEptHook, &CommandEptHookHelp, NULL};

  g_CommandList["!epthook2"] = {&CommandEptHook2, &CommandEptHook2Help, NULL};

  g_CommandList["!cpuid"] = {&CommandCpuid, &CommandCpuidHelp, NULL};

  g_CommandList["!msrread"] = {&CommandMsrread, &CommandMsrreadHelp, NULL};

  g_CommandList["!msrwrite"] = {&CommandMsrwrite, &CommandMsrwriteHelp, NULL};

  g_CommandList["!tsc"] = {&CommandTsc, &CommandTscHelp, NULL};

  g_CommandList["!pmc"] = {&CommandPmc, &CommandPmcHelp, NULL};

  g_CommandList["!dr"] = {&CommandDr, &CommandDrHelp, NULL};

  g_CommandList["!ioin"] = {&CommandIoin, &CommandIoinHelp, NULL};

  g_CommandList["!ioout"] = {&CommandIoout, &CommandIooutHelp, NULL};

  g_CommandList["!exception"] = {&CommandException, &CommandExceptionHelp,
                                 NULL};

  g_CommandList["!interrupt"] = {&CommandInterrupt, &CommandInterruptHelp,
                                 NULL};

  g_CommandList["!syscall"] = {&CommandSyscallAndSysret, &CommandSyscallHelp,
                               NULL};

  g_CommandList["!sysret"] = {&CommandSyscallAndSysret, &CommandSysretHelp,
                              NULL};

  g_CommandList["!hide"] = {&CommandHide, &CommandHideHelp, NULL};

  g_CommandList["!unhide"] = {&CommandUnhide, &CommandUnhideHelp, NULL};

  g_CommandList["!measure"] = {&CommandMeasure, &CommandMeasureHelp, NULL};

  g_CommandList["lm"] = {&CommandLm, &CommandLmHelp, NULL};

  g_CommandList["p"] = {&CommandP, &CommandPHelp, NULL};
  g_CommandList["pr"] = {&CommandP, &CommandPHelp, NULL};

  g_CommandList["t"] = {&CommandT, &CommandTHelp, NULL};
  g_CommandList["tr"] = {&CommandT, &CommandTHelp, NULL};

  g_CommandList["db"] = {&CommandReadMemoryAndDisassembler,
                         &CommandReadMemoryAndDisassemblerHelp, NULL};
  g_CommandList["dc"] = {&CommandReadMemoryAndDisassembler,
                         &CommandReadMemoryAndDisassemblerHelp, NULL};
  g_CommandList["dd"] = {&CommandReadMemoryAndDisassembler,
                         &CommandReadMemoryAndDisassemblerHelp, NULL};
  g_CommandList["dq"] = {&CommandReadMemoryAndDisassembler,
                         &CommandReadMemoryAndDisassemblerHelp, NULL};
  g_CommandList["!db"] = {&CommandReadMemoryAndDisassembler,
                          &CommandReadMemoryAndDisassemblerHelp, NULL};
  g_CommandList["!dc"] = {&CommandReadMemoryAndDisassembler,
                          &CommandReadMemoryAndDisassemblerHelp, NULL};
  g_CommandList["!dd"] = {&CommandReadMemoryAndDisassembler,
                          &CommandReadMemoryAndDisassemblerHelp, NULL};
  g_CommandList["!dq"] = {&CommandReadMemoryAndDisassembler,
                          &CommandReadMemoryAndDisassemblerHelp, NULL};
  g_CommandList["!u"] = {&CommandReadMemoryAndDisassembler,
                         &CommandReadMemoryAndDisassemblerHelp, NULL};
  g_CommandList["u"] = {&CommandReadMemoryAndDisassembler,
                        &CommandReadMemoryAndDisassemblerHelp, NULL};
  g_CommandList["!u2"] = {&CommandReadMemoryAndDisassembler,
                          &CommandReadMemoryAndDisassemblerHelp, NULL};
  g_CommandList["u2"] = {&CommandReadMemoryAndDisassembler,
                         &CommandReadMemoryAndDisassemblerHelp, NULL};

  g_CommandList["eb"] = {&CommandEditMemory, &CommandEditMemoryHelp, NULL};
  g_CommandList["ed"] = {&CommandEditMemory, &CommandEditMemoryHelp, NULL};
  g_CommandList["eq"] = {&CommandEditMemory, &CommandEditMemoryHelp, NULL};
  g_CommandList["!eb"] = {&CommandEditMemory, &CommandEditMemoryHelp, NULL};
  g_CommandList["!ed"] = {&CommandEditMemory, &CommandEditMemoryHelp, NULL};
  g_CommandList["!eq"] = {&CommandEditMemory, &CommandEditMemoryHelp, NULL};

  g_CommandList["sb"] = {&CommandSearchMemory, &CommandSearchMemoryHelp, NULL};
  g_CommandList["sd"] = {&CommandSearchMemory, &CommandSearchMemoryHelp, NULL};
  g_CommandList["sq"] = {&CommandSearchMemory, &CommandSearchMemoryHelp, NULL};
  g_CommandList["!sb"] = {&CommandSearchMemory, &CommandSearchMemoryHelp, NULL};
  g_CommandList["!sd"] = {&CommandSearchMemory, &CommandSearchMemoryHelp, NULL};
  g_CommandList["!sq"] = {&CommandSearchMemory, &CommandSearchMemoryHelp, NULL};
}
