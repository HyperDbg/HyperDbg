/**
 * @file dt.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief !dr commands
 * @details
 * @version 0.1
 * @date 2020-06-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */

#include "pch.h"

VOID CommandDrHelp() {
  ShowMessages("!dr : Monitors any access to debug registers.\n\n");
  ShowMessages("syntax : \t!dr core [core index "
               "(hex value)] pid [process id (hex value)] condition {[assembly "
               "in hex]} code {[assembly in hex]} buffer [pre-require buffer - "
               "(hex value)] \n");

  ShowMessages("\t\te.g : !dr\n");
  ShowMessages("\t\te.g : !dr pid 400\n");
  ShowMessages("\t\te.g : !dr core 2 pid 400\n");
}

VOID CommandDr(vector<string> SplittedCommand) {

  PDEBUGGER_GENERAL_EVENT_DETAIL Event;
  PDEBUGGER_GENERAL_ACTION Action;
  UINT32 EventLength;
  UINT32 ActionLength;

  //
  // Interpret and fill the general event and action fields
  //
  //
  if (!InterpretGeneralEventAndActionsFields(
          &SplittedCommand, DEBUG_REGISTERS_ACCESSED, &Event, &EventLength,
          &Action, &ActionLength)) {
    CommandDrHelp();
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
