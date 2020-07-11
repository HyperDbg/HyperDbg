/**
 * @file epthook.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief !epthook command
 * @details
 * @version 0.1
 * @date 2020-07-10
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */

#include "pch.h"

VOID CommandEptHookHelp() {
  ShowMessages("!epthook : Puts a hidden-hook EPT (hidden breakpoints) .\n\n");
  ShowMessages(
      "syntax : \t!epthook [Virtual Address (hex value)] core [core index "
      "(hex value)] pid [process id (hex value)] condition {[assembly "
      "in hex]} code {[assembly in hex]} buffer [pre-require buffer - (hex "
      "value)] \n");

  ShowMessages("\t\te.g : !epthook fffff801deadb000\n");
  ShowMessages("\t\te.g : !epthook fffff801deadb000 pid 400\n");
  ShowMessages("\t\te.g : !epthook fffff801deadb000 core 2 pid 400\n");
}

VOID CommandEptHook(vector<string> SplittedCommand) {

  PDEBUGGER_GENERAL_EVENT_DETAIL Event;
  PDEBUGGER_GENERAL_ACTION Action;
  UINT32 EventLength;
  UINT32 ActionLength;
  BOOLEAN GetAddress = FALSE;
  UINT64 OptionalParam1 = 0; // Set the target address

  if (SplittedCommand.size() < 2) {
    ShowMessages("incorrect use of '!epthook'\n");
    CommandEptHookHelp();
    return;
  }

  //
  // Interpret and fill the general event and action fields
  //
  if (!InterpretGeneralEventAndActionsFields(
          &SplittedCommand, HIDDEN_HOOK_EXEC_CC, &Event, &EventLength, &Action,
          &ActionLength)) {
    CommandEptHookHelp();
    return;
  }

  //
  // Interpret command specific details (if any)
  //
  for (auto Section : SplittedCommand) {
    if (!Section.compare("!epthook")) {
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
        CommandEptHookHelp();
        return;
      } else {
        GetAddress = TRUE;
      }
    } else {
      //
      // Unkonwn parameter
      //
      ShowMessages("Unknown parameter '%s'\n", Section.c_str());
      CommandEptHookHelp();
      return;
    }
  }
  if (OptionalParam1 == 0) {
    ShowMessages(
        "Please choose an address to put the hidden breakpoint on it.\n");
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
