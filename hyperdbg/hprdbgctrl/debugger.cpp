/**
 * @file debugger.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Interpret general fields
 * @details
 * @version 0.1
 * @date 2020-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern UINT64 g_EventTag;
extern LIST_ENTRY g_EventTrace;
extern BOOLEAN g_EventTraceInitialized;

/**
 * @brief Interpret conditions (if an event has condition) and custom code
 * @details If this function returns true then it means that there is a condtion
 * or code buffer in this command split and the details are returned in the
 * input structure
 *
 * @param SplittedCommand All the commands
 */
BOOLEAN
InterpretConditionsAndCodes(vector<string> *SplittedCommand,
                            BOOLEAN IsConditionBuffer, PUINT64 BufferAddrss,
                            PUINT32 BufferLength) {
  BOOLEAN IsTextVisited = FALSE;
  BOOLEAN IsInState = FALSE;
  BOOLEAN IsEnded = FALSE;
  string Temp;
  string AppendedFinalBuffer;
  vector<string> SaveBuffer;
  vector<CHAR> ParsedBytes;
  unsigned char *FinalBuffer;
  vector<int> IndexesToRemove;
  int Index = 0;

  for (auto Section : *SplittedCommand) {
    Index++;

    if (IsInState) {

      //
      // Check if the buffer is ended or not
      //
      if (!Section.compare("}")) {
        //
        // Save to remove this string from the command
        //
        IndexesToRemove.push_back(Index);
        IsEnded = TRUE;
        break;
      }

      //
      // Check if the condition is end or not
      //
      if (HasEnding(Section, "}")) {
        //
        // Save to remove this string from the command
        //
        IndexesToRemove.push_back(Index);

        //
        // remove the last character and append it to the ConditionBuffer
        //
        SaveBuffer.push_back(Section.substr(0, Section.size() - 1));

        IsEnded = TRUE;
        break;
      }

      //
      // Save to remove this string from the command
      //
      IndexesToRemove.push_back(Index);

      //
      // Add the codes into condition bi
      //
      SaveBuffer.push_back(Section);

      //
      // We want to stay in this condition
      //
      continue;
    }

    if (IsTextVisited && !Section.compare("{")) {
      //
      // Save to remove this string from the command
      //
      IndexesToRemove.push_back(Index);

      IsInState = TRUE;
      continue;
    }
    if (IsTextVisited && Section.rfind("{", 0) == 0) {
      //
      // Section starts with {
      //

      //
      // Check if it ends with }
      //
      if (HasEnding(Section, "}")) {

        //
        // Save to remove this string from the command
        //
        IndexesToRemove.push_back(Index);

        Temp = Section.erase(0, 1);
        SaveBuffer.push_back(Temp.substr(0, Temp.size() - 1));

        IsEnded = TRUE;
        break;
      }
      //
      // Save to remove this string from the command
      //
      IndexesToRemove.push_back(Index);

      SaveBuffer.push_back(Section.erase(0, 1));

      IsInState = TRUE;
      continue;
    }

    if (IsConditionBuffer) {
      if (!Section.compare("condition")) {

        //
        // Save to remove this string from the command
        //
        IndexesToRemove.push_back(Index);

        IsTextVisited = TRUE;
        continue;
      }
    } else {
      //
      // It's code
      //
      if (!Section.compare("code")) {

        //
        // Save to remove this string from the command
        //
        IndexesToRemove.push_back(Index);

        IsTextVisited = TRUE;
        continue;
      }
    }

    if (IsConditionBuffer) {
      if (!Section.compare("condition{")) {

        //
        // Save to remove this string from the command
        //
        IndexesToRemove.push_back(Index);

        IsTextVisited = TRUE;
        IsInState = TRUE;
        continue;
      }
    } else {
      //
      // It's code
      //
      if (!Section.compare("code{")) {

        //
        // Save to remove this string from the command
        //
        IndexesToRemove.push_back(Index);

        IsTextVisited = TRUE;
        IsInState = TRUE;
        continue;
      }
    }

    if (IsConditionBuffer) {
      if (Section.rfind("condition{", 0) == 0) {

        //
        // Save to remove this string from the command
        //
        IndexesToRemove.push_back(Index);

        IsTextVisited = TRUE;
        IsInState = TRUE;

        if (!HasEnding(Section, "}")) {
          //
          // Section starts with condition{
          //
          SaveBuffer.push_back(Section.erase(0, 10));
          continue;
        } else {
          //
          // remove the last character and first character append it to the
          // ConditionBuffer
          //
          Temp = Section.erase(0, 10);
          SaveBuffer.push_back(Temp.substr(0, Temp.size() - 1));

          IsEnded = TRUE;
          break;
        }
      }
    } else {
      //
      // It's a code
      //

      if (Section.rfind("code{", 0) == 0) {

        //
        // Save to remove this string from the command
        //
        IndexesToRemove.push_back(Index);

        IsTextVisited = TRUE;
        IsInState = TRUE;

        if (!HasEnding(Section, "}")) {
          //
          // Section starts with condition{
          //
          SaveBuffer.push_back(Section.erase(0, 5));
          continue;
        } else {
          //
          // remove the last character and first character append it to the
          // ConditionBuffer
          //
          Temp = Section.erase(0, 5);
          SaveBuffer.push_back(Temp.substr(0, Temp.size() - 1));

          IsEnded = TRUE;
          break;
        }
      }
    }
  }

  //
  // Now we have everything in condition buffer
  // Check to see if it is empty or not
  //
  if (SaveBuffer.size() == 0) {
    //
    // Nothing in condition buffer, return zero
    //
    return FALSE;
  }

  //
  // Check if we see '}' at the end
  //
  if (!IsEnded) {
    //
    // Nothing in condition buffer, return zero
    //
    return FALSE;
  }

  //
  // Append a 'ret' at the end of the buffer
  //
  SaveBuffer.push_back("c3");

  //
  // If we reach here then there is sth in condition buffer
  //
  for (auto Section : SaveBuffer) {

    //
    // Check if the section is started with '0x'
    //
    if (Section.rfind("0x", 0) == 0 || Section.rfind("0X", 0) == 0 ||
        Section.rfind("\\x", 0) == 0 || Section.rfind("\\X", 0) == 0) {
      Temp = Section.erase(0, 2);
    } else if (Section.rfind("x", 0) == 0 || Section.rfind("X", 0) == 0) {
      Temp = Section.erase(0, 1);
    } else {
      Temp = Section;
    }

    //
    // replace \x s
    //
    ReplaceAll(Temp, "\\x", "");

    //
    // check if the buffer is aligned to 2
    //
    if (Temp.size() % 2 != 0) {

      //
      // Add a zero to the start of the buffer
      //
      Temp.insert(0, 1, '0');
    }

    if (!IsHexNotation(Temp)) {
      ShowMessages("Please enter condition code in a hex notation.\n");
      return FALSE;
    }
    AppendedFinalBuffer.append(Temp);
  }

  //
  // Convert it to vectored bytes
  //
  ParsedBytes = HexToBytes(AppendedFinalBuffer);

  //
  // Convert to a contigues buffer
  //
  FinalBuffer = (unsigned char *)malloc(ParsedBytes.size());
  std::copy(ParsedBytes.begin(), ParsedBytes.end(), FinalBuffer);

  //
  // Set the buffer and length
  //
  *BufferAddrss = (UINT64)FinalBuffer;
  *BufferLength = ParsedBytes.size();

  //
  // Removing the code or condition indexes from the command
  //
  int NewIndexToRemove = 0;
  for (auto IndexToRemove : IndexesToRemove) {
    NewIndexToRemove++;

    SplittedCommand->erase(SplittedCommand->begin() +
                           (IndexToRemove - NewIndexToRemove));
  }
  return TRUE;
}

