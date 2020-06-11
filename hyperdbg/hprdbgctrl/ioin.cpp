/**
 * @file ioin.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief !ioin command
 * @details
 * @version 0.1
 * @date 2020-06-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */

#include "pch.h"

VOID CommandIoinHelp() {
  ShowMessages("!ioin : Detects the execution of IN (I/O instructions) "
               "instructions.\n\n");
  ShowMessages("syntax : \t!ioin [port (hex value) - if not specific means "
               "all ports] core [core index (hex value)] pid [process id (hex "
               "value)] condition {[assembly "
               "in hex]} code {[assembly in hex]} buffer [pre-require buffer - "
               "(hex value)] \n");

  ShowMessages("\t\te.g : !ioin\n");
  ShowMessages("\t\te.g : !ioin 0x64\n");
  ShowMessages("\t\te.g : !ioin pid 400\n");
  ShowMessages("\t\te.g : !ioin core 2 pid 400\n");
}

VOID CommandIoin(vector<string> SplittedCommand) {

  PDEBUGGER_GENERAL_EVENT_DETAIL Event;
  PDEBUGGER_GENERAL_ACTION Action;
  UINT32 EventLength;
  UINT32 ActionLength;
  UINT64 SpecialTarget = DEBUGGER_EVENT_ALL_IO_PORTS;
  BOOLEAN GetPort = FALSE;

  //
  // Interpret and fill the general event and action fields
  //
  //
  if (!InterpretGeneralEventAndActionsFields(
          &SplittedCommand, IN_INSTRUCTION_EXECUTION, &Event, &EventLength,
          &Action, &ActionLength)) {
    CommandIoinHelp();
    return;
  }

  //
  // Interpret command specific details (if any)
  //
  for (auto Section : SplittedCommand) {
    if (!Section.compare("!ioin")) {
      continue;
    } else if (!GetPort) {

      //
      // It's probably an I/O port
      //
      if (!ConvertStringToUInt64(Section, &SpecialTarget)) {
        //
        // Unkonwn parameter
        //
        ShowMessages("Unknown parameter '%s'\n\n", Section.c_str());
        CommandIoinHelp();
        return;
      } else {
        GetPort = TRUE;
      }
    } else {
      //
      // Unkonwn parameter
      //
      ShowMessages("Unknown parameter '%s'\n", Section.c_str());
      CommandIoinHelp();
      return;
    }
  }

  //
  // Set the target I/O port
  //
  Event->OptionalParam1 = SpecialTarget;

  //
  // Send the ioctl to the kernel for event registeration
  //
  SendEventToKernel(Event, EventLength);

  //
  // Add the event to the kernel
  //
  RegisterActionToEvent(Action, ActionLength);
}
