/**
 * @file libhyperdbg.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Main interface to connect applications to driver
 * @details
 * @version 0.1
 * @date 2020-04-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

using namespace std;

//
// Global Variables
//
extern HANDLE     g_DeviceHandle;
extern HANDLE     g_IsDriverLoadedSuccessfully;
extern BOOLEAN    g_IsVmxOffProcessStart;
extern PVOID      g_MessageHandler;
extern PVOID      g_MessageHandlerSharedBuffer;
extern TCHAR      g_DriverLocation[MAX_PATH];
extern TCHAR      g_DriverName[MAX_PATH];
extern BOOLEAN    g_UseCustomDriverLocation;
extern LIST_ENTRY g_EventTrace;
extern BOOLEAN    g_LogOpened;
extern BOOLEAN    g_BreakPrintingOutput;
extern BOOLEAN    g_IsConnectedToRemoteDebugger;
extern BOOLEAN    g_OutputSourcesInitialized;
extern BOOLEAN    g_IsSerialConnectedToRemoteDebugger;
extern BOOLEAN    g_IsDebuggerModulesLoaded;
extern BOOLEAN    g_IsReversingMachineModulesLoaded;
extern LIST_ENTRY g_OutputSources;

/**
 * @brief Set the function callback that will be called if any message
 * needs to be shown
 *
 * @param Handler Function that handles the messages
 * @return VOID
 */
VOID
SetTextMessageCallback(PVOID Handler)
{
    g_MessageHandler = Handler;
}

/**
 * @brief Set the function callback that will be called if any message
 * needs to be shown
 *
 * @param Handler Function that handles the messages
 * @return PVOID
 */
PVOID
SetTextMessageCallbackUsingSharedBuffer(PVOID Handler)
{
    g_MessageHandler             = Handler;
    g_MessageHandlerSharedBuffer = malloc(COMMUNICATION_BUFFER_SIZE + TCP_END_OF_BUFFER_CHARS_COUNT);

    if (!g_MessageHandlerSharedBuffer)
    {
        g_MessageHandler = NULL;
        return NULL;
    }

    RtlZeroMemory(g_MessageHandlerSharedBuffer, COMMUNICATION_BUFFER_SIZE + TCP_END_OF_BUFFER_CHARS_COUNT);

    return g_MessageHandlerSharedBuffer;
}

/**
 * @brief Unset the function callback that will be called if any message
 * needs to be shown
 *
 * @return VOID
 */
VOID
UnsetTextMessageCallback()
{
    g_MessageHandler = NULL;
    free(g_MessageHandlerSharedBuffer);
    g_MessageHandlerSharedBuffer = NULL;
}

/**
 * @brief Show messages
 *
 * @param Fmt format string message
 * @param ... arguments
 * @return VOID
 */