/* ==============================================================================================
 */

/**
 * @brief Register the event to the kernel
 */

BOOLEAN
SendEventToKernel(PDEBUGGER_GENERAL_EVENT_DETAIL Event,
                  UINT32 EventBufferLength) {
  BOOL Status;
  ULONG ReturnedLength;
  DEBUGGER_EVENT_AND_ACTION_REG_BUFFER ReturnedBuffer = {0};

  //
  // Test
  //
  /*
  ShowMessages("Tag : %llx\n", Event->Tag);
  ShowMessages("Command String : %s\n", Event->CommandStringBuffer);
  ShowMessages("CoreId : 0x%x\n", Event->CoreId);
  ShowMessages("Pid : 0x%x\n", Event->ProcessId);
  ShowMessages("Optional Param 1 : %llx\n", Event->OptionalParam1);
  ShowMessages("Optional Param 2 : %llx\n", Event->OptionalParam2);
  ShowMessages("Optional Param 3 : %llx\n", Event->OptionalParam3);
  ShowMessages("Optional Param 4 : %llx\n", Event->OptionalParam4);
  ShowMessages("Count of Actions : %d\n", Event->CountOfActions);
  ShowMessages("Event Type : %d\n", Event->EventType);
  return TRUE;
  */

  if (!g_DeviceHandle) {
    ShowMessages("Handle not found, probably the driver is not loaded.\n");
    return FALSE;
  }

  //
  // Send IOCTL
  //

  Status =
      DeviceIoControl(g_DeviceHandle,                  // Handle to device
                      IOCTL_DEBUGGER_REGISTER_EVENT, // IO Control code
                      Event,                         // Input Buffer to driver.
                      EventBufferLength,             // Input buffer length
                      &ReturnedBuffer, // Output Buffer from driver.
                      sizeof(DEBUGGER_EVENT_AND_ACTION_REG_BUFFER), // Length
                                                                    // of
                                                                    // output
                                                                    // buffer
                                                                    // in
                                                                    // bytes.
                      &ReturnedLength, // Bytes placed in buffer.
                      NULL             // synchronous call
      );

  if (!Status) {
    ShowMessages("Ioctl failed with code 0x%x\n", GetLastError());
    return FALSE;
  }

  return TRUE;
}

