/**
 * @file hprdbgctrl.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
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
extern Callback   g_MessageHandler;
extern TCHAR      g_DriverLocation[MAX_PATH];
extern LIST_ENTRY g_EventTrace;
extern BOOLEAN    g_EventTraceInitialized;
extern BOOLEAN    g_LogOpened;
extern BOOLEAN    g_BreakPrintingOutput;
extern BOOLEAN    g_IsConnectedToRemoteDebugger;
extern BOOLEAN    g_OutputSourcesInitialized;
extern BOOLEAN    g_IsSerialConnectedToRemoteDebugger;
extern LIST_ENTRY g_OutputSources;

/**
 * @brief Set the function callback that will be called if anything received
 * from the kernel
 *
 * @param handler Function that handles the messages
 */
void
HyperdbgSetTextMessageCallback(Callback handler)
{
    g_MessageHandler = handler;
}

/**
 * @brief Show messages received from kernel driver
 *
 * @param Fmt format string message
 */
void
ShowMessages(const char * Fmt, ...)
{
    va_list ArgList;
    va_list Args;
    char    TempMessage[PacketChunkSize] = {0};

    if (g_MessageHandler == NULL && !g_IsConnectedToRemoteDebugger &&
        !g_IsSerialConnectedToRemoteDebugger)
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
    int sprintfresult = vsprintf(TempMessage, Fmt, ArgList);
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
            g_MessageHandler(TempMessage);
        }
    }
}

#if !UseDbgPrintInsteadOfUsermodeMessageTracking

/**
 * @brief Read kernel buffers using IRP Pending
 *
 * @param Device Driver handle
 */