VOID
ShowMessages(const char * Fmt, ...)
{
    va_list ArgList;
    va_list Args;
    char    TempMessage[COMMUNICATION_BUFFER_SIZE + TCP_END_OF_BUFFER_CHARS_COUNT] = {0};

    if (g_MessageHandler == NULL && !g_IsConnectedToRemoteDebugger && !g_IsSerialConnectedToRemoteDebugger)
    {
        va_start(Args, Fmt);
        vprintf(Fmt, Args);
        va_end(Args);
        if (!g_LogOpened)
        {
            return;
        }
    }

    va_start(ArgList, Fmt);
    int sprintfresult = vsprintf_s(TempMessage, Fmt, ArgList);
    va_end(ArgList);

    if (sprintfresult != -1)
    {
        if (g_IsConnectedToRemoteDebugger)
        {
            //
            // vsprintf_s and vswprintf_s return the number of characters written,
            // not including the terminating null character, or a negative value
            // if an output error occurs.
            //
            RemoteConnectionSendResultsToHost(TempMessage, sprintfresult);
        }
        else if (g_IsSerialConnectedToRemoteDebugger)
        {
            KdSendUsermodePrints(TempMessage, sprintfresult);
        }

        if (g_LogOpened)
        {
            //
            // .logopen command executed
            //
            LogopenSaveToFile(TempMessage);
        }
        if (g_MessageHandler != NULL)
        {
            //
            // There is another handler
            //
            if (g_MessageHandlerSharedBuffer == NULL)
            {
                ((SendMessageWithParamCallback)g_MessageHandler)(TempMessage);
            }
            else
            {
                memcpy(g_MessageHandlerSharedBuffer, TempMessage, strlen(TempMessage) + 1);
                ((SendMessageWWithSharedBufferCallback)g_MessageHandler)();
            }
        }
    }
}

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
    UINT32                 OperationCode;
    DWORD                  ErrorNum;
    HANDLE                 Handle;

    RegisterEvent.hEvent = NULL;
    RegisterEvent.Type   = IRP_BASED;

    //
    // Create another handle to be used in for reading kernel messages,
    // it is because I noticed that if I use a same handle for IRP Pending
    // and other IOCTLs then if I complete that IOCTL then both of the current
    // IOCTL and the Pending IRP are completed and return to user mode,
    // even if it's odd but that what happens, so this way we can solve it
    // if you know why this problem happens, then contact me !
    //
    Handle = CreateFileA(
        "\\\\.\\HyperDbgDebuggerDevice",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, /// lpSecurityAttirbutes
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
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
    char * OutputBuffer = (char *)malloc(UsermodeBufferSize);

    try
    {
        while (TRUE)
        {
            if (!g_IsVmxOffProcessStart)
            {
                //
                // Clear the buffer
                //
                ZeroMemory(OutputBuffer, UsermodeBufferSize);

                Sleep(DefaultSpeedOfReadingKernelMessages); // we're not trying to eat all of the CPU ;)

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
                OperationCode = 0;
                memcpy(&OperationCode, OutputBuffer, sizeof(UINT32));

                /*
        ShowMessages("Returned Length : 0x%x \n", ReturnedLength);
        ShowMessages("Operation Code : 0x%x \n", OperationCode);
                */

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
            else
            {
                //
                // the thread should not work anymore
                //
                free(OutputBuffer);

                //
                // closeHandle
                //
                if (!CloseHandle(Handle))
                {
                    ShowMessages("err, closing handle 0x%x\n", GetLastError());
                }

                return;
            }
        }
    }
    catch (const std::exception &)
    {
        ShowMessages("err, exception occurred in creating handle or parsing buffer\n");
    }

    free(OutputBuffer);

    //
    // closeHandle
    //
    if (!CloseHandle(Handle))
    {
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
IrpBasedBufferThread(void * data)
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
 * @brief Install VMM driver
 *
 * @return INT return zero if it was successful or non-zero if there
 * was error
 */
INT
HyperDbgInstallVmmDriver()
{
    //
    // The driver is not started yet so let us the install driver
    // First setup full path to driver name
    //

    //
    // If the user has not specified a custom driver location, then we
    // need to find the driver in the same directory as the executable
    //
    if (!g_UseCustomDriverLocation)
    {
        if (!SetupPathForFileName(KERNEL_DEBUGGER_DRIVER_NAME_AND_EXTENSION, g_DriverLocation, sizeof(g_DriverLocation), TRUE))
        {
            return 1;
        }

        //
        // Use default driver name
        //
        strcpy_s(g_DriverName, KERNEL_DEBUGGER_DRIVER_NAME);
    }

    if (!ManageDriver(g_DriverName, g_DriverLocation, DRIVER_FUNC_INSTALL))
    {
        ShowMessages("unable to install VMM driver\n");

        //
        // Error - remove driver
        //
        ManageDriver(g_DriverName, g_DriverLocation, DRIVER_FUNC_REMOVE);

        return 1;
    }

    return 0;
}

/**
 * @brief Stop the driver
 *
 * @return int return zero if it was successful or non-zero if there
 * was error
 */
int
HyperDbgStopDriver(LPCTSTR DriverName)
{
    //
    // Unload the driver if loaded
    //
    if (g_DriverLocation[0] != (TCHAR)0 && ManageDriver(DriverName, g_DriverLocation, DRIVER_FUNC_STOP))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

/**
 * @brief Stop VMM driver
 *
 * @return INT return zero if it was successful or non-zero if there
 * was error
 */
INT
HyperDbgStopVmmDriver()
{
    return HyperDbgStopDriver(g_DriverName);
}

/**
 * @brief Remove the driver
 *
 * @return int return zero if it was successful or non-zero if there
 * was error
 */
int
HyperDbgUninstallDriver(LPCTSTR DriverName)
{
    //
    // Unload the driver if loaded.  Ignore any errors
    //
    if (g_DriverLocation[0] != (TCHAR)0 && ManageDriver(DriverName, g_DriverLocation, DRIVER_FUNC_REMOVE))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

/**
 * @brief Remove the VMM driver
 *
 * @return INT return zero if it was successful or non-zero if there
 * was error
 */
INT
HyperDbgUninstallVmmDriver()
{
    return HyperDbgUninstallDriver(g_DriverName);
}

/**
 * @brief Create handle from VMM module
 *
 * @return INT return zero if it was successful or non-zero if there
 * was error
 */
INT
HyperDbgCreateHandleFromVmmModule()
{
    DWORD ErrorNum;
    DWORD ThreadId;

    if (g_DeviceHandle)
    {
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
        "\\\\.\\HyperDbgDebuggerDevice",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, /// lpSecurityAttirbutes
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        NULL); /// lpTemplateFile

    if (g_DeviceHandle == INVALID_HANDLE_VALUE)
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
            ShowMessages("err, CreateFile failed (%x)\n", ErrorNum);
        }

        g_DeviceHandle = NULL;
        return 1;
    }

    //
    // Initialize the list of events
    //
    InitializeListHead(&g_EventTrace);

#if !UseDbgPrintInsteadOfUsermodeMessageTracking
    HANDLE Thread = CreateThread(NULL, 0, IrpBasedBufferThread, NULL, 0, &ThreadId);

    // if (Thread)
    // {
    // ShowMessages("thread Created successfully\n");
    // }

#endif

    return 0;
}

/**
 * @brief Unload VMM driver
 *
 * @return INT return zero if it was successful or non-zero if there
 * was error
 */
INT
HyperDbgUnloadVmm()
{
    BOOL Status;

    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnOne);

    ShowMessages("start terminating...\n");

    //
    // Uninitialize the user debugger if it's initialized
    //
    UdUninitializeUserDebugger();

    //
    // Send IOCTL to mark complete all IRP Pending
    //
    Status = DeviceIoControl(g_DeviceHandle,      // Handle to device
                             IOCTL_TERMINATE_VMX, // IO Control Code (IOCTL)
                             NULL,                // Input Buffer to driver.
                             0,                   // Length of input buffer in bytes. (x 2 is bcuz
                                                  // as the driver is x64 and has 64 bit values)
                             NULL,                // Output Buffer from driver.
                             0,                   // Length of output buffer in bytes.
                             NULL,                // Bytes placed in buffer.
                             NULL                 // synchronous call
    );

    //
    // wait to make sure we don't use an invalid handle in another Ioctl
    //
    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        return 1;
    }

    //
    // Send IOCTL to mark complete all IRP Pending
    //
    Status = DeviceIoControl(
        g_DeviceHandle,                                      // Handle to device
        IOCTL_RETURN_IRP_PENDING_PACKETS_AND_DISALLOW_IOCTL, // IO
                                                             // Control
                                                             // code
        NULL,                                                // Input Buffer to driver.
        0,                                                   // Length of input buffer in bytes. (x 2 is bcuz as the
                                                             // driver is x64 and has 64 bit values)
        NULL,                                                // Output Buffer from driver.
        0,                                                   // Length of output buffer in bytes.
        NULL,                                                // Bytes placed in buffer.
        NULL                                                 // synchronous call
    );

    //
    // wait to make sure we don't use an invalid handle in another Ioctl
    //
    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        return 1;
    }

    //
    // Indicate that the finish process start or not
    //
    g_IsVmxOffProcessStart = TRUE;

    Sleep(1000); // Wait so next thread can return from IRP Pending

    //
    // Send IRP_MJ_CLOSE to driver to terminate Vmxs
    //
    if (!CloseHandle(g_DeviceHandle))
    {
        ShowMessages("err, closing handle 0x%x\n", GetLastError());
        return 1;
    };

    //
    // Null the handle to indicate that the driver's device is not ready
    // to use
    //
    g_DeviceHandle = NULL;

    //
    // Debugger module is not loaded anymore
    //
    g_IsDebuggerModulesLoaded = FALSE;

    //
    // Check if we found an already built symbol table
    //
    SymbolDeleteSymTable();

    ShowMessages("you're not on HyperDbg's hypervisor anymore!\n");

    return 0;
}