/* ==============================================================================================
 */

/**
 * @brief Register the action to the event
 */

BOOLEAN
RegisterActionToEvent(PDEBUGGER_GENERAL_ACTION Action,
                      UINT32 ActionsBufferLength) {
  BOOL Status;
  ULONG ReturnedLength;
  DEBUGGER_EVENT_AND_ACTION_REG_BUFFER ReturnedBuffer = {0};

  //
  // Test
  //
  /*
  ShowMessages("Tag : %llx\n", Action->EventTag);
  ShowMessages("Action Type : %d\n", Action->ActionType);
  ShowMessages("Custom Code Buffer Size : 0x%x\n",
               Action->CustomCodeBufferSize);
  return TRUE;
  */

  if (!g_DeviceHandle) {
    ShowMessages("Handle not found, probably the driver is not loaded.\n");
    return FALSE;
  }

  //
  // Send IOCTL
  //

  Status =
      DeviceIoControl(g_DeviceHandle,                       // Handle to device
                      IOCTL_DEBUGGER_ADD_ACTION_TO_EVENT, // IO Control code
                      Action,              // Input Buffer to driver.
                      ActionsBufferLength, // Input buffer length
                      &ReturnedBuffer,     // Output Buffer from driver.
                      sizeof(DEBUGGER_EVENT_AND_ACTION_REG_BUFFER), // Length
                                                                    // of
                                                                    // output
                                                                    // buffer
                                                                    // in
                                                                    // bytes.
                      &ReturnedLength, // Bytes placed in buffer.
                      NULL             // synchronous call
      );

  if (!Status) {
    ShowMessages("Ioctl failed with code 0x%x\n", GetLastError());
    return FALSE;
  }

  return TRUE;
}

UINT64 GetNewDebuggerEventTag() { return g_EventTag++; }

/**
 * @brief Interpret general event fields
 * @details If this function returns true then it means that there
 * was no error in parsing the general event details
 *
 * @param SplittedCommand All the commands
 */
