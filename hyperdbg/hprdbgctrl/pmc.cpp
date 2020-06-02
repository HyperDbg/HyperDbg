/**
 * @file pmc.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief !pmc commands
 * @details
 * @version 0.1
 * @date 2020-06-03
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */

#include "pch.h"

VOID CommandPmcHelp() {
  ShowMessages("!pmc : Monitors execution of rdpmc instructions.\n\n");
  ShowMessages("syntax : \t!tsc core [core index "
               "(hex value)] pid [process id (hex value)] condition {[assembly "
               "in hex]} code {[assembly in hex]} buffer [pre-require buffer - "
               "(hex value)] \n");

  ShowMessages("\t\te.g : !pmc\n");
  ShowMessages("\t\te.g : !pmc pid 400\n");
  ShowMessages("\t\te.g : !pmc core 2 pid 400\n");
}

VOID CommandPmc(vector<string> SplittedCommand) {

  PDEBUGGER_GENERAL_EVENT_DETAIL Event;
  PDEBUGGER_GENERAL_ACTION Action;
  UINT32 EventLength;
  UINT32 ActionLength;

  //
  // Interpret and fill the general event and action fields
  //
  //
  if (!InterpretGeneralEventAndActionsFields(
          &SplittedCommand, PMC_INSTRUCTION_EXECUTION, &Event, &EventLength,
          &Action, &ActionLength)) {
    CommandPmcHelp();
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
