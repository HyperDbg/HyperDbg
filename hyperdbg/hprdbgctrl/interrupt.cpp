/**
 * @file interrupt.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief !interrupt command
 * @details
 * @version 0.1
 * @date 2020-06-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */

#include "pch.h"

VOID CommandInterruptHelp() {
  ShowMessages("!interrupt : monitors the external interrupt (IDT > 32).\n\n");
  ShowMessages("syntax : \t!interrupt [entry index (hex value) - should be "
               "selected] core [core index (hex value)] pid [process id (hex "
               "value)] condition {[assembly in hex]} code {[assembly in hex]} "
               "buffer [pre-require buffer - (hex value)] \n");
  ShowMessages("\nNote : The index should be greater than 0x20 (33) and less "
               "than 0xFF (255).\n");
  ShowMessages("\t\te.g : !interrupt 0x2f\n");
  ShowMessages("\t\te.g : !interrupt 0x2f pid 400\n");
  ShowMessages("\t\te.g : !interrupt 0x2f core 2 pid 400\n");
}

VOID CommandInterrupt(vector<string> SplittedCommand) {

  PDEBUGGER_GENERAL_EVENT_DETAIL Event;
  PDEBUGGER_GENERAL_ACTION Action;
  UINT32 EventLength;
  UINT32 ActionLength;
  UINT64 SpecialTarget = 0;
  BOOLEAN GetEntry = FALSE;

  //
  // Interpret and fill the general event and action fields
  //
  //
  if (!InterpretGeneralEventAndActionsFields(
          &SplittedCommand, EXTERNAL_INTERRUPT_OCCURRED, &Event, &EventLength,
          &Action, &ActionLength)) {
    CommandInterruptHelp();
    return;
  }

  //
  // Interpret command specific details (if any)
  // 
  //
  for (auto Section : SplittedCommand) {
    if (!Section.compare("!interrupt")) {
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
        CommandInterruptHelp();
        return;
      } else {
        //
        // Check if entry is valid or not
        //
        if (!(SpecialTarget >= 33 && SpecialTarget <= 0xff)) {
          //
          // Entry is invalid (this command is designed for just entries
          // between 33 to 255)
          //
          ShowMessages("The entry should be between 0x21 to 0xFF or the "
                       "entries between 33 to 255.\n\n");
          CommandInterruptHelp();
          return;
        }
        GetEntry = TRUE;
      }
    } else {
      //
      // Unkonwn parameter
      //
      ShowMessages("Unknown parameter '%s'\n", Section.c_str());
      CommandInterruptHelp();
      return;
    }
  }

  if (SpecialTarget == 0) {
    //
    // The user didn't set the target interrupt, even though it's possible to
    // get all interrupts but it makes the system not resposive so it's wrong
    // to trigger event on all interrupts and we're not going to support it
    //
    ShowMessages("Please specify an interrupt index to monitor, HyperDbg "
                 "doesn't support to trigger events on all interrupts because "
                 "it's not reasonable and make the system unresponsive\n");
    CommandInterruptHelp();
    return;
  }

  //
  // Set the target interrupt
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
