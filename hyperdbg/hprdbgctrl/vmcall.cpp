/**
 * @file vmcall.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief !vmcall command
 * @details
 * @version 0.1
 * @date 2020-07-03
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */

#include "pch.h"

VOID CommandVmcallHelp() {
  ShowMessages("!vmcall : Monitors execution of VMCALL instruction.\n\n");
  ShowMessages("syntax : \t!vmcall core [core index "
               "(hex value)] pid [process id (hex value)] condition {[assembly "
               "in hex]} code {[assembly in hex]} buffer [pre-require buffer - "
               "(hex value)] \n");

  ShowMessages("\t\te.g : !vmcall\n");
  ShowMessages("\t\te.g : !vmcall pid 400\n");
  ShowMessages("\t\te.g : !vmcall core 2 pid 400\n");
}

VOID CommandVmcall(vector<string> SplittedCommand) {

  PDEBUGGER_GENERAL_EVENT_DETAIL Event;
  PDEBUGGER_GENERAL_ACTION Action;
  UINT32 EventLength;
  UINT32 ActionLength;

  //
  // Interpret and fill the general event and action fields
  //
  //
  if (!InterpretGeneralEventAndActionsFields(
          &SplittedCommand, VMCALL_INSTRUCTION_EXECUTION, &Event, &EventLength,
          &Action, &ActionLength)) {
      CommandVmcallHelp();
    return;
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
