/**
 * @file rev-ctrl.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Controller of the reversing machine's module
 * @details
 *
 * @version 0.2
 * @date 2023-03-23
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Request service from the reversing machine
 *
 * @param RevRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
RevRequestService(REVERSING_MACHINE_RECONSTRUCT_MEMORY_REQUEST * RevRequest)
{
    BOOLEAN Status;
    ULONG   ReturnedLength;

    //
    // Check if debugger is loaded or not
    //
    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnFalse);

    //
    // Send the request to the kernel
    //
    Status = DeviceIoControl(
        g_DeviceHandle,                                      // Handle to device
        IOCTL_REQUEST_REV_MACHINE_SERVICE,                   // IO Control
                                                             // code
        RevRequest,                                          // Input Buffer to driver.
        SIZEOF_REVERSING_MACHINE_RECONSTRUCT_MEMORY_REQUEST, // Input buffer length
        RevRequest,                                          // Output Buffer from driver.
        SIZEOF_REVERSING_MACHINE_RECONSTRUCT_MEMORY_REQUEST, // Length of output
                                                             // buffer in bytes.
        &ReturnedLength,                                     // Bytes placed in buffer.
        NULL                                                 // synchronous call
    );

    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        return FALSE;
    }

    //
    // Check if the service request was successful or not
    //
    if (RevRequest->KernelStatus == DEBUGGER_OPERATION_WAS_SUCCESSFUL)
    {
        ShowMessages("the reversing machine service request was successful!\n");
    }
    else
    {
        ShowErrorMessage(RevRequest->KernelStatus);
        return FALSE;
    }

    return FALSE;
}
