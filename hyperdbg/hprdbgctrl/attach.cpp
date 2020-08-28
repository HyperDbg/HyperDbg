/**
 * @file attach.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief .attach command
 * @details
 * @version 0.1
 * @date 2020-08-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern BOOLEAN g_IsAttachedToUsermodeProcess;

/**
 * @brief help of .attach command
 *
 * @return VOID
 */
VOID CommandAttachHelp() {
  ShowMessages(".attach : attach to debug a user-mode process.\n\n");
  ShowMessages("syntax : \t.attach pid [process id (hex)]\n");
  ShowMessages("\t\te.g : .attach pid b60 \n");
}

/**
 * @brief .attach command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID CommandAttach(vector<string> SplittedCommand) {

  BOOLEAN Status;
  ULONG ReturnedLength;
  UINT64 TargetPid;
  DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS AttachRequest = {0};

  if (SplittedCommand.size() <= 2) {
    ShowMessages("incorrect use of '.attach'\n\n");
    CommandAttachHelp();
    return;
  }

  //
  // Find out whether the user enters pid or not
  //
  if (!SplittedCommand.at(1).compare("pid")) {

    //
    // Check for the user to not add extra arguments
    //
    if (SplittedCommand.size() != 3) {
      ShowMessages("incorrect use of '.attach'\n\n");
      CommandAttachHelp();
      return;
    }

    //
    // It's just a pid for the process
    //
    if (!ConvertStringToUInt64(SplittedCommand.at(2), &TargetPid)) {
      ShowMessages("incorrect process id\n\n");
      return;
    }

  } else {

    //
    // Invalid argument for the second parameter to the command
    //
    ShowMessages("incorrect use of '.attach'\n\n");
    CommandAttachHelp();
    return;
  }

  //
  // Check if debugger is loaded or not
  //
  if (!g_DeviceHandle) {
    ShowMessages("Handle not found, probably the driver is not loaded. Did you "
                 "use 'load' command?\n");
    return;
  }

  //
  // We wanna attach to a remote process
  //
  AttachRequest.IsAttach = TRUE;

  //
  // Set the process id
  //
  AttachRequest.ProcessId = TargetPid;

  //
  // Send the request to the kernel
  //

  Status = DeviceIoControl(
      g_DeviceHandle,                                 // Handle to device
      IOCTL_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS, // IO Control
                                                      // code
      &AttachRequest,                                 // Input Buffer to driver.
      SIZEOF_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS, // Input buffer length
      &AttachRequest, // Output Buffer from driver.
      SIZEOF_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS, // Length of output
                                                       // buffer in bytes.
      &ReturnedLength, // Bytes placed in buffer.
      NULL             // synchronous call
  );

  if (!Status) {
    ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
    return;
  }

  //
  // Check if attaching was successful then we can set the attached to true
  //
  if (AttachRequest.Result == DEBUGEER_OPERATION_WAS_SUCCESSFULL) {
    g_IsAttachedToUsermodeProcess = TRUE;
  }
}