/**
 * @brief load vmm module
 *
 * @return int return zero if it was successful or non-zero if there
 */
INT
HyperDbgLoadVmmModule()
{
    BOOL   Status;
    HANDLE hToken;
    char   CpuId[13] = {0};

    //
    // Enable Debug privilege
    //
    Status = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);
    if (!Status)
    {
        ShowMessages("err, OpenProcessToken failed (%x)\n", GetLastError());
        return 1;
    }

    Status = SetPrivilege(hToken, SE_DEBUG_NAME, TRUE);
    if (!Status)
    {
        CloseHandle(hToken);
        return 1;
    }

    //
    // Read the vendor string
    //
    CpuReadVendorString(CpuId);

    ShowMessages("current processor vendor is : %s\n", CpuId);

    if (strcmp(CpuId, "GenuineIntel") == 0)
    {
        ShowMessages("virtualization technology is vt-x\n");
    }
    else
    {
        ShowMessages("this program is not designed to run in a non-VT-x "
                     "environment !\n");
        return 1;
    }

    if (VmxSupportDetection())
    {
        ShowMessages("vmx operation is supported by your processor\n");
    }
    else
    {
        ShowMessages("vmx operation is not supported by your processor\n");
        return 1;
    }

    //
    // Create event to show if the hypervisor is loaded or not
    //
    g_IsDriverLoadedSuccessfully = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (HyperDbgCreateHandleFromVmmModule() == 1)
    {
        //
        // No need to handle anymore
        //
        CloseHandle(g_IsDriverLoadedSuccessfully);
        return 1;
    }

    //
    // Vmm module (Hypervisor) is loaded
    //

    //
    // We wait for the first message from the kernel debugger to continue
    //
    WaitForSingleObject(
        g_IsDriverLoadedSuccessfully,
        INFINITE);

    //
    // No need to handle anymore
    //
    CloseHandle(g_IsDriverLoadedSuccessfully);

    //
    // If we reach here so the module are loaded
    //
    g_IsDebuggerModulesLoaded = TRUE;

    ShowMessages("vmm module is running...\n");

    return 0;
}
