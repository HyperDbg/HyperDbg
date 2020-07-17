/**
 * @file unhide.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief !unhide command
 * @details
 * @version 0.1
 * @date 2020-07-07
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

VOID CommandUnhideHelp() {
  ShowMessages("!unhide : Reveals the debugger to the applications.\n\n");
  ShowMessages("syntax : \t!unhide\n");
  ShowMessages("\t\te.g : !unhide\n");
}

VOID CommandUnhide(vector<string> SplittedCommand) {

  BOOLEAN Status;
  ULONG ReturnedLength;
  DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE UnhideRequest = {0};

  if (SplittedCommand.size() >= 2) {
    ShowMessages("incorrect use of '!unhide'\n\n");
    CommandUnhideHelp();
    return;
  }

  //
  // Check if debugger is loaded or not
  //
  if (!g_DeviceHandle) {
    ShowMessages("Handle not found, probably the driver is not loaded.\n");
    return;
  }

  //
  // We don't wanna hide the debugger and make transparent vm-exits
  //
  UnhideRequest.IsHide = FALSE;

  //
  // Send the request to the kernel
  //

  Status = DeviceIoControl(
      g_DeviceHandle, // Handle to device
      IOCTL_DEBUGGER_HIDE_AND_UNHIDE_TO_TRANSPARENT_THE_DEBUGGER, // IO Control
                                                                  // code
      &UnhideRequest, // Input Buffer to driver.
      SIZEOF_DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE, // Input buffer length
      &UnhideRequest, // Output Buffer from driver.
      SIZEOF_DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE, // Length of output
                                                          // buffer in bytes.
      &ReturnedLength, // Bytes placed in buffer.
      NULL             // synchronous call
  );

  if (!Status) {
    ShowMessages("Ioctl failed with code 0x%x\n", GetLastError());
    return;
  }

  if (UnhideRequest.KernelStatus == DEBUGEER_OPERATION_WAS_SUCCESSFULL) {
    ShowMessages("transparent debugging successfully disabled :)\n");

  } else if (UnhideRequest.KernelStatus ==
             DEBUGEER_ERROR_UNABLE_TO_HIDE_OR_UNHIDE_DEBUGGER) {
    ShowMessages("unable to unhide the debugger (transparent-debugging) :(\n");

  } else {
    ShowMessages("unknown error occured :(\n");
  }
}
