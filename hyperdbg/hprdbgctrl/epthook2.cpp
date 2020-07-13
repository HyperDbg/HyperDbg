/**
 * @file epthook2.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief !epthook2 command
 * @details
 * @version 0.1
 * @date 2020-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

VOID CommandEptHook2Help() {
  ShowMessages("!epthook2 : Puts a hidden-hook EPT (detours) .\n\n");
  ShowMessages(
      "syntax : \t!epthook2 [Virtual Address (hex value)] core [core index "
      "(hex value)] pid [process id (hex value)] condition {[assembly "
      "in hex]} code {[assembly in hex]} buffer [pre-require buffer - (hex "
      "value)] \n");

  ShowMessages("\t\te.g : !epthook2 fffff801deadb000\n");
  ShowMessages("\t\te.g : !epthook2 fffff801deadb000 pid 400\n");
  ShowMessages("\t\te.g : !epthook2 fffff801deadb000 core 2 pid 400\n");
}

VOID CommandEptHook2(vector<string> SplittedCommand) {

  PDEBUGGER_GENERAL_EVENT_DETAIL Event;
  PDEBUGGER_GENERAL_ACTION Action;
  UINT32 EventLength;
  UINT32 ActionLength;
  BOOLEAN GetAddress = FALSE;
  UINT64 OptionalParam1 = 0; // Set the target address

  if (SplittedCommand.size() < 2) {
    ShowMessages("incorrect use of '!epthook2'\n");
    CommandEptHook2Help();
    return;
  }

  //
  // Interpret and fill the general event and action fields
  //

  if (!InterpretGeneralEventAndActionsFields(
          &SplittedCommand, HIDDEN_HOOK_EXEC_DETOURS, &Event, &EventLength,
          &Action, &ActionLength)) {
    CommandEptHook2Help();
    return;
  }

  //
  // Interpret command specific details (if any)
  //
  for (auto Section : SplittedCommand) {
    if (!Section.compare("!epthook2")) {
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
        CommandEptHook2Help();
        return;
      } else {
        GetAddress = TRUE;
      }
    } else {
      //
      // Unkonwn parameter
      //
      ShowMessages("Unknown parameter '%s'\n", Section.c_str());
      CommandEptHook2Help();
      return;
    }
  }
  if (OptionalParam1 == 0) {
    ShowMessages("Please choose an address to put the hook on it.\n");
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
