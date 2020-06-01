/**
 * @file syscall-sysret.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief !cpuid commands
 * @details
 * @version 0.1
 * @date 2020-05-30
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */

#include "pch.h"

VOID CommandCpuidHelp() {
  ShowMessages("!cpuid : Monitors execution of a special cpuid index or all "
               "cpuids instructions.\n\n");
  ShowMessages("syntax : \t!cpuid core [core index "
               "(hex value)] pid [process id (hex value)] condition {[assembly "
               "in hex]} code {[assembly in hex]} buffer [pre-require buffer - "
               "(hex value)] \n");

  ShowMessages("\t\te.g : !cpuid\n");
  ShowMessages("\t\te.g : !cpuid pid 400\n");
  ShowMessages("\t\te.g : !cpuid core 2 pid 400\n");
}

VOID CommandCpuid(vector<string> SplittedCommand) {

  PDEBUGGER_GENERAL_EVENT_DETAIL Event;
  PDEBUGGER_GENERAL_ACTION Action;
  UINT32 EventLength;
  UINT32 ActionLength;

  //
  // Interpret and fill the general event and action fields
  //
  //
  if (!InterpretGeneralEventAndActionsFields(
          &SplittedCommand, CPUID_INSTRUCTION_EXECUTION, &Event, &EventLength,
          &Action, &ActionLength)) {
    CommandCpuidHelp();
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