BOOLEAN InterpretGeneralEventAndActionsFields(
    vector<string> *SplittedCommand, DEBUGGER_EVENT_TYPE_ENUM EventType,
    PDEBUGGER_GENERAL_EVENT_DETAIL *EventDetailsToFill,
    PUINT32 EventBufferLength, PDEBUGGER_GENERAL_ACTION *ActionDetailsToFill,
    PUINT32 ActionBufferLength) {

  PDEBUGGER_GENERAL_EVENT_DETAIL TempEvent;
  UINT64 ConditionBufferAddress;
  UINT32 ConditionBufferLength = 0;
  UINT64 CodeBufferAddress;
  UINT32 CodeBufferLength = 0;
  UINT32 LengthOfEventBuffer = 0;
  string CommandString;
  BOOLEAN HasConditionBuffer = FALSE;
  BOOLEAN HasCodeBuffer = FALSE;
  BOOLEAN IsNextCommandPid = FALSE;
  BOOLEAN IsNextCommandCoreId = FALSE;
  BOOLEAN IsNextCommandBufferSize = FALSE;
  UINT32 CoreId;
  UINT32 ProcessId;
  UINT32 RequestBuffer = 0;
  vector<int> IndexesToRemove;
  int NewIndexToRemove = 0;
  int Index = 0;

  //
  // Create a command string to show in the history
  //
  for (auto Section : *SplittedCommand) {

    CommandString.append(Section);
    CommandString.append(" ");
  }
  //
  // Compute the size of buffer + 1 null for the end of buffer
  //

  UINT64 BufferOfCommandStringLength = CommandString.size() + 1;

  //
  // Allocate Buffer and zero for command to the buffer
  //
  PVOID BufferOfCommandString = malloc(BufferOfCommandStringLength);

  RtlZeroMemory(BufferOfCommandString, BufferOfCommandStringLength);

  //
  // Copy the string to the buffer
  //
  memcpy(BufferOfCommandString, CommandString.c_str(), CommandString.size());

  //
  // Check if there is a condition buffer in the command
  //
  if (!InterpretConditionsAndCodes(SplittedCommand, TRUE,
                                   &ConditionBufferAddress,
                                   &ConditionBufferLength)) {

    //
    // Indicate condition is not available
    //
    HasConditionBuffer = FALSE;

    //
    // ShowMessages("\nNo condition!\n");
    //

  } else {
    //
    // Indicate condition is available
    //
    HasConditionBuffer = TRUE;

    ShowMessages(
        "\n========================= Condition =========================\n");

    ShowMessages(
        "\nUINT64  DebuggerCheckForCondition(PGUEST_REGS Regs_RCX, PVOID "
        "Context_RDX)\n{\n");

    //
    // Disassemble the buffer
    //
    HyperDbgDisassembler((unsigned char *)ConditionBufferAddress, 0x0,
                         ConditionBufferLength);

    ShowMessages("}\n\n");

    ShowMessages(
        "=============================================================\n");
  }

  //
  // Check if there is a code buffer in the command
  //
  if (!InterpretConditionsAndCodes(SplittedCommand, FALSE, &CodeBufferAddress,
                                   &CodeBufferLength)) {

    //
    // Indicate code is not available
    //
    HasCodeBuffer = FALSE;
    //
    // ShowMessages("\nNo custom code!\n");
    //
  } else {

    //
    // Indicate code is available
    //
    HasCodeBuffer = TRUE;

    ShowMessages(
        "\n=========================    Code    =========================\n");
    ShowMessages("\nPVOID DebuggerRunCustomCodeFunc(PVOID "
                 "PreAllocatedBufferAddress_RCX, "
                 "PGUEST_REGS Regs_RDX, PVOID Context_R8)\n{\n");

    //
    // Disassemble the buffer
    //
    HyperDbgDisassembler((unsigned char *)CodeBufferAddress, 0x0,
                         CodeBufferLength);

    ShowMessages("}\n\n");

    ShowMessages(
        "=============================================================\n");
  }

  //
  // Create action and event based on previously parsed buffers
  // (DEBUGGER_GENERAL_ACTION)
  //
  // Allocate the buffer (with ConditionBufferLength and CodeBufferLength)
  //

  /*

  Layout of Buffer
   ________________________________
  |                                |
  |  DEBUGGER_GENERAL_EVENT_DETAIL |
  |                                |
  |________________________________|
  |                                |
  |       Condition Buffer         |
  |                                |
  |________________________________|
  |                                |
  |     DEBUGGER_GENERAL_ACTION    |
  |                                |
  |________________________________|
  |                                |
  |     Condition Custom Code      |
  |                                |
  |________________________________|


  */

  LengthOfEventBuffer =
      sizeof(DEBUGGER_GENERAL_EVENT_DETAIL) + ConditionBufferLength;

  TempEvent = (PDEBUGGER_GENERAL_EVENT_DETAIL)malloc(LengthOfEventBuffer);
  RtlZeroMemory(TempEvent, LengthOfEventBuffer);

  //
  // Check if buffer is availabe
  //
  if (TempEvent == NULL) {
    //
    // The heap is not available
    //
    return FALSE;
  }

  //
  // Event is enabled by default when created
  //
  TempEvent->IsEnabled = TRUE;

  //
  // Get a new tag for it
  //
  TempEvent->Tag = GetNewDebuggerEventTag();

  //
  // Set the core Id and Process Id to all cores and all
  // processes, next time we check whether the user needs
  // a special core or a special process then we change it
  //
  TempEvent->CoreId = DEBUGGER_EVENT_APPLY_TO_ALL_CORES;
  TempEvent->ProcessId = DEBUGGER_EVENT_APPLY_TO_ALL_PROCESSES;

  //
  // Set the event type
  //
  TempEvent->EventType = EventType;

  //
  // Get the current time
  //
  TempEvent->CreationTime = time(0);

  //
  // Set buffer string command
  //
  TempEvent->CommandStringBuffer = BufferOfCommandString;

  //
  // Fill the buffer of condition for event
  //
  if (HasConditionBuffer) {

    memcpy((PVOID)((UINT64)TempEvent + sizeof(DEBUGGER_GENERAL_EVENT_DETAIL)),
           (PVOID)ConditionBufferAddress, ConditionBufferLength);

    //
    // Set the size of the buffer for event condition
    //
    TempEvent->ConditionBufferSize = ConditionBufferLength;
  }

  //
  // Allocate the Action
  //
  UINT32 LengthOfActionBuffer =
      sizeof(DEBUGGER_GENERAL_ACTION) + CodeBufferLength;
  PDEBUGGER_GENERAL_ACTION TempAction =
      (PDEBUGGER_GENERAL_ACTION)malloc(LengthOfActionBuffer);

  RtlZeroMemory(TempAction, LengthOfActionBuffer);
  //
  // Fill the buffer of custom code for action
  //
  if (HasCodeBuffer) {

    memcpy((PVOID)((UINT64)TempAction + sizeof(DEBUGGER_GENERAL_ACTION)),
           (PVOID)CodeBufferAddress, CodeBufferLength);
    //
    // Set the action Tag
    //
    TempAction->EventTag = TempEvent->Tag;

    //
    // Set the action type
    //
    TempAction->ActionType = RUN_CUSTOM_CODE;

    //
    // Set the action buffer size
    //
    TempAction->CustomCodeBufferSize = CodeBufferLength;

    //
    // Increase the count of actions
    //
    TempEvent->CountOfActions = TempEvent->CountOfActions + 1;
  }

  //
  // Interpret rest of the command
  //

  for (auto Section : *SplittedCommand) {
    Index++;
    if (IsNextCommandBufferSize) {

      if (!ConvertStringToUInt32(Section, &RequestBuffer)) {
        return FALSE;
      } else {
        //
        // Set the specific requested buffer size
        //
        TempAction->PreAllocatedBuffer = RequestBuffer;
      }
      IsNextCommandBufferSize = FALSE;

      //
      // Add index to remove it from the command
      //
      IndexesToRemove.push_back(Index);

      continue;
    }
    if (IsNextCommandPid) {

      if (!ConvertStringToUInt32(Section, &ProcessId)) {
        return FALSE;
      } else {
        //
        // Set the specific process id
        //
        TempEvent->ProcessId = ProcessId;
      }
      IsNextCommandPid = FALSE;

      //
      // Add index to remove it from the command
      //
      IndexesToRemove.push_back(Index);

      continue;
    }
    if (IsNextCommandCoreId) {
      if (!ConvertStringToUInt32(Section, &CoreId)) {
        return FALSE;
      } else {
        //
        // Set the specific core id
        //
        TempEvent->CoreId = CoreId;
      }
      IsNextCommandCoreId = FALSE;

      //
      // Add index to remove it from the command
      //
      IndexesToRemove.push_back(Index);

      continue;
    }

    if (!Section.compare("pid")) {
      IsNextCommandPid = TRUE;

      //
      // Add index to remove it from the command
      //
      IndexesToRemove.push_back(Index);

      continue;
    }
    if (!Section.compare("core")) {
      IsNextCommandCoreId = TRUE;

      //
      // Add index to remove it from the command
      //
      IndexesToRemove.push_back(Index);

      continue;
    }

    if (!Section.compare("buffer")) {
      IsNextCommandBufferSize = TRUE;

      //
      // Add index to remove it from the command
      //
      IndexesToRemove.push_back(Index);

      continue;
    }
  }
  //
  // Additional validation
  //
  if (IsNextCommandCoreId) {
    ShowMessages("error : please specify a value for 'core'\n\n");
    return FALSE;
  }
  if (IsNextCommandPid) {
    ShowMessages("error : please specify a value for 'pid'\n\n");
    return FALSE;
  }
  if (IsNextCommandBufferSize) {
    ShowMessages("errlr : please specify a value for 'buffer'\n\n");
    return FALSE;
  }

  //
  // Fill the address and length of event before release
  //
  *EventDetailsToFill = TempEvent;
  *EventBufferLength = LengthOfEventBuffer;

  //
  // Fill the address and length of action before release
  //
  *ActionDetailsToFill = TempAction;
  *ActionBufferLength = LengthOfActionBuffer;

  //
  // Remove the command that we interpreted above from the command
  //
  for (auto IndexToRemove : IndexesToRemove) {
    NewIndexToRemove++;
    SplittedCommand->erase(SplittedCommand->begin() +
                           (IndexToRemove - NewIndexToRemove));
  }

  //
  // Check if list is initialized or not
  //
  if (!g_EventTraceInitialized) {
    InitializeListHead(&g_EventTrace);
    g_EventTraceInitialized = TRUE;
  }

  //
  // Add the event to the trace list
  //
  InsertHeadList(&g_EventTrace, &(TempEvent->CommandsEventList));

  //
  // Everything is ok, let's return TRUE
  //
  return TRUE;
}
