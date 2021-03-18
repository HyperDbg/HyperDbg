/**
 * @file bl.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief bl command
 * @details
 * @version 0.1
 * @date 2021-03-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;

/**
 * @brief help of bl command
 *
 * @return VOID
 */
VOID CommandBlHelp() {
  ShowMessages("bl : lists all the enabled and disabled breakpoints.\n\n");
  ShowMessages("syntax : \tbl\n");
}

/**
 * @brief handler of bl command
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID CommandBl(vector<string> SplittedCommand, string Command) {

  //////////////////////////////////////////////////////////////////
  /// test should be removed
  UINT64 NextRip = 0;
  RFLAGS Rflags = {0};
  unsigned char data[] = {
      0x48, 0x8B, 0x05, 0x39, 0x00,
      0x13, 0x00, // mov rax, qword ptr ds:[<SomeModule.SomeData>]
      0x50,       // push rax
      0xFF, 0x15, 0xF2, 0x10, 0x00,
      0x00,       // call qword ptr ds:[<SomeModule.SomeFunction>]
      0x85, 0xC0, // test eax, eax
      0x0F, 0x84, 0x00, 0x00, 0x00,
      0x00,                        // jz 0x007FFFFFFF400016
      0xE9, 0xE5, 0x0F, 0x00, 0x00 // jmp <SomeModule.EntryPoint>
  };

  HyperDbgFindNextInstruction(DEBUGGER_REMOTE_STEPPING_REQUEST_STEP_IN, data,
                              sizeof(data), Rflags, TRUE, 0x10000000, &NextRip);

  return;

  //////////////////////////////////////////////////////////////////

  DEBUGGEE_BP_LIST_OR_MODIFY_PACKET Request = {0};

  //
  // Validate the commands
  //
  if (SplittedCommand.size() != 1) {
    ShowMessages("incorrect use of 'bl'\n\n");
    CommandBlHelp();
    return;
  }

  //
  // Check if the remote serial debuggee is paused or not (connected or not)
  //
  if (g_IsSerialConnectedToRemoteDebuggee) {

    //
    // Perform listing breakpoint
    //
    Request.Request = DEBUGGEE_BREAKPOINT_MODIFICATION_REQUEST_LIST_BREAKPOINTS;

    //
    // Send the request
    //
    KdSendListOrModifyPacketToDebuggee(&Request);
    ShowMessages("\n");

  } else {
    ShowMessages("err, listing breakpoints is only valid if you connected to "
                 "a debuggee in debugger-mode\n");
  }
}
