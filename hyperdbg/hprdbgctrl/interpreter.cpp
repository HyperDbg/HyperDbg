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
extern BOOLEAN g_LogOpened;
extern BOOLEAN g_ExecutingScript;
extern BOOLEAN g_IsConnectedToHyperDbgLocally;
extern BOOLEAN g_IsConnectedToRemoteDebuggee;
extern BOOLEAN g_IsRemoteDebuggerMessageReceived;
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

  //
  // Save the command into log open file
  //
  if (g_LogOpened && !g_ExecutingScript) {
    LogopenSaveToFile("HyperDbg >");
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
  if (g_IsConnectedToRemoteDebuggee && !IsItALocalCommand(FirstCommand)) {
    RemoteConnectionSendCommand(Command, strlen(Command) + 1);

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

  if (!FirstCommand.compare("clear") || !FirstCommand.compare("cls") ||
      !FirstCommand.compare(".cls")) {

    if (HelpCommand)
      CommandClearScreenHelp();
    else
      CommandClearScreen();

  } else if (!FirstCommand.compare(".connect") ||
             !FirstCommand.compare("connect")) {

    if (HelpCommand)
      CommandConnectHelp();
    else
      CommandConnect(SplittedCommand);

  } else if (!FirstCommand.compare(".listen") ||
             !FirstCommand.compare("listen")) {

    if (HelpCommand)
      CommandListenHelp();
    else
      CommandListen(SplittedCommand);

  } else if (!FirstCommand.compare("g") || !FirstCommand.compare("go")) {

    if (HelpCommand)
      CommandGHelp();
    else
      CommandG(SplittedCommand);

  } else if (!FirstCommand.compare(".attach") ||
             !FirstCommand.compare("attach")) {

    if (HelpCommand)
      CommandAttachHelp();
    else
      CommandAttach(SplittedCommand);

  } else if (!FirstCommand.compare(".detach") ||
             !FirstCommand.compare("detach")) {

    if (HelpCommand)
      CommandDetachHelp();
    else
      CommandDetach(SplittedCommand);

  } else if (!FirstCommand.compare("t")) {

    if (HelpCommand)
      CommandTHelp();
    else
      CommandT(SplittedCommand);

  } else if (!FirstCommand.compare("sleep")) {

    if (HelpCommand)
      CommandSleepHelp();
    else
      CommandSleep(SplittedCommand);

  } else if (!FirstCommand.compare("event") ||
             !FirstCommand.compare("events")) {

    if (HelpCommand)
      CommandEventsHelp();
    else
      CommandEvents(SplittedCommand);

  } else if (!FirstCommand.compare("setting") ||
             !FirstCommand.compare("settings")) {

    if (HelpCommand)
      CommandSettingsHelp();
    else
      CommandSettings(SplittedCommand);

  } else if (!FirstCommand.compare(".disconnect") ||
             !FirstCommand.compare("disconnect")) {

    if (HelpCommand)
      CommandDisconnectHelp();
    else
      CommandDisconnect(SplittedCommand);

  } else if (!FirstCommand.compare(".status") ||
             !FirstCommand.compare("status")) {

    if (HelpCommand)
      CommandStatusHelp();
    else
      CommandStatus(SplittedCommand);

  } else if (!FirstCommand.compare("load")) {

    if (HelpCommand)
      CommandLoadHelp();
    else
      CommandLoad(SplittedCommand);

  } else if (!FirstCommand.compare("exit") || !FirstCommand.compare(".exit")) {

    if (HelpCommand)
      CommandExitHelp();
    else
      CommandExit(SplittedCommand);

  } else if (!FirstCommand.compare("flush")) {

    if (HelpCommand)
      CommandFlushHelp();
    else
      CommandFlush(SplittedCommand);

  } else if (!FirstCommand.compare("pause")) {

    if (HelpCommand)
      CommandPauseHelp();
    else
      CommandPause(SplittedCommand);

  } else if (!FirstCommand.compare("unload")) {

    if (HelpCommand)
      CommandUnloadHelp();
    else
      CommandUnload(SplittedCommand);

  } else if (!FirstCommand.compare(".script")) {

    if (HelpCommand)
      CommandScriptHelp();
    else
      CommandScript(SplittedCommand, CommandString);

  } else if (!FirstCommand.compare("output")) {

    if (HelpCommand)
      CommandOutputHelp();
    else
      CommandOutput(SplittedCommand, CommandString);

  } else if (!FirstCommand.compare("print")) {

    if (HelpCommand)
      CommandPrintHelp();
    else
      CommandPrint(SplittedCommand, CommandString);

  } else if (!FirstCommand.compare(".logopen")) {

    if (HelpCommand)
      CommandLogopenHelp();
    else
      CommandLogopen(SplittedCommand, CommandString);

  } else if (!FirstCommand.compare(".logclose")) {

    if (HelpCommand)
      CommandLogcloseHelp();
    else
      CommandLogclose(SplittedCommand);

  } else if (!FirstCommand.compare("test")) {

    if (HelpCommand)
      CommandTestHelp();
    else
      CommandTest(SplittedCommand);

  } else if (!FirstCommand.compare("cpu")) {

    if (HelpCommand)
      CommandCpuHelp();
    else
      CommandCpu(SplittedCommand);

  } else if (!FirstCommand.compare("wrmsr")) {

    if (HelpCommand)
      CommandWrmsrHelp();
    else
      CommandWrmsr(SplittedCommand);

  } else if (!FirstCommand.compare("rdmsr")) {

    if (HelpCommand)
      CommandRdmsrHelp();
    else
      CommandRdmsr(SplittedCommand);

  } else if (!FirstCommand.compare("!va2pa")) {

    if (HelpCommand)
      CommandVa2paHelp();
    else
      CommandVa2pa(SplittedCommand);

  } else if (!FirstCommand.compare("!pa2va")) {

    if (HelpCommand)
      CommandPa2vaHelp();
    else
      CommandPa2va(SplittedCommand);

  } else if (!FirstCommand.compare(".formats")) {

    if (HelpCommand)
      CommandFormatsHelp();
    else
      CommandFormats(SplittedCommand);

  } else if (!FirstCommand.compare("!pte")) {

    if (HelpCommand)
      CommandPteHelp();
    else
      CommandPte(SplittedCommand);

  } else if (!FirstCommand.compare("!monitor")) {

    if (HelpCommand)
      CommandMonitorHelp();
    else
      CommandMonitor(SplittedCommand);

  } else if (!FirstCommand.compare("!vmcall")) {

    if (HelpCommand)
      CommandVmcallHelp();
    else
      CommandVmcall(SplittedCommand);

  } else if (!FirstCommand.compare("!epthook") || !FirstCommand.compare("bh") ||
             !FirstCommand.compare("bp")) {

    if (HelpCommand)
      CommandEptHookHelp();
    else
      CommandEptHook(SplittedCommand);

  } else if (!FirstCommand.compare("!epthook2")) {

    if (HelpCommand)
      CommandEptHook2Help();
    else
      CommandEptHook2(SplittedCommand);

  } else if (!FirstCommand.compare("!cpuid")) {

    if (HelpCommand)
      CommandCpuidHelp();
    else
      CommandCpuid(SplittedCommand);

  } else if (!FirstCommand.compare("!msrread")) {

    if (HelpCommand)
      CommandMsrreadHelp();
    else
      CommandMsrread(SplittedCommand);

  } else if (!FirstCommand.compare("!msrwrite")) {

    if (HelpCommand)
      CommandMsrwriteHelp();
    else
      CommandMsrwrite(SplittedCommand);

  } else if (!FirstCommand.compare("!tsc")) {

    if (HelpCommand)
      CommandTscHelp();
    else
      CommandTsc(SplittedCommand);

  } else if (!FirstCommand.compare("!pmc")) {

    if (HelpCommand)
      CommandPmcHelp();
    else
      CommandPmc(SplittedCommand);

  } else if (!FirstCommand.compare("!dr")) {

    if (HelpCommand)
      CommandDrHelp();
    else
      CommandDr(SplittedCommand);

  } else if (!FirstCommand.compare("!ioin")) {

    if (HelpCommand)
      CommandIoinHelp();
    else
      CommandIoin(SplittedCommand);

  } else if (!FirstCommand.compare("!ioout")) {

    if (HelpCommand)
      CommandIooutHelp();
    else
      CommandIoout(SplittedCommand);

  } else if (!FirstCommand.compare("!exception")) {

    if (HelpCommand)
      CommandExceptionHelp();
    else
      CommandException(SplittedCommand);

  } else if (!FirstCommand.compare("!interrupt")) {

    if (HelpCommand)
      CommandInterruptHelp();
    else
      CommandInterrupt(SplittedCommand);

  } else if (!FirstCommand.compare("!syscall")) {

    if (HelpCommand)
      CommandSyscallHelp();
    else
      CommandSyscallAndSysret(SplittedCommand);

  } else if (!FirstCommand.compare("!sysret")) {

    if (HelpCommand)
      CommandSysretHelp();
    else
      CommandSyscallAndSysret(SplittedCommand);

  } else if (!FirstCommand.compare("!hide")) {

    if (HelpCommand)
      CommandHideHelp();
    else
      CommandHide(SplittedCommand, CommandString);

  } else if (!FirstCommand.compare("!unhide")) {

    if (HelpCommand)
      CommandUnhideHelp();
    else
      CommandUnhide(SplittedCommand);

  } else if (!FirstCommand.compare("!measure")) {

    if (HelpCommand)
      CommandMeasureHelp();
    else
      CommandMeasure(SplittedCommand);

  } else if (!FirstCommand.compare("lm")) {

    if (HelpCommand)
      CommandLmHelp();
    else
      CommandLm(SplittedCommand);

  } else if (!FirstCommand.compare("db") || !FirstCommand.compare("dc") ||
             !FirstCommand.compare("dd") || !FirstCommand.compare("dq") ||
             !FirstCommand.compare("!db") || !FirstCommand.compare("!dc") ||
             !FirstCommand.compare("!dd") || !FirstCommand.compare("!dq") ||
             !FirstCommand.compare("!u") || !FirstCommand.compare("u") ||
             !FirstCommand.compare("!u2") || !FirstCommand.compare("u2")) {

    if (HelpCommand)
      CommandReadMemoryAndDisassemblerHelp();
    else
      CommandReadMemoryAndDisassembler(SplittedCommand);

  } else if (!FirstCommand.compare("!epthook2")) {

    if (HelpCommand)
      CommandEptHook2Help();
    else
      CommandEptHook2(SplittedCommand);

  } else if (!FirstCommand.compare("eb") || !FirstCommand.compare("ed") ||
             !FirstCommand.compare("eq") || !FirstCommand.compare("!eb") ||
             !FirstCommand.compare("!ed") || !FirstCommand.compare("!eq")) {

    if (HelpCommand)
      CommandEditMemoryHelp();
    else
      CommandEditMemory(SplittedCommand);

  } else if (!FirstCommand.compare("sb") || !FirstCommand.compare("sd") ||
             !FirstCommand.compare("sq") || !FirstCommand.compare("!sb") ||
             !FirstCommand.compare("!sd") || !FirstCommand.compare("!sq")) {

    if (HelpCommand)
      CommandSearchMemoryHelp();
    else
      CommandSearchMemory(SplittedCommand);

  } else {
    ShowMessages("Couldn't resolve error at '%s'\n", FirstCommand.c_str());
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
    ShowMessages("HyperDbg >");
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
