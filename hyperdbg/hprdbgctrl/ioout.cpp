/**
 * @file ioout.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief !ioout command
 * @details
 * @version 0.1
 * @date 2020-06-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */

#include "pch.h"

VOID CommandIooutHelp() {
  ShowMessages("!ioout : Detects the execution of OUT (I/O instructions) "
               "instructions.\n\n");
  ShowMessages("syntax : \t!ioout [port (hex value) - if not specific means "
               "all ports] core [core index (hex value)] pid [process id (hex "
               "value)] condition {[assembly "
               "in hex]} code {[assembly in hex]} buffer [pre-require buffer - "
               "(hex value)] \n");

  ShowMessages("\t\te.g : !ioout\n");
  ShowMessages("\t\te.g : !ioout 0x64\n");
  ShowMessages("\t\te.g : !ioout pid 400\n");
  ShowMessages("\t\te.g : !ioout core 2 pid 400\n");
}

VOID CommandIoout(vector<string> SplittedCommand) {

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
          &SplittedCommand, OUT_INSTRUCTION_EXECUTION, &Event, &EventLength,
          &Action, &ActionLength)) {
    CommandIooutHelp();
    return;
  }

  //
  // Interpret command specific details (if any)
  // 
  //
  for (auto Section : SplittedCommand) {
    if (!Section.compare("!ioout")) {
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
        CommandIooutHelp();
        return;
      } else {
        GetPort = TRUE;
      }
    } else {
      //
      // Unkonwn parameter
      //
      ShowMessages("Unknown parameter '%s'\n", Section.c_str());
      CommandIooutHelp();
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
