/**
 * @file hprdbgrmctrl.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Controller of the reversing machine's module
 * @details
 *
 * @version 0.2
 * @date 2023-02-01
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Holds the global handle of device which is used
 * to send the request to the kernel by IOCTL, this
 * handle is not used for IRP Pending of message tracing
 * this handle used in reversing machine
 *
 */
HANDLE g_DeviceHandleReversingMachine;

/**
 * @brief Handle to show that if the debugger is loaded successfully
 *
 */
HANDLE g_IsDriverLoadedSuccessfully = NULL;

/**
 * @brief Holds the global handle of device which is used
 * to send the request to the kernel by IOCTL, this
 * handle is not used for IRP Pending of message tracing
 * this handle is used in KD VMM
 *
 */
HANDLE g_DeviceHandle;

/**
 * @brief Checks whether the RM module is loaded or not
 *
 */
BOOLEAN g_IsReversingMachineModulesLoaded;

/**
 * @brief Shows whether the vmxoff process start or not
 *
 */
BOOLEAN g_IsVmxOffProcessStart;

#ifdef __cplusplus
extern "C" {
#endif

__declspec(dllexport) int ReversingMachineStart();
__declspec(dllexport) int ReversingMachineStop();

#ifdef __cplusplus
}
#endif

/**
 * @brief Show messages
 *
 * @param Fmt format string message
 */
VOID ShowMessages(const char* Fmt, ...)
{
    va_list ArgList;

    va_start(ArgList, Fmt);
    vprintf(Fmt, ArgList);
    va_end(ArgList);
}

/**
 * @brief Read kernel buffers using IRP Pending
 *
 * @param Device Driver handle
 */
