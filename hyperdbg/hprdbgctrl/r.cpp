/**
 * @file r.cpp
 * @author Alee Amini (aleeaminiz@gmail.com)
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief r command
 * @details
 * @version 0.1
 * @date 2021-02-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

using namespace std;

//
// Global Variables
//
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;

map<string, REGS_ENUM> RegistersMap = {
    {"rax", REGISTER_RAX}, {"rbx", REGISTER_RBX},       {"rcx", REGISTER_RCX},
    {"rdx", REGISTER_RDX}, {"rsi", REGISTER_RSI},       {"rdi", REGISTER_RDI},
    {"rbp", REGISTER_RBP}, {"rsp", REGISTER_RSP},       {"r8", REGISTER_R8},
    {"r9", REGISTER_R9},   {"r10", REGISTER_R10},       {"r11", REGISTER_R11},
    {"r12", REGISTER_R12}, {"r13", REGISTER_R13},       {"r14", REGISTER_R14},
    {"r15", REGISTER_R15}, {"ds", REGISTER_DS},         {"es", REGISTER_ES},
    {"fs", REGISTER_FS},   {"gs", REGISTER_GS},         {"cs", REGISTER_CS},
    {"ss", REGISTER_SS},   {"eflags", REGISTER_EFLAGS}, {"rip", REGISTER_RIP}};

/**
 * @brief help of r command
 *
 * @return VOID
 */
VOID CommandRHelp() {
  ShowMessages("r : read or modify registers.\n\n");
  ShowMessages("syntax : \tr [register] [= expr]\n");
  ShowMessages("\t\te.g : r @rax\n");
  ShowMessages("\t\te.g : r rax\n");
  ShowMessages("\t\te.g : r rax = 0x55\n");
}

/**
 * @brief handler of r command
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID CommandR(vector<string> SplittedCommand, string Command) {

  //
  // Interpret here
  //
  REGS_ENUM Reg;
  vector<string> Tmp;
  PDEBUGGEE_REGISTER_READ_DESCRIPTION RegD =
      new DEBUGGEE_REGISTER_READ_DESCRIPTION;

  if (SplittedCommand[0] != "r") {

    return;
  }

  //
  // clear additional space of the command string
  //
  Command.erase(0, 1);
  ReplaceAll(Command, " ", "");

  //
  // if command does not contain a '=' means user wants to read it
  //
  if (Command.find('=', 0) == string::npos) {
    Reg = RegistersMap[Command];
    // Reg = RegsMap[Command.erase(0, 1)];
    if (Reg != 0) {
      RegD->RegisterID = Reg;
      // RegD->Modified = FALSE;
      // RegD->Value = "";

      ShowMessages("Command is : r %s\n", Command.c_str());

      //
      // send the request
      //
      if (g_IsSerialConnectedToRemoteDebuggee) {

        KdSendReadRegisterPacketToDebuggee(RegD);
      } else {
        ShowMessages("err, reading registers (r) is not valid in the current "
                     "context, you "
                     "should connect to a debuggee.\n");
      }

    } else {
      ShowMessages("err, register %s is invalid\n", Command.c_str());
    }
  }

  //
  // if command contains a '=' means user wants modify the register
  //
  else if (Command.find("=", 0)) {
    Tmp = Split(Command, '=');
    if (Tmp.size() == 2) {
      Reg = RegistersMap[Tmp[0]];
      // Reg = RegsMap[Command.erase(0, 1)];
      if (Reg != 0) {
        RegD->RegisterID = Reg;
        // RegD->Modified = TRUE;
        // RegD->Value = Tmp[1];

        ShowMessages("Command is : r %s=%s\n", Tmp[0],
                     Tmp[1]); //, RegD->Value);

        //
        // send the request
        //
      }
    }
  }

  delete (RegD);
}
