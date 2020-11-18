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

//
// Global Variables
//
extern LIST_ENTRY g_OutputSources;
extern BOOLEAN g_OutputSourcesInitialized;

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
  DEBUGGER_EVENT_FORWARDING_TYPE Type;
  string DetailsOfSource;
  PLIST_ENTRY TempList = 0;
  BOOLEAN OutputSourceFound = FALSE;
  HANDLE SourceHandle = INVALID_HANDLE_VALUE;

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

    //
    // check if the parameters are okay for a create or not
    //
    if (SplittedCommand.size() <= 4) {
      ShowMessages("incorrect use of 'output'\n\n");
      CommandOutputHelp();
      return;
    }

    //
    // Check for the type of the output source
    //
    if (!SplittedCommand.at(3).compare("file")) {
      Type = EVENT_FORWARDING_FILE;
    } else if (!SplittedCommand.at(3).compare("namedpipe")) {
      Type = EVENT_FORWARDING_NAMEDPIPE;
    } else if (!SplittedCommand.at(3).compare("tcp")) {
      Type = EVENT_FORWARDING_TCP;
    } else {
      ShowMessages("incorrect type near '%s'\n\n",
                   SplittedCommand.at(3).c_str());
      CommandOutputHelp();
      return;
    }

    //
    // Check to make sure that the name doesn't exceed the maximum character
    //
    if (SplittedCommand.at(2).size() >=
        MAXIMUM_CHARACTERS_FOR_EVENT_FORWARDING_NAME) {

      ShowMessages("name of the output cannot exceed form %d.\n\n",
                   MAXIMUM_CHARACTERS_FOR_EVENT_FORWARDING_NAME);
      CommandOutputHelp();
      return;
    }

    //
    // Search to see if there is another output source with the
    // same name which we don't want to create two or more output
    // sources with the same name
    //

    if (g_OutputSourcesInitialized) {
      TempList = &g_OutputSources;

      while (&g_OutputSources != TempList->Flink) {

        TempList = TempList->Flink;

        PDEBUGGER_EVENT_FORWARDING CurrentOutputSourceDetails =
            CONTAINING_RECORD(TempList, DEBUGGER_EVENT_FORWARDING,
                              OutputSourcesList);

        if (strcmp(CurrentOutputSourceDetails->Name,
                   SplittedCommand.at(2).c_str()) == 0) {

          //
          // Indicate that we found this item
          //
          OutputSourceFound = TRUE;

          //
          // No need to search through the list anymore
          //
          break;
        }
      }

      //
      // Check whether the entered name already exists
      //
      if (OutputSourceFound) {

        ShowMessages("err, the name you entered, already exists, please choose "
                     "another name.\n\n");
        return;
      }
    }

    //
    // try to open the source and get the handle
    //

    DetailsOfSource = Command.substr(Command.find(SplittedCommand.at(3)) +
                                         SplittedCommand.at(3).size() + 1,
                                     Command.size());

    SourceHandle = ForwardingCreateOutputSource(Type, DetailsOfSource);

    //
    // Check if it's a valid handle or not
    //
    if (SourceHandle == INVALID_HANDLE_VALUE) {
      ShowMessages(
          "err, invalid address or cannot open or find the address.\n\n");
      return;
    }

    //
    // allocate the buffer for storing the event forwarding details
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

    //
    // Set the type
    //
    EventForwardingObject->Type = Type;

    //
    // Get a new tag
    //
    EventForwardingObject->OutputUniqueTag = ForwardingGetNewOutputSourceTag();

    //
    // Move the name of the output source to the buffer
    //
    strcpy_s(EventForwardingObject->Name, SplittedCommand.at(2).c_str());

    //
    // Check if list is initialized or not
    //
    if (!g_OutputSourcesInitialized) {
      InitializeListHead(&g_OutputSources);
      g_OutputSourcesInitialized = TRUE;
    }

    //
    // Add the source to the trace list
    //
    InsertHeadList(&g_OutputSources,
                   &(EventForwardingObject->OutputSourcesList));

  } else if (!SplittedCommand.at(1).compare("open")) {

    //
    // It's an open
    //

    //
    // Now we should find the corresponding object in the memory and
    // pass it to the global open functions
    //
    TempList = &g_OutputSources;

    while (&g_OutputSources != TempList->Flink) {

      TempList = TempList->Flink;

      PDEBUGGER_EVENT_FORWARDING CurrentOutputSourceDetails = CONTAINING_RECORD(
          TempList, DEBUGGER_EVENT_FORWARDING, OutputSourcesList);

      if (strcmp(CurrentOutputSourceDetails->Name,
                 SplittedCommand.at(2).c_str()) == 0) {
        //
        // Indicate that we found this item
        //
        OutputSourceFound = TRUE;

        //
        // Open the output
        //
        ForwardingOpenOutputSource(CurrentOutputSourceDetails);

        //
        // No need to search through the list anymore
        //
        break;
      }
    }

    if (!OutputSourceFound) {
      ShowMessages("err, the name you entered, not found.\n\n");
      return;
    }

  } else if (!SplittedCommand.at(1).compare("close")) {

    //
    // It's a close
    //

    //
    // Now we should find the corresponding object in the memory and
    // pass it to the global close functions
    //
    TempList = &g_OutputSources;

    while (&g_OutputSources != TempList->Flink) {

      TempList = TempList->Flink;

      PDEBUGGER_EVENT_FORWARDING CurrentOutputSourceDetails = CONTAINING_RECORD(
          TempList, DEBUGGER_EVENT_FORWARDING, OutputSourcesList);

      if (strcmp(CurrentOutputSourceDetails->Name,
                 SplittedCommand.at(2).c_str()) == 0) {

        //
        // Indicate that we found this item
        //
        OutputSourceFound = TRUE;

        //
        // Close the output
        //
        ForwardingCloseOutputSource(CurrentOutputSourceDetails);

        //
        // No need to search through the list anymore
        //
        break;
      }
    }

    if (!OutputSourceFound) {
      ShowMessages("err, the name you entered, not found.\n\n");
      return;
    }

  } else {
    //
    // Invalid argument
    //
    ShowMessages("incorrect option at '%s'\n\n", SplittedCommand.at(1).c_str());
    CommandOutputHelp();
    return;
  }
}
