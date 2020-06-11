/**
 * @file exception.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief !exception command
 * @details
 * @version 0.1
 * @date 2020-06-03
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */

#include "pch.h"

VOID CommandExceptionHelp() {
  ShowMessages("!exception : monitors the first 32 entry of IDT.\n\n");
  ShowMessages(
      "syntax : \t!exception [entry index (hex value) - if not specific means "
      "first 32 entries of IDT] core [core index (hex value)] pid [process id "
      "(hex "
      "value)] condition {[assembly "
      "in hex]} code {[assembly in hex]} buffer [pre-require buffer - "
      "(hex value)] \n");
  ShowMessages("\nNote that monitoring page-faults (entry 0xe) is implemented "
               "differently.\n");
  ShowMessages("\t\te.g : !exception\n");
  ShowMessages("\t\te.g : !exception 0xe\n");
  ShowMessages("\t\te.g : !exception pid 400\n");
  ShowMessages("\t\te.g : !exception core 2 pid 400\n");
}

VOID CommandException(vector<string> SplittedCommand) {

  PDEBUGGER_GENERAL_EVENT_DETAIL Event;
  PDEBUGGER_GENERAL_ACTION Action;
  UINT32 EventLength;
  UINT32 ActionLength;
  UINT64 SpecialTarget = DEBUGGER_EVENT_EXCEPTIONS_ALL_FIRST_32_ENTRIES;
  BOOLEAN GetEntry = FALSE;

  //
  // Interpret and fill the general event and action fields
  //
  //
  if (!InterpretGeneralEventAndActionsFields(
          &SplittedCommand, EXCEPTION_OCCURRED, &Event, &EventLength, &Action,
          &ActionLength)) {
    CommandExceptionHelp();
    return;
  }

  //
  // Interpret command specific details (if any)
  //
  for (auto Section : SplittedCommand) {
    if (!Section.compare("!exception")) {
      continue;
    } else if (!GetEntry) {

      //
      // It's probably an index
      //
      if (!ConvertStringToUInt64(Section, &SpecialTarget)) {
        //
        // Unkonwn parameter
        //
        ShowMessages("Unknown parameter '%s'\n\n", Section.c_str());
        CommandExceptionHelp();
        return;
      } else {
        //
        // Check if entry is valid or not
        //
        if (SpecialTarget >= 33) {
          //
          // Entry is invalid (this command is designed for just first 32
          // entries)
          //
          ShowMessages("The entry should be between 0x0 to 0x20 or first 32 "
                       "entries.'\n\n");
          CommandExceptionHelp();
          return;
        }
        GetEntry = TRUE;
      }
    } else {
      //
      // Unkonwn parameter
      //
      ShowMessages("Unknown parameter '%s'\n", Section.c_str());
      CommandExceptionHelp();
      return;
    }
  }

  //
  // Set the target exception (if not specific then it means all exceptions)
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
