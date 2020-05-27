/**
 * @file syscall-sysret.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief !syscall and !sysret commands
 * @details
 * @version 0.1
 * @date 2020-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

VOID CommandSyscallHelp() {
  ShowMessages("!syscall : Monitors and hooks all execution of syscall "
               "instructions.\n\n");
  ShowMessages("syntax : \t!syscall core [core index "
               "(hex value)] pid [process id (hex value)] condition {[assembly "
               "in hex]} code {[assembly in hex]} buffer [pre-require buffer - "
               "(hex value)] \n");

  ShowMessages("\t\te.g : !syscall\n");
  ShowMessages("\t\te.g : !syscall pid 400\n");
  ShowMessages("\t\te.g : !syscall core 2 "
               "pid 400\n");
}

VOID CommandSysretHelp() {
  ShowMessages("!sysret : Monitors and hooks all execution of sysret "
               "instructions.\n\n");
  ShowMessages("syntax : \t!sysret core [core index "
               "(hex value)] pid [process id (hex value)] condition {[assembly "
               "in hex]} code {[assembly in hex]} buffer [pre-require buffer - "
               "(hex value)] \n");

  ShowMessages("\t\te.g : !sysret\n");
  ShowMessages("\t\te.g : !sysret pid 400\n");
  ShowMessages("\t\te.g : !sysret core 2 "
               "pid 400\n");
}

void CommandSyscallAndSysret(vector<string> SplittedCommand) {

  PDEBUGGER_GENERAL_EVENT_DETAIL Event;
  PDEBUGGER_GENERAL_ACTION Action;
  UINT32 EventLength;
  UINT32 ActionLength;
  UINT64 TargetAddress;

  //
  // Interpret and fill the general event and action fields
  //
  //
  if (!SplittedCommand.at(0).compare("!syscall")) {
    if (!InterpretGeneralEventAndActionsFields(
            &SplittedCommand, SYSCALL_HOOK_EFER_SYSCALL, &Event, &EventLength,
            &Action, &ActionLength)) {
      CommandSyscallHelp();
      return;
    }
  } else {
    if (!InterpretGeneralEventAndActionsFields(
            &SplittedCommand, SYSCALL_HOOK_EFER_SYSRET, &Event, &EventLength,
            &Action, &ActionLength)) {
      CommandSysretHelp();
      return;
    }
  }

  //
  // Send the ioctl to the kernel for event registeration
  //
  SendEventToKernel(Event, EventLength);

  //
  // Add the event to the kernel
  //
  RegisterActionToEvent(Action, ActionLength);
}