void
ReadIrpBasedBuffer()
{
    BOOL                   Status;
    ULONG                  ReturnedLength;
    REGISTER_NOTIFY_BUFFER RegisterEvent;
    UINT32                 OperationCode;
    DWORD                  ErrorNum;
    HANDLE                 Handle;
    BOOLEAN                OutputSourceFound;
    PLIST_ENTRY            TempList;

    /*
  ShowMessages(" =============================== Kernel-Mode Logs (Driver) "
               "===============================\n");
               */

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
        "\\\\.\\HyperdbgHypervisorDevice",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, /// lpSecurityAttirbutes
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        NULL); /// lpTemplateFile

    if (Handle == INVALID_HANDLE_VALUE)
    {
        ErrorNum = GetLastError();
        if (ErrorNum == 5)
        {
            ShowMessages("Error: Access denied! Are you sure you have administrator "
                         "rights?\n");
        }
        else
        {
            ShowMessages("CreateFile failed with error: 0x%x\n", ErrorNum);
        }

        g_DeviceHandle = NULL;
        return;
    }

    //
    // allocate buffer for transfering messages
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

                Sleep(200); // we're not trying to eat all of the CPU ;)

                Status = DeviceIoControl(
                    Handle,               // Handle to device
                    IOCTL_REGISTER_EVENT, // IO Control code
                    &RegisterEvent,       // Input Buffer to driver.
                    SIZEOF_REGISTER_EVENT *
                        2,              // Length of input buffer in bytes. (x 2 is bcuz as the
                                        // driver is x64 and has 64 bit values)
                    OutputBuffer,       // Output Buffer from driver.
                    UsermodeBufferSize, // Length of output buffer in bytes.
                    &ReturnedLength,    // Bytes placed in buffer.
                    NULL                // synchronous call
                );

                if (!Status)
                {
                    ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
                    break;
                }

                if (g_BreakPrintingOutput)
                {
                    //
                    // means that the user asserts a CTRL+C or CTRL+BREAK Signal
                    // we shouldn't show or save anything in this case
                    //
                    continue;
                }

                /*
        ShowMessages("========================= Kernel Mode (Buffer) "
                     "=========================\n");
                     */

                OperationCode = 0;
                memcpy(&OperationCode, OutputBuffer, sizeof(UINT32));

                /*
        ShowMessages("Returned Length : 0x%x \n", ReturnedLength);
        ShowMessages("Operation Code : 0x%x \n", OperationCode);
        */

                switch (OperationCode)
                {
                case OPERATION_LOG_NON_IMMEDIATE_MESSAGE:

                    /*
          ShowMessages(
              "A buffer of messages (OPERATION_LOG_NON_IMMEDIATE_MESSAGE) :\n");
              */
                    ShowMessages("%s", OutputBuffer + sizeof(UINT32));
                    break;
                case OPERATION_LOG_INFO_MESSAGE:

                    /*
           ShowMessages("Information log (OPERATION_LOG_INFO_MESSAGE) :\n");
           */
                    ShowMessages("%s", OutputBuffer + sizeof(UINT32));
                    break;
                case OPERATION_LOG_ERROR_MESSAGE:
                    /*
        ShowMessages("Error log (OPERATION_LOG_ERROR_MESSAGE) :\n");
        */
                    ShowMessages("%s", OutputBuffer + sizeof(UINT32));
                    break;
                case OPERATION_LOG_WARNING_MESSAGE:
                    /*
        ShowMessages("Warning log (OPERATION_LOG_WARNING_MESSAGE) :\n");
        */
                    ShowMessages("%s", OutputBuffer + sizeof(UINT32));
                    break;

                case OPERATION_COMMAND_FROM_DEBUGGER_CLOSE_AND_UNLOAD_VMM:
                    KdCloseConnection();
                    break;

                case OPERATION_DEBUGGEE_USER_INPUT:

                    KdHandleUserInputInDebuggee(OutputBuffer + sizeof(UINT32));
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
                        (PDEBUGGER_MODIFY_EVENTS)(OutputBuffer + sizeof(UINT32)));

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

                default:
                    /*
        ShowMessages("Message From Debugger :\n");
        */

                    //
                    // Set output source to not found
                    //
                    OutputSourceFound = FALSE;

                    //
                    // Check if there are available output sources
                    //
                    if (g_OutputSourcesInitialized)
                    {
                        //
                        // Now, we should check whether the following flag matches
                        // with an output or not, also this is not where we want to
                        // check output resources
                        //
                        TempList = &g_EventTrace;
                        while (&g_EventTrace != TempList->Blink)
                        {
                            TempList = TempList->Blink;

                            PDEBUGGER_GENERAL_EVENT_DETAIL EventDetail = CONTAINING_RECORD(
                                TempList,
                                DEBUGGER_GENERAL_EVENT_DETAIL,
                                CommandsEventList);

                            if (EventDetail->HasCustomOutput)
                            {
                                //
                                // Output source found
                                //
                                OutputSourceFound = TRUE;

                                //
                                // Send the event to output sources
                                //
                                if (!ForwardingPerformEventForwarding(
                                        EventDetail,
                                        OutputBuffer + sizeof(UINT32),
                                        ReturnedLength - sizeof(UINT32)))
                                {
                                    ShowMessages("err, there was an error transferring the "
                                                 "message to the remote sources.\n");
                                }

                                break;
                            }
                        }
                    }

                    //
                    // Show the message if the source not found
                    //
                    if (!OutputSourceFound)
                    {
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
                return;
            }
        }
    }
    catch (const std::exception &)
    {
        ShowMessages(" Exception !\n");
    }
    free(OutputBuffer);
}

/**
 * @brief Create a thread for pending buffers
 *
 * @param Data
 * @return DWORD Device Handle
 */
DWORD WINAPI
ThreadFunc(void * data)
{
    //
    // Do stuff.  This will be the first function called on the new
    // thread. When this function returns, the thread goes away.  See
    // MSDN for more details. Test Irp Based Notifications
    //
    ReadIrpBasedBuffer();

    return 0;
}
#endif

/**
 * @brief Install the driver
 *
 * @return int return zero if it was successful or non-zero if there
 * was error
 */
