/**
 * @file events.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief events commands
 * @details
 * @version 0.1
 * @date 2020-07-24
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern LIST_ENTRY g_EventTrace;
extern BOOLEAN g_EventTraceInitialized;

VOID CommandEventsHelp() {
  ShowMessages("events : show active and disabled events\n");
  ShowMessages("syntax : \tevents [e|d|c] [event number (hex value)]\n");

  ShowMessages("\t\te.g : events \n");
  ShowMessages("\t\te.g : events e 12\n");
  ShowMessages("\t\te.g : events d 10\n");
  ShowMessages("\t\te.g : events c 10\n");
}

VOID CommandEvents(vector<string> SplittedCommand) {

  PDEBUGGER_GENERAL_EVENT_DETAIL CommandDetail;

  if (!g_EventTraceInitialized) {
    ShowMessages("no active/disabled events \n");
    return;
  }

  PLIST_ENTRY TempList = 0;
  TempList = &g_EventTrace;
  while (&g_EventTrace != TempList->Blink) {
    TempList = TempList->Blink;

    CommandDetail = CONTAINING_RECORD(TempList, DEBUGGER_GENERAL_EVENT_DETAIL,
                                      CommandsEventList);

    ShowMessages("%x\t(%s)\t    %s\n", CommandDetail->Tag - 0x1000000,
                 CommandDetail->IsEnabled ? "enabled" : "disabled",
                 CommandDetail->CommandStringBuffer);
  }
}

VOID CommandEventsModifyEvents(UINT64 Tag) {

  BOOLEAN Status;
  ULONG ReturnedLength;
  /*
  //
  // Check if debugger is loaded or not
  //
  if (!g_DeviceHandle) {
    ShowMessages("Handle not found, probably the driver is not loaded.\n");
    return;
  }

  //
  // Send the request to the kernel
  //

  Status = DeviceIoControl(
      g_DeviceHandle, // Handle to device
      IOCTL_DEBUGGER_HIDE_AND_UNHIDE_TO_TRANSPARENT_THE_DEBUGGER, // IO Control
                                                                  // code
      &HideRequest, // Input Buffer to driver.
      SIZEOF_DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE, // Input buffer length
      &HideRequest, // Output Buffer from driver.
      SIZEOF_DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE, // Length of output
                                                          // buffer in bytes.
      &ReturnedLength, // Bytes placed in buffer.
      NULL             // synchronous call
  );

  if (!Status) {
    ShowMessages("Ioctl failed with code 0x%x\n", GetLastError());
    return;
  }

  if (HideRequest.KernelStatus == DEBUGEER_OPERATION_WAS_SUCCESSFULL) {
    ShowMessages("transparent debugging successfully enabled :)\n");

  } else if (HideRequest.KernelStatus ==
             DEBUGEER_ERROR_UNABLE_TO_HIDE_OR_UNHIDE_DEBUGGER) {
    ShowMessages("unable to hide the debugger (transparent-debugging) :(\n");

  } else {
    ShowMessages("unknown error occured :(\n");
  }
  */
}
