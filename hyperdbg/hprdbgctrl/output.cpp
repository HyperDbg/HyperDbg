/**
 * @file output.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief output command
 * @details
 * @version 0.1
 * @date 2020-11-05
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#define MAXIMUM_CHARACTERS_FOR_EVENT_FORWARDING_NAME 50

/**
 * @brief event forwarding type
 *
 */
typedef enum _DEBUGGER_EVENT_FORWARDING_TYPE {
  EVENT_FORWARDING_NAMEDPIPE,
  EVENT_FORWARDING_FILE,
  EVENT_FORWARDING_TCP
} DEBUGGER_EVENT_FORWARDING_TYPE;

/**
 * @brief event forwarding states
 *
 */
typedef enum _DEBUGGER_EVENT_FORWARDING_STATE {
  EVENT_FORWARDING_STATE_NOT_OPENNED,
  EVENT_FORWARDING_STATE_OPENNED,
  EVENT_FORWARDING_CLOSED
} DEBUGGER_EVENT_FORWARDING_STATE;

/**
 * @brief structures hold the detail of event forwarding
 *
 */
typedef struct _DEBUGGER_EVENT_FORWARDING {

  DEBUGGER_EVENT_FORWARDING_TYPE Type;
  DEBUGGER_EVENT_FORWARDING_STATE State;
  HANDLE Handle;
  UINT64 OutputUniqueTag;
  CHAR Name[MAXIMUM_CHARACTERS_FOR_EVENT_FORWARDING_NAME];

} DEBUGGER_EVENT_FORWARDING, *PDEBUGGER_EVENT_FORWARDING;

/**
 * @brief help of output command
 *
 * @return VOID
 */
VOID CommandOutputHelp() {
  ShowMessages("output : create an output instance that can be used in event "
               "forwarding.\n\n");
  ShowMessages("syntax : \toutput [create|open|close] type "
               "[file|namedpipe|tcp] address\n");
  ShowMessages("\t\te.g : output create MyOutputName1 file "
               "c:\\users\\sina\\desktop\\output.txt\n");
  ShowMessages("\t\te.g : output create MyOutputName2 tcp 192.168.1.10:8080\n");
  ShowMessages("\t\te.g : output create MyOutputName3 namedpipe "
               "\\\\.\\Pipe\\HyperDbgOutput\n");
  ShowMessages("\t\te.g : output open MyOutputName1\n");
  ShowMessages("\t\te.g : output close MyOutputName1\n");
}

/**
 * @brief output command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID CommandOutput(vector<string> SplittedCommand, string Command) {

  PDEBUGGER_EVENT_FORWARDING EventForwardingObject;

  if (SplittedCommand.size() <= 2) {
    ShowMessages("incorrect use of 'output'\n\n");
    CommandOutputHelp();
    return;
  }

  //
  // Check if it's a create, open, or close
  //
  if (!SplittedCommand.at(1).compare("create")) {
    //
    // It's a create
    //
    EventForwardingObject =
        (PDEBUGGER_EVENT_FORWARDING)malloc(sizeof(DEBUGGER_EVENT_FORWARDING));

    if (EventForwardingObject == NULL) {
      ShowMessages("err, in allocating memory for event forwarding.\n\n");
      return;
    }

    RtlZeroMemory(EventForwardingObject, sizeof(DEBUGGER_EVENT_FORWARDING));

    //
    // Set the state
    //
    EventForwardingObject->State = EVENT_FORWARDING_STATE_NOT_OPENNED;

  } else if (!SplittedCommand.at(1).compare("open")) {
    //
    // It's an open
    //
  } else if (!SplittedCommand.at(1).compare("close")) {
    //
    // It's a close
    //
  } else {
    //
    // Invalid argument
    //
    ShowMessages("incorrect option at '%s'\n\n", SplittedCommand.at(1).c_str());
    CommandOutputHelp();
    return;
  }
}
