/**
 * @file tsc.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief !tsc commands
 * @details
 * @version 0.1
 * @date 2020-06-02
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */

#include "pch.h"

VOID CommandTscHelp() {
  ShowMessages("!tsc : Monitors execution of rdtsc/rdtscp instructions.\n\n");
  ShowMessages("syntax : \t!tsc core [core index "
               "(hex value)] pid [process id (hex value)] condition {[assembly "
               "in hex]} code {[assembly in hex]} buffer [pre-require buffer - "
               "(hex value)] \n");

  ShowMessages("\t\te.g : !tsc\n");
  ShowMessages("\t\te.g : !tsc pid 400\n");
  ShowMessages("\t\te.g : !tsc core 2 pid 400\n");
}

VOID CommandTsc(vector<string> SplittedCommand) {

  PDEBUGGER_GENERAL_EVENT_DETAIL Event;
  PDEBUGGER_GENERAL_ACTION Action;
  UINT32 EventLength;
  UINT32 ActionLength;

  //
  // Interpret and fill the general event and action fields
  //
  //
  if (!InterpretGeneralEventAndActionsFields(
          &SplittedCommand, TSC_INSTRUCTION_EXECUTION, &Event, &EventLength,
          &Action, &ActionLength)) {
    CommandTscHelp();
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
