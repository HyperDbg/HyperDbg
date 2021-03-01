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

//
// Structure for registers
//
struct User_regs_struct {
  unsigned long r15;
  unsigned long r14;
  unsigned long r13;
  unsigned long r12;
  unsigned long rbp;
  unsigned long rbx;
  unsigned long r11;
  unsigned long r10;
  unsigned long r9;
  unsigned long r8;
  unsigned long rax;
  unsigned long rcx;
  unsigned long rdx;
  unsigned long rsi;
  unsigned long rdi;
  unsigned long rip;
  unsigned long cs;
  unsigned long eflags;
  unsigned long rsp;
  unsigned long ss;
  unsigned long fs_base;
  unsigned long gs_base;
  unsigned long ds;
  unsigned long es;
  unsigned long fs;
  unsigned long gs;
};

enum RegsEnum {
  rax = 1,
  rbx = 2,
  rcx = 3,
  rdx = 4,
  rsi = 5,
  rdi = 6,
  rbp = 7,
  rsp = 8,
  r8 = 9,
  r9 = 10,
  r10 = 11,
  r11 = 12,
  r12 = 13,
  r13 = 14,
  r14 = 15,
  r15 = 16,
  ds = 17,
  es = 18,
  fs = 19,
  gs = 20,
  cs = 21,
  ss = 22,
  eflags = 23,
  rip = 24
};
map<string, RegsEnum> RegsMap = {
    {"rax", rax}, {"rbx", rbx}, {"rcx", rcx},       {"rdx", rdx}, {"rsi", rsi},
    {"rdi", rdi}, {"rbp", rbp}, {"rsp", rsp},       {"r8", r8},   {"r9", r9},
    {"r10", r10}, {"r11", r11}, {"r12", r12},       {"r13", r13}, {"r14", r14},
    {"r15", r15}, {"ds", ds},   {"es", es},         {"fs", fs},   {"gs", gs},
    {"cs", cs},   {"ss", ss},   {"eflags", eflags}, {"rip", rip}};

//
// Register Descriptor Structure to use in r command.
//
typedef struct RegDescStruct {
  RegsEnum RegisterID;
  BOOLEAN Modified;
  string Value;
} RegDesc;

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
  RegsEnum Reg;
  vector<string> Tmp;
  RegDesc *RegD = new RegDesc;

  if (SplittedCommand[0] != "r")
    return;

  //
  // clear additional space of the command string
  //
  Command.erase(0, 1);
  ReplaceAll(Command, " ", "");
  if (Command.find('=', 0) == string::npos) {
    Reg = RegsMap[Command];
    Reg = RegsMap[Command.erase(0, 1)];
    if (Reg != 0) {
      RegD->RegisterID = Reg;
      RegD->Modified = FALSE;
      RegD->Value = "";
      ShowMessages("Command is : r %s\n", Command.c_str());
    } else {
      ShowMessages("regvalue %s is invalid\n", Command.c_str());
    }
  }
  //
  // if command contains a '=' means user wants modify the register
  //

  else if (Command.find("=", 0)) {
    Tmp = Split(Command, '=');
    if (Tmp.size() == 2) {
      Reg = RegsMap[Tmp[0]];
      Reg = RegsMap[Tmp[0].erase(0, 1)];
      if (Reg != 0) {
          RegD->RegisterID = Reg;
          RegD->Modified = TRUE;
          RegD->Value = Tmp[1];
          ShowMessages("Command is : r %d=%s\n", RegD->RegisterID, RegD->Value);
      }
    }
  }

  delete (RegD);
}
