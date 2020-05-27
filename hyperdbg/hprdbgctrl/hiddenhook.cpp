/**
 * @file hiddenhook.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief !hiddenhook command
 * @details
 * @version 0.1
 * @date 2020-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

VOID CommandHiddenHookHelp() {
  ShowMessages("!hiddenhook : Puts a hidden-hook EPT (detours) .\n\n");
  ShowMessages(
      "syntax : \t!hiddenhook [Virtual Address (hex value)] core [core index "
      "(hex value)] pid [process id (hex value)] condition {[assembly "
      "in hex]} code {[assembly in hex]} buffer [pre-require buffer - "
      "(hex value)] \n");

  ShowMessages("\t\te.g : !hiddenhook fffff801deadb000\n");
  ShowMessages("\t\te.g : !hiddenhook fffff801deadb000 pid 400\n");
  ShowMessages("\t\te.g : !hiddenhook fffff801deadb000 core 2 pid 400\n");
}

VOID CommandHiddenHook(vector<string> SplittedCommand) {

  PDEBUGGER_GENERAL_EVENT_DETAIL Event;
  PDEBUGGER_GENERAL_ACTION Action;
  UINT32 EventLength;
  UINT32 ActionLength;
  BOOLEAN GetAddress = FALSE;
  UINT64 OptionalParam1 = 0; // Set the target address

  if (SplittedCommand.size() < 2) {
    ShowMessages("incorrect use of '!hiddenhook'\n");
    CommandHiddenHookHelp();
    return;
  }

  //
  // Interpret and fill the general event and action fields
  //

  if (!InterpretGeneralEventAndActionsFields(
          &SplittedCommand, HIDDEN_HOOK_EXEC_DETOURS, &Event, &EventLength,
          &Action, &ActionLength)) {
    CommandHiddenHookHelp();
    return;
  }

  //
  // Interpret command specific details (if any)
  //

  for (auto Section : SplittedCommand) {
    if (!Section.compare("!hiddenhook")) {
      continue;
    } else if (!GetAddress) {
      //
      // It's probably address
      //
      if (!ConvertStringToUInt64(Section, &OptionalParam1)) {
        //
        // Unkonwn parameter
        //
        ShowMessages("Unknown parameter '%s'\n\n", Section.c_str());
        CommandHiddenHookHelp();
        return;
      } else {
        GetAddress = TRUE;
      }
    } else {
      //
      // Unkonwn parameter
      //
      ShowMessages("Unknown parameter '%s'\n", Section.c_str());
      CommandHiddenHookHelp();
      return;
    }
  }
  if (OptionalParam1 == 0) {
    ShowMessages("Please choose an address to put the breakpoint on it.\n");
    return;
  }

  //
  // Set the optional parameters
  //
  Event->OptionalParam1 = OptionalParam1;

  //
  // Send the ioctl to the kernel for event registeration
  //
  SendEventToKernel(Event, EventLength);

  //
  // Add the event to the kernel
  //
  RegisterActionToEvent(Action, ActionLength);
}
