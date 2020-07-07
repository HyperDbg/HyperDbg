/**
 * @file hide.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief !hide command
 * @details
 * @version 0.1
 * @date 2020-07-07
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

VOID CommandHideHelp() {
  ShowMessages("!hide : tries to make HyperDbg transparent from anti-debugging "
               "and anti-hypervisor methods.\n\n");
  ShowMessages("syntax : \t!hide\n");
  ShowMessages("\t\te.g : !hide\n");
}

VOID CommandHide(vector<string> SplittedCommand) {

  BOOLEAN Status;
  ULONG ReturnedLength;
  DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE HideRequest = {0};

  if (SplittedCommand.size() >= 2) {
    ShowMessages("incorrect use of '!hide'\n\n");
    CommandHideHelp();
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
  // We wanna hide the debugger and make transparent vm-exits
  //
  HideRequest.IsHide = TRUE;

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
}