HPRDBGCTRL_API int
HyperdbgInstallVmmDriver()
{
    //
    // The driver is not started yet so let us the install driver.
    // First setup full path to driver name.
    //

    if (!SetupDriverName(g_DriverLocation, sizeof(g_DriverLocation)))
    {
        return 1;
    }

    if (!ManageDriver(DRIVER_NAME, g_DriverLocation, DRIVER_FUNC_INSTALL))
    {
        ShowMessages("Unable to install driver\n");

        //
        // Error - remove driver.
        //
        ManageDriver(DRIVER_NAME, g_DriverLocation, DRIVER_FUNC_REMOVE);

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
HPRDBGCTRL_API int
HyperdbgStopDriver()
{
    //
    // Unload the driver if loaded.  Ignore any errors.
    //
    if (g_DriverLocation[0] != (TCHAR)0)
    {
        ManageDriver(DRIVER_NAME, g_DriverLocation, DRIVER_FUNC_STOP);
    }
    return 0;
}

/**
 * @brief Remove the driver
 *
 * @return int return zero if it was successful or non-zero if there
 * was error
 */
HPRDBGCTRL_API int
HyperdbgUninstallDriver()
{
    //
    // Unload the driver if loaded.  Ignore any errors.
    //
    if (g_DriverLocation[0] != (TCHAR)0)
    {
        ManageDriver(DRIVER_NAME, g_DriverLocation, DRIVER_FUNC_REMOVE);
    }
    return 0;
}

/**
 * @brief Load the driver
 *
 * @return int return zero if it was successful or non-zero if there
 * was error
 */
HPRDBGCTRL_API int
HyperdbgLoadVmm()
{
    string CpuID;
    DWORD  ErrorNum;
    HANDLE hProcess;
    DWORD  ThreadId;

    if (g_DeviceHandle)
    {
        ShowMessages("Handle of driver found, if you use 'load' before, please "
                     "first unload it then call 'unload'.\n");
        return 1;
    }

    CpuID = ReadVendorString();

    ShowMessages("Current processor vendor is : %s\n", CpuID.c_str());

    if (CpuID == "GenuineIntel")
    {
        ShowMessages("The Processor virtualization technology is VT-x\n");
    }
    else
    {
        ShowMessages("This program is not designed to run in a non-VT-x "
                     "environemnt !\n");
        return 1;
    }

    if (VmxSupportDetection())
    {
        ShowMessages("VMX Operation is supported by your processor\n");
    }
    else
    {
        ShowMessages("VMX Operation is not supported by your processor\n");
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
        "\\\\.\\HyperdbgHypervisorDevice",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, /// lpSecurityAttirbutes
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        NULL); /// lpTemplateFile

    if (g_DeviceHandle == INVALID_HANDLE_VALUE)
    {
        ErrorNum = GetLastError();
        if (ErrorNum == 5)
        {
            ShowMessages("Error: Access denied! Are you sure you have administrator "
                         "rights?\n");
        }
        else
        {
            ShowMessages("CreateFile failed with error: 0x%x\n", ErrorNum);
        }

        g_DeviceHandle = NULL;
        return 1;
    }

    //
    // Initialize the list of events
    //
    InitializeListHead(&g_EventTrace);

#if !UseDbgPrintInsteadOfUsermodeMessageTracking
    HANDLE Thread = CreateThread(NULL, 0, ThreadFunc, NULL, 0, &ThreadId);

    if (Thread)
    {
        // ShowMessages("Thread Created successfully\n");
    }

#endif

    //
    // Register the CTRL+C and CTRL+BREAK Signals handler
    //
    if (!SetConsoleCtrlHandler(BreakController, TRUE))
    {
        ShowMessages("Error in registering CTRL+C and CTRL+BREAK Signals "
                     "handler\n");
        return 1;
    }

    return 0;
}

/**
 * @brief Unload driver
 *
 * @return int return zero if it was successful or non-zero if there
 * was error
 */
HPRDBGCTRL_API int
HyperdbgUnload()
{
    BOOL Status;

    if (!g_DeviceHandle)
    {
        ShowMessages("Handle not found, probably the driver is not "
                     "initialized.\n");
        return 1;
    }

    ShowMessages("Start terminating VMX...\n");

    //
    // Send IOCTL to mark complete all IRP Pending
    //
    Status = DeviceIoControl(g_DeviceHandle,      // Handle to device
                             IOCTL_TERMINATE_VMX, // IO Control code
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
        ShowMessages("Error : 0x%x\n", GetLastError());
    };

    //
    // Null the handle to indicate that the driver's device is not ready
    // to use
    //
    g_DeviceHandle = NULL;

    ShowMessages("You're not on HyperDbg's hypervisor anymore !\n");
}
