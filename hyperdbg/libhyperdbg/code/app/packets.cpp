/**
 * @file packets.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Functions for handling packets from the driver
 * @details
 * @version 0.19
 * @date 2026-05-28
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

using namespace std;

//
// Global Variables
//
extern HANDLE  g_DeviceHandle;
extern HANDLE  g_IsDriverLoadedSuccessfully;
extern BOOLEAN g_IsVmxOffProcessStart;
extern BOOLEAN g_BreakPrintingOutput;
extern BOOLEAN g_OutputSourcesInitialized;

/**
 * @brief Read kernel buffers using IRP Pending
 *
 * @param Device Driver handle
 * @return VOID
 */
VOID
ReadIrpBasedBuffer()
{
    BOOL                   Status;
    ULONG                  ReturnedLength;
    REGISTER_NOTIFY_BUFFER RegisterEvent;
    DWORD                  ErrorNum;
    HANDLE                 Handle;
    UINT32                 OperationCode;

    RegisterEvent.hEvent = NULL;
    RegisterEvent.Type   = IRP_BASED;

    //
    // Keep the packet reader on a dedicated synchronous handle. It blocks on
    // a pending IOCTL while the main debugger handle continues sending other
    // synchronous IOCTLs.
    //
    Handle = CreateFileA(
        "\\\\.\\HyperDbgDebuggerDevice",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, /// lpSecurityAttirbutes
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL); /// lpTemplateFile

    if (Handle == INVALID_HANDLE_VALUE)
    {
        ErrorNum = GetLastError();

        if (ErrorNum == ERROR_ACCESS_DENIED)
        {
            ShowMessages("err, access denied\nare you sure you have administrator "
                         "rights?\n");
        }
        else if (ErrorNum == ERROR_GEN_FAILURE)
        {
            ShowMessages("err, a device attached to the system is not functioning\n"
                         "vmx feature might be disabled from BIOS or VBS/HVCI is active\n");
        }
        else
        {
            ShowMessages("err, CreateFile failed with (%x)\n", ErrorNum);
        }

        g_DeviceHandle = NULL;
        Handle         = NULL;

        return;
    }

    //
    // allocate buffer for transferring messages
    //
    CHAR * OutputBuffer = (CHAR *)malloc(UsermodeBufferSize);

    try
    {
        while (!g_IsVmxOffProcessStart)
        {
            //
            // Clear the buffer
            //
            ZeroMemory(OutputBuffer, UsermodeBufferSize);

            Status = DeviceIoControl(
                Handle,                    // Handle to device
                IOCTL_REGISTER_EVENT,      // IO Control Code (IOCTL)
                &RegisterEvent,            // Input Buffer to driver.
                SIZEOF_REGISTER_EVENT * 2, // Length of input buffer in bytes. (x 2 is bcuz as the
                                           // driver is x64 and has 64 bit values)
                OutputBuffer,              // Output Buffer from driver.
                UsermodeBufferSize,        // Length of output buffer in bytes.
                &ReturnedLength,           // Bytes placed in buffer.
                NULL                       // synchronous call
            );

            if (!Status)
            {
                //
                // Error occurred for second time, and we show the error message
                //
                // ShowMessages("ioctl failed with code 0x%x\n", GetLastError());

                //
                // if we reach here, the packet is probably failed, it might
                // be because of using flush command
                //
                continue;
            }

            //
            // Compute the received buffer's operation code
            //
            memcpy(&OperationCode, OutputBuffer, sizeof(UINT32));

            // ShowMessages("Returned Length : 0x%x \n", ReturnedLength);
            // ShowMessages("Operation Code : 0x%x \n", OperationCode);

            //
            // Check if the operation code contains mandatory debuggee bit
            // If that's the case, we shouldn't wait (sleep) for new messages
            //
            if ((OperationCode & OPERATION_MANDATORY_DEBUGGEE_BIT) == 0)
            {
                Sleep(DefaultSpeedOfReadingKernelMessages); // we're not trying to eat all of the CPU ;)
            }

            switch (OperationCode)
            {
            case OPERATION_LOG_NON_IMMEDIATE_MESSAGE:

                if (g_BreakPrintingOutput)
                {
                    //
                    // means that the user asserts a CTRL+C or CTRL+BREAK Signal
                    // we shouldn't show or save anything in this case
                    //
                    continue;
                }

                ShowMessages("%s", OutputBuffer + sizeof(UINT32));

                break;

            case OPERATION_LOG_MESSAGE_MANDATORY:

                ShowMessages("%s", OutputBuffer + sizeof(UINT32));

                break;

            case OPERATION_LOG_INFO_MESSAGE:

                if (g_BreakPrintingOutput)
                {
                    //
                    // means that the user asserts a CTRL+C or CTRL+BREAK Signal
                    // we shouldn't show or save anything in this case
                    //
                    continue;
                }

                ShowMessages("%s", OutputBuffer + sizeof(UINT32));

                break;

            case OPERATION_LOG_ERROR_MESSAGE:
                if (g_BreakPrintingOutput)
                {
                    //
                    // means that the user asserts a CTRL+C or CTRL+BREAK Signal
                    // we shouldn't show or save anything in this case
                    //
                    continue;
                }

                ShowMessages("%s", OutputBuffer + sizeof(UINT32));

                break;

            case OPERATION_LOG_WARNING_MESSAGE:

                if (g_BreakPrintingOutput)
                {
                    //
                    // means that the user asserts a CTRL+C or CTRL+BREAK Signal
                    // we shouldn't show or save anything in this case
                    //
                    continue;
                }

                ShowMessages("%s", OutputBuffer + sizeof(UINT32));

                break;

            case OPERATION_COMMAND_FROM_DEBUGGER_CLOSE_AND_UNLOAD_VMM:

                KdCloseConnection();

                break;

            case OPERATION_DEBUGGEE_USER_INPUT:

                KdHandleUserInputInDebuggee((DEBUGGEE_USER_INPUT_PACKET *)(OutputBuffer + sizeof(UINT32)));

                break;

            case OPERATION_DEBUGGEE_REGISTER_EVENT:

                KdRegisterEventInDebuggee(
                    (PDEBUGGER_GENERAL_EVENT_DETAIL)(OutputBuffer + sizeof(UINT32)),
                    ReturnedLength);

                break;

            case OPERATION_DEBUGGEE_ADD_ACTION_TO_EVENT:

                KdAddActionToEventInDebuggee(
                    (PDEBUGGER_GENERAL_ACTION)(OutputBuffer + sizeof(UINT32)),
                    ReturnedLength);

                break;

            case OPERATION_DEBUGGEE_CLEAR_EVENTS:

                KdSendModifyEventInDebuggee(
                    (PDEBUGGER_MODIFY_EVENTS)(OutputBuffer + sizeof(UINT32)),
                    TRUE);

                break;

            case OPERATION_DEBUGGEE_CLEAR_EVENTS_WITHOUT_NOTIFYING_DEBUGGER:

                KdSendModifyEventInDebuggee(
                    (PDEBUGGER_MODIFY_EVENTS)(OutputBuffer + sizeof(UINT32)),
                    FALSE);

                break;

            case OPERATION_HYPERVISOR_DRIVER_IS_SUCCESSFULLY_LOADED:

                //
                // Indicate that driver (Hypervisor) is loaded successfully
                //
                SetEvent(g_IsDriverLoadedSuccessfully);

                break;

            case OPERATION_HYPERVISOR_DRIVER_END_OF_IRPS:

                //
                // End of receiving messages (IRPs), nothing to do
                // it will just end the thread at next round because of the check of
                // g_IsVmxOffProcessStart at the beginning of the loop
                //
                break;

            case OPERATION_COMMAND_FROM_DEBUGGER_RELOAD_SYMBOL:

                //
                // Pause debugger after getting the results
                //
                KdReloadSymbolsInDebuggee(TRUE,
                                          ((PDEBUGGEE_SYMBOL_REQUEST_PACKET)(OutputBuffer + sizeof(UINT32)))->ProcessId);

                break;

            case OPERATION_NOTIFICATION_FROM_USER_DEBUGGER_PAUSE:

                //
                // handle pausing packet from user debugger
                //
                UdHandleUserDebuggerPausing(
                    (PDEBUGGEE_UD_PAUSED_PACKET)(OutputBuffer + sizeof(UINT32)));

                break;

            default:

                //
                // Check if there are available output sources
                //
                if (!g_OutputSourcesInitialized || !ForwardingCheckAndPerformEventForwarding(OperationCode,
                                                                                             OutputBuffer + sizeof(UINT32),
                                                                                             ReturnedLength - sizeof(UINT32) - 1))
                {
                    if (g_BreakPrintingOutput)
                    {
                        //
                        // means that the user asserts a CTRL+C or CTRL+BREAK Signal
                        // we shouldn't show or save anything in this case
                        //
                        continue;
                    }

                    ShowMessages("%s", OutputBuffer + sizeof(UINT32));
                }

                break;
            }
        }
    }
    catch (const std::exception &)
    {
        ShowMessages("err, exception occurred in creating handle or parsing buffer\n");
    }

    free(OutputBuffer);

    //
    // close handle
    //
    if (!CloseHandle(Handle))
    {
        ShowMessages("err, closing handle 0x%x\n", GetLastError());
    }
}

/**
 * @brief Create a thread for pending buffers
 *
 * @param Data
 * @return DWORD Device Handle
 */
DWORD WINAPI
IrpBasedBufferThread(PVOID Data)
{
    //
    // Do stuff.  This will be the first function called on the new
    // thread. When this function returns, the thread goes away.  See
    // MSDN for more details. Test Irp Based Notifications
    //
    ReadIrpBasedBuffer();

    return 0;
}
