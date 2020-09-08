/**
 * @file t.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief t command
 * @details
 * @version 0.1
 * @date 2020-09-08
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of t command
 *
 * @return VOID
 */
VOID CommandTHelp() {
  ShowMessages("t : execute each instructions and breaks to the debugger "
               "(step-in).\n\n");
  ShowMessages("syntax : \tt\n");
}

/**
 * @brief t command help
 *
 * @param SplittedCommand
 * @return VOID
 */
VOID CommandT(vector<string> SplittedCommand) {

  BOOL Status;
  ULONG ReturnedLength;
  DEBUGGER_STEPPINGS SteppingsRequest = {0};

  if (SplittedCommand.size() != 1) {
    ShowMessages("incorrect use of 't'\n\n");
    CommandTHelp();
    return;
  }

  if (!g_DeviceHandle) {
    ShowMessages("Handle not found, probably the driver is not loaded. Did you "
                 "use 'load' command?\n");
    return;
  }

  //
  // Set the action and details
  //
  SteppingsRequest.SteppingAction = STEPPINGS_ACTION_STEP_INTO;

  //
  // Send the request to the kernel
  //
  Status = DeviceIoControl(g_DeviceHandle,            // Handle to device
                           IOCTL_DEBUGGER_STEPPINGS,  // IO Control code
                           &SteppingsRequest,         // Input Buffer to driver.
                           SIZEOF_DEBUGGER_STEPPINGS, // Input buffer length
                           &SteppingsRequest, // Output Buffer from driver.
                           SIZEOF_DEBUGGER_STEPPINGS, // Length of output buffer
                                                      // in bytes.
                           &ReturnedLength,           // Bytes placed in buffer.
                           NULL                       // synchronous call
  );

  if (!Status) {
    ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
    return;
  }

  if (SteppingsRequest.KernelStatus == DEBUGEER_OPERATION_WAS_SUCCESSFULL) {

    //
    // Show the current instruction
    //
  } else {
    //
    // There was an error
    //
    ShowErrorMessage(SteppingsRequest.KernelStatus);
  }
}