void ReadIrpBasedBuffer()
{
    BOOL Status;
    ULONG ReturnedLength;
    REGISTER_NOTIFY_BUFFER RegisterEvent;
    UINT32 OperationCode;
    DWORD ErrorNum;
    HANDLE Handle;
    BOOLEAN OutputSourceFound;
    PLIST_ENTRY TempList;

    RegisterEvent.hEvent = NULL;
    RegisterEvent.Type = IRP_BASED;

    //
    // Create another handle to be used in for reading kernel messages,
    // it is because I noticed that if I use a same handle for IRP Pending
    // and other IOCTLs then if I complete that IOCTL then both of the current
    // IOCTL and the Pending IRP are completed and return to user mode,
    // even if it's odd but that what happens, so this way we can solve it
    // if you know why this problem happens, then contact me !
    //
    Handle = CreateFileA(
        "\\\\.\\HyperDbgReversingMachineDevice",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, /// lpSecurityAttirbutes
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        NULL); /// lpTemplateFile

    if (Handle == INVALID_HANDLE_VALUE) {
        ErrorNum = GetLastError();
        if (ErrorNum == ERROR_ACCESS_DENIED) {
            ShowMessages("err, access denied\nare you sure you have administrator "
                         "rights?\n");
        } else if (ErrorNum == ERROR_GEN_FAILURE) {
            ShowMessages("err, a device attached to the system is not functioning\n"
                         "vmx feature might be disabled from BIOS or VBS/HVCI is active\n");
        } else {
            ShowMessages("err, CreateFile failed with (%x)\n", ErrorNum);
        }

        g_DeviceHandle = NULL;
        Handle = NULL;

        return;
    }

    //
    // allocate buffer for transfering messages
    //
    char* OutputBuffer = (char*)malloc(UsermodeBufferSize);

    try {
        while (TRUE) {

            //
            // Clear the buffer
            //
            ZeroMemory(OutputBuffer, UsermodeBufferSize);

            Sleep(DefaultSpeedOfReadingKernelMessages); // we're not trying to eat all of the CPU ;)

            Status = DeviceIoControl(
                Handle, // Handle to device
                IOCTL_REGISTER_EVENT, // IO Control code
                &RegisterEvent, // Input Buffer to driver.
                SIZEOF_REGISTER_EVENT * 2, // Length of input buffer in bytes. (x 2 is bcuz as the
                                           // driver is x64 and has 64 bit values)
                OutputBuffer, // Output Buffer from driver.
                UsermodeBufferSize, // Length of output buffer in bytes.
                &ReturnedLength, // Bytes placed in buffer.
                NULL // synchronous call
            );

            if (!Status) {

                //
                // Error occured for second time, and we show the error message
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
            OperationCode = 0;
            memcpy(&OperationCode, OutputBuffer, sizeof(UINT32));

            switch (OperationCode) {
            case OPERATION_LOG_NON_IMMEDIATE_MESSAGE:

                ShowMessages("%s", OutputBuffer + sizeof(UINT32));

                break;
            case OPERATION_LOG_INFO_MESSAGE:

                ShowMessages("%s", OutputBuffer + sizeof(UINT32));

                break;
            case OPERATION_LOG_ERROR_MESSAGE:

                ShowMessages("%s", OutputBuffer + sizeof(UINT32));

                break;
            case OPERATION_LOG_WARNING_MESSAGE:

                ShowMessages("%s", OutputBuffer + sizeof(UINT32));

                break;

            default:

                ShowMessages("err, unknown message is received in rev module");

                break;
            }
        }
    } catch (const std::exception&) {
        ShowMessages(" Exception !\n");
    }
    free(OutputBuffer);

    //
    // closeHandle
    //
    if (!CloseHandle(Handle)) {
        ShowMessages("err, closing handle 0x%x\n", GetLastError());
    };
}

/**
 * @brief Create a thread for pending buffers
 *
 * @param Data
 * @return DWORD Device Handle
 */
DWORD WINAPI
IrpBasedBufferThread(void* data)
{
    //
    // Do stuff.  This will be the first function called on the new
    // thread. When this function returns, the thread goes away.  See
    // MSDN for more details. Test Irp Based Notifications
    //
    ReadIrpBasedBuffer();

    return 0;
}

/**
 * @brief Unload the reversing machine driver
 *
 * @return int return zero if it was successful or non-zero if there
 * was error
 */
int ReversingMachineStop()
{
    //
    // Not implemented
    //
    return 0;
}

/**
 * @brief Load the reversing machine driver
 *
 * @return int return zero if it was successful or non-zero if there
 * was error
 */
int ReversingMachineStart()
{
    DWORD ErrorNum;
    DWORD ThreadId;

    if (g_DeviceHandle) {
        ShowMessages("handle of the driver found, if you use 'load' before, please "
                     "unload it using 'unload'\n");
        return 1;
    }

    //
    // Make sure that this variable is false, because it might be set to
    // true as the result of a previous load
    //
    g_IsVmxOffProcessStart = FALSE;

    //
    // Init entering vmx
    //
    g_DeviceHandle = CreateFileA(
        "\\\\.\\HyperDbgReversingMachineDevice",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, /// lpSecurityAttirbutes
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        NULL); /// lpTemplateFile

    if (g_DeviceHandle == INVALID_HANDLE_VALUE) {
        ErrorNum = GetLastError();
        if (ErrorNum == ERROR_ACCESS_DENIED) {
            ShowMessages("err, access denied\nare you sure you have administrator "
                         "rights?\n");
        } else if (ErrorNum == ERROR_GEN_FAILURE) {
            ShowMessages("err, a device attached to the system is not functioning\n"
                         "vmx feature might be disabled from BIOS or VBS/HVCI is active\n");
        } else {
            ShowMessages("err, CreateFile failed (%x)\n", ErrorNum);
        }

        g_DeviceHandle = NULL;
        return 1;
    }

    HANDLE Thread = CreateThread(NULL, 0, IrpBasedBufferThread, NULL, 0, &ThreadId);

    // if (Thread)
    // {
    // ShowMessages("thread Created successfully\n");
    // }

    //
    // Operation was successful
    //
    return 0;
}
