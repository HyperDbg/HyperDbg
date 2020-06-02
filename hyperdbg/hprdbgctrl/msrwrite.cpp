/**
 * @file msrwrite.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief !msrwrite command
 * @details
 * @version 0.1
 * @date 2020-06-02
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */

#include "pch.h"

VOID CommandMsrwriteHelp() {
  ShowMessages("!msrwrite : detect the execution of wrmsr instructions.\n\n");
  ShowMessages("syntax : \t!msrwrite [msr (hex value) - if not specific means "
               "all msrs] core [core index (hex value)] pid [process id (hex "
               "value)] condition {[assembly "
               "in hex]} code {[assembly in hex]} buffer [pre-require buffer - "
               "(hex value)] \n");

  ShowMessages("\t\te.g : !msrwrite\n");
  ShowMessages("\t\te.g : !msrwrite 0xc0000082\n");
  ShowMessages("\t\te.g : !msrwrite pid 400\n");
  ShowMessages("\t\te.g : !msrwrite core 2 pid 400\n");
}

VOID CommandMsrwrite(vector<string> SplittedCommand) {

  PDEBUGGER_GENERAL_EVENT_DETAIL Event;
  PDEBUGGER_GENERAL_ACTION Action;
  UINT32 EventLength;
  UINT32 ActionLength;
  UINT64 SpecialTarget = DEBUGGER_EVENT_MSR_READ_OR_WRITE_ALL_MSRS;
  BOOLEAN GetAddress = FALSE;

  //
  // Interpret and fill the general event and action fields
  //
  //
  if (!InterpretGeneralEventAndActionsFields(
          &SplittedCommand, WRMSR_INSTRUCTION_EXECUTION, &Event, &EventLength,
          &Action, &ActionLength)) {
    CommandMsrwriteHelp();
    return;
  }

  //
  // Interpret command specific details (if any), it is because we can use
  // special msr bitmap here
  //
  for (auto Section : SplittedCommand) {
    if (!Section.compare("!msrwrite")) {
      continue;
    } else if (!GetAddress) {

      //
      // It's probably an msr
      //
      if (!ConvertStringToUInt64(Section, &SpecialTarget)) {
        //
        // Unkonwn parameter
        //
        ShowMessages("Unknown parameter '%s'\n\n", Section.c_str());
        CommandMsrwriteHelp();
        return;
      } else {
        GetAddress = TRUE;
      }
    } else {
      //
      // Unkonwn parameter
      //
      ShowMessages("Unknown parameter '%s'\n", Section.c_str());
      CommandMsrwriteHelp();
      return;
    }
  }

  //
  // Set the target msr (if not specific then it means all msrs)
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
