/**
 * @file print.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief print command
 * @details
 * @version 0.1
 * @date 2020-10-08
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of print command
 *
 * @return VOID
 */
VOID CommandPrintHelp() {
  ShowMessages("print : evaluate expressions.\n\n");
  ShowMessages("syntax : \tprint [expression]\n");
  ShowMessages("\t\te.g : print dq(poi(@rcx))\n");
}

/**
 * @brief handler of print command
 *
 * @param SplittedCommand
 * @return VOID
 */
VOID CommandPrint(vector<string> SplittedCommand, string Expr) {

  BOOL Status;
  ULONG ReturnedLength;
  DEBUGGER_PRINT PrintRequest = {0};

  if (SplittedCommand.size() == 1) {
    ShowMessages("incorrect use of 'print'\n\n");
    CommandPrintHelp();
    return;
  }

  if (!g_DeviceHandle) {
    ShowMessages("Handle not found, probably the driver is not loaded. Did you "
                 "use 'load' command?\n");
    return;
  }

  //
  // Trim the command
  //
  Trim(Expr);

  //
  // Remove print from it
  //
  Expr.erase(0, 5);

  //
  // Trim it again
  //
  Trim(Expr);

  printf("Expression : %s \n", Expr.c_str());

  //
  // Send the request to the kernel
  //
  Status = DeviceIoControl(g_DeviceHandle,        // Handle to device
                           IOCTL_DEBUGGER_PRINT,  // IO Control coder
                           &PrintRequest,         // Input Buffer to driver.
                           SIZEOF_DEBUGGER_PRINT, // Input buffer length
                           &PrintRequest,         // Output Buffer from driver.
                           SIZEOF_DEBUGGER_PRINT, // Length of output buffer
                                                  // in bytes.
                           &ReturnedLength,       // Bytes placed in buffer.
                           NULL                   // synchronous call
  );

  if (!Status) {
    ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
    return;
  }
}
