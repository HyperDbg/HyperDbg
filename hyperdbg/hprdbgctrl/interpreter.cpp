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
extern BOOLEAN g_LogOpened;
extern BOOLEAN g_ExecutingScript;

/**
 * @brief Interpret commands
 *
 * @param Command The text of command
 * @return int returns return zero if it was successful or non-zero if there was
 * error
 */
BOOL A = FALSE;
int _cdecl HyperdbgInterpreter(const char *Command) {

  if (!A) {
    //
    // Register the CTRL+C and CTRL+BREAK Signals handler
    //
    if (!SetConsoleCtrlHandler(BreakController, TRUE)) {
      ShowMessages(
          "Error in registering CTRL+C and CTRL+BREAK Signals handler\n");
      return 1;
    }
    A = TRUE;
  }

  string CommandString(Command);

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

  if (!FirstCommand.compare("clear") || !FirstCommand.compare("cls") ||
      !FirstCommand.compare(".cls")) {
    CommandClearScreen();
  } else if (!FirstCommand.compare(".connect")) {
    CommandConnect(SplittedCommand);
  } else if (!FirstCommand.compare("event") ||
             !FirstCommand.compare("events")) {
    CommandEvents(SplittedCommand);
  } else if (!FirstCommand.compare("connect")) {
    ShowMessages("Couldn't resolve error at '%s', did you mean '.connect'?",
                 FirstCommand.c_str());
  } else if (!FirstCommand.compare("disconnect")) {
    ShowMessages("Couldn't resolve error at '%s', did you mean '.disconnect'?",
                 FirstCommand.c_str());
  } else if (!FirstCommand.compare(".disconnect")) {
    CommandDisconnect(SplittedCommand);
  } else if (!FirstCommand.compare("load")) {
    CommandLoad(SplittedCommand);
  } else if (!FirstCommand.compare("exit") || !FirstCommand.compare(".exit")) {
    CommandExit(SplittedCommand);
  } else if (!FirstCommand.compare("unload")) {
    CommandUnload(SplittedCommand);
  } else if (!FirstCommand.compare(".script")) {
    CommandScript(SplittedCommand, CommandString);
  } else if (!FirstCommand.compare(".logopen")) {
    CommandLogopen(SplittedCommand, CommandString);
  } else if (!FirstCommand.compare(".logclose")) {
    CommandLogclose(SplittedCommand);
  } else if (!FirstCommand.compare("test")) {
    CommandTest(SplittedCommand);
  } else if (!FirstCommand.compare("cpu")) {
    CommandCpu(SplittedCommand);
  } else if (!FirstCommand.compare("wrmsr")) {
    CommandWrmsr(SplittedCommand);
  } else if (!FirstCommand.compare("rdmsr")) {
    CommandRdmsr(SplittedCommand);
  } else if (!FirstCommand.compare("!va2pa")) {
    CommandVa2pa(SplittedCommand);
  } else if (!FirstCommand.compare("!pa2va")) {
    CommandPa2va(SplittedCommand);
  } else if (!FirstCommand.compare(".formats")) {
    CommandFormats(SplittedCommand);
  } else if (!FirstCommand.compare("!pte")) {
    CommandPte(SplittedCommand);
  } else if (!FirstCommand.compare("!monitor")) {
    CommandMonitor(SplittedCommand);
  } else if (!FirstCommand.compare("!vmcall")) {
    CommandVmcall(SplittedCommand);
  } else if (!FirstCommand.compare("!epthook")) {
    CommandEptHook(SplittedCommand);
  } else if (!FirstCommand.compare("!epthook2")) {
    CommandEptHook2(SplittedCommand);
  } else if (!FirstCommand.compare("!cpuid")) {
    CommandCpuid(SplittedCommand);
  } else if (!FirstCommand.compare("!msrread")) {
    CommandMsrread(SplittedCommand);
  } else if (!FirstCommand.compare("!msrwrite")) {
    CommandMsrwrite(SplittedCommand);
  } else if (!FirstCommand.compare("!tsc")) {
    CommandTsc(SplittedCommand);
  } else if (!FirstCommand.compare("!pmc")) {
    CommandPmc(SplittedCommand);
  } else if (!FirstCommand.compare("!dr")) {
    CommandDr(SplittedCommand);
  } else if (!FirstCommand.compare("!ioin")) {
    CommandIoin(SplittedCommand);
  } else if (!FirstCommand.compare("!ioout")) {
    CommandIoout(SplittedCommand);
  } else if (!FirstCommand.compare("!exception")) {
    CommandException(SplittedCommand);
  } else if (!FirstCommand.compare("!interrupt")) {
    CommandInterrupt(SplittedCommand);
  } else if (!FirstCommand.compare("!syscall") ||
             !FirstCommand.compare("!sysret")) {
    CommandSyscallAndSysret(SplittedCommand);
  } else if (!FirstCommand.compare("!hide")) {
    CommandHide(SplittedCommand);
  } else if (!FirstCommand.compare("!unhide")) {
    CommandUnhide(SplittedCommand);
  } else if (!FirstCommand.compare("lm")) {
    CommandLm(SplittedCommand);
  } else if (!FirstCommand.compare("db") || !FirstCommand.compare("dc") ||
             !FirstCommand.compare("dd") || !FirstCommand.compare("dq") ||
             !FirstCommand.compare("!db") || !FirstCommand.compare("!dc") ||
             !FirstCommand.compare("!dd") || !FirstCommand.compare("!dq") ||
             !FirstCommand.compare("!u") || !FirstCommand.compare("u")) {
    CommandReadMemoryAndDisassembler(SplittedCommand);
  } else if (!FirstCommand.compare("!epthook2") ||
             !FirstCommand.compare("bh")) {
    CommandEptHook2(SplittedCommand);
  } else {
    ShowMessages("Couldn't resolve error at '%s'", FirstCommand.c_str());
    ShowMessages("\n");
  }

  //
  // Save the command into log open file
  //
  if (g_LogOpened && !g_ExecutingScript) {
    LogopenSaveToFile("\n");
  }

  return 0;
}
