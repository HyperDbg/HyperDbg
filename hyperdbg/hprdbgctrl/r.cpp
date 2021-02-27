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

  ShowMessages("Command is : %s\n", Command.c_str());
}
