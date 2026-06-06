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
extern BOOLEAN    g_IsMessageLoggingWindowClosed;
extern TCHAR      g_DriverLocation[MAX_PATH];
extern TCHAR      g_DriverName[MAX_PATH];
extern BOOLEAN    g_UseCustomDriverLocation;
extern LIST_ENTRY g_EventTrace;
extern BOOLEAN    g_IsKdModuleLoaded;
extern BOOLEAN    g_IsVmmModuleLoaded;
extern BOOLEAN    g_IsHyperTraceModuleLoaded;

/**
 * @brief Install (start) VMM driver
 *
 * @return INT return zero if it was successful or non-zero if there
 * was error
 */
INT
HyperDbgStartDriver()
{
    if (!ManageDriver(g_DriverName, g_DriverLocation, DRIVER_FUNC_INSTALL))
    {
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
 * @return INT return zero if it was successful or non-zero if there
 * was error
 */
INT
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
 * @brief Install KD (Kernel Debugger) driver
 *
 * @return INT return zero if it was successful or non-zero if there
 * was error
 */
INT
HyperDbgInstallKdDriver()
{
    //
    // Check if the driver is already loaded, if that's the case, we shouldn't try to load it again
    //
    if (g_IsKdModuleLoaded)
    {
        //
        // The driver is already loaded, so we shouldn't try to load it again
        // but we can consider it as success and return zero
        //
        return 0;
    }

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

    if (HyperDbgStartDriver() != 0)
    {
        ShowMessages("unable to install KD driver\n");

        return 1;
    }

    return 0;
}

/**
 * @brief Start KD driver
 *
 * @return INT return zero if it was successful or non-zero if there was error
 */
INT
HyperDbgStartKdDriver()
{
    return HyperDbgStartDriver();
}

/**
 * @brief Stop KD driver
 *
 * @return INT return zero if it was successful or non-zero if there was error
 */
INT
HyperDbgStopKdDriver()
{
    return HyperDbgStopDriver(g_DriverName);
}

/**
 * @brief Remove the driver
 *
 * @return INT return zero if it was successful or non-zero if there
 * was error
 */
INT
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
 * @brief Remove the KD (Kernel Debugger) driver
 *
 * @return INT return zero if it was successful or non-zero if there
 * was error
 */
INT
HyperDbgUninstallKdDriver()
{
    return HyperDbgUninstallDriver(g_DriverName);
}

/**
 * @brief Initialize VMM module
 *
 * @return INT return zero if it was successful or non-zero if there
 * was error
 */
INT
HyperDbgInitHyperTraceModule()
{
    BOOL                            Status;
    DWORD                           BytesReturned;
    DEBUGGER_INIT_HYPERTRACE_PACKET InitHyperTracePacket = {0};

    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnOne);

    //
    // Send IOCTL to initialize HyperTrace module
    //
    Status = DeviceIoControl(g_DeviceHandle,                         // Handle to device
                             IOCTL_INIT_HYPERTRACE,                  // IO Control Code (IOCTL)
                             &InitHyperTracePacket,                  // Input Buffer to driver.
                             SIZEOF_DEBUGGER_INIT_HYPERTRACE_PACKET, // Length of input buffer in bytes.
                             &InitHyperTracePacket,                  // Output Buffer from driver.
                             SIZEOF_DEBUGGER_INIT_HYPERTRACE_PACKET, // Length of output buffer in bytes.
                             &BytesReturned,                         // Bytes placed in buffer.
                             NULL                                    // synchronous call
    );

    //
    // Check if the IOCTL was successful, if not show the error message and return
    //
    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        return 1;
    }

    //
    // Check the kernel status
    //
    if (InitHyperTracePacket.KernelStatus != DEBUGGER_OPERATION_WAS_SUCCESSFUL)
    {
        ShowErrorMessage(InitHyperTracePacket.KernelStatus);
        return 1;
    }

    return 0;
}

/**
 * @brief Initialize VMM module
 *
 * @return INT return zero if it was successful or non-zero if there
 * was error
 */
INT
HyperDbgInitVmmModule()
{
    BOOL                     Status;
    DWORD                    BytesReturned;
    DEBUGGER_INIT_VMM_PACKET InitVmmPacket = {0};

    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnOne);

    //
    // Create event to show if the kd module is loaded or not
    //
    g_IsDriverLoadedSuccessfully = CreateEvent(NULL, FALSE, FALSE, NULL);

    //
    // Send IOCTL to initialize VMM module
    //
    Status = DeviceIoControl(g_DeviceHandle,                  // Handle to device
                             IOCTL_INIT_VMM,                  // IO Control Code (IOCTL)
                             &InitVmmPacket,                  // Input Buffer to driver.
                             SIZEOF_DEBUGGER_INIT_VMM_PACKET, // Length of input buffer in bytes.
                             &InitVmmPacket,                  // Output Buffer from driver.
                             SIZEOF_DEBUGGER_INIT_VMM_PACKET, // Length of output buffer in bytes.
                             &BytesReturned,                  // Bytes placed in buffer.
                             NULL                             // synchronous call
    );

    //
    // check if the IOCTL was successful, if not show the error message and return
    //
    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        CloseHandle(g_IsDriverLoadedSuccessfully);
        return 1;
    }

    //
    // Check the kernel status for early errors (e.g., HyperTrace already loaded)
    //
    if (InitVmmPacket.KernelStatus != DEBUGGER_OPERATION_WAS_SUCCESSFUL &&
        InitVmmPacket.KernelStatus != 0)
    {
        ShowErrorMessage(InitVmmPacket.KernelStatus);
        CloseHandle(g_IsDriverLoadedSuccessfully);
        return 1;
    }

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
    // VMM module is initialized at this point
    //
    return 0;
}

/**
 * @brief Create handle from KD (HyperKD) module
 *
 * @return INT return zero if it was successful or non-zero if there
 * was error
 */
INT
HyperDbgCreateHandleFromKdModule()
{
    DWORD ErrorNum;
    DWORD ThreadId;

    if (g_DeviceHandle)
    {
        ShowMessages("handle of the driver found, if you use the 'load' command before, please "
                     "unload it using the 'unload' command\n");
        return 1;
    }

    //
    // Make sure that this variable is false, because it might be set to
    // true as the result of a previous load
    //
    g_IsMessageLoggingWindowClosed = FALSE;

    //
    // Init entering vmx
    //
    g_DeviceHandle = CreateFileA(
        "\\\\.\\HyperDbgDebuggerDevice",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, /// lpSecurityAttirbutes
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
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
 * @brief Unload VMM module
 *
 * @return INT return zero if it was successful or non-zero if there
 * was error
 */
INT
HyperDbgUnloadVmm()
{
    BOOL  Status;
    DWORD BytesReturned;

    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnOne);

    ShowMessages("start terminating vmm...\n");

    //
    // Check if HyperTrace module is loaded, if so we need to unload it before unloading VMM
    //
    if (g_IsHyperTraceModuleLoaded)
    {
        ShowMessages(
            "the trace module is currently loaded and will be unloaded before the vmm module\nnote that hypertrace (trace) "
            "use the hypervisor (vmm) features when it is loaded after vmm, however, hypertrace can also operate without the vmm "
            "module, although hypervisor-specific features will not be available\n"
            "the 'trace' module will now be unloaded automatically. You can reload it later "
            "using the command 'load trace', which will load the trace module again without enabling hypervisor-dependent features\n");

        HyperDbgUnloadHyperTrace();
    }

    //
    // Uninitialize the user debugger if it's initialized
    //
    UdUninitializeUserDebugger();

    //
    // Send IOCTL terminate VMX
    //
    Status = DeviceIoControl(g_DeviceHandle,      // Handle to device
                             IOCTL_TERMINATE_VMX, // IO Control Code (IOCTL)
                             NULL,                // Input Buffer to driver.
                             0,                   // Length of input buffer in bytes. (x 2 is bcuz
                                                  // as the driver is x64 and has 64 bit values)
                             NULL,                // Output Buffer from driver.
                             0,                   // Length of output buffer in bytes.
                             &BytesReturned,      // Bytes placed in buffer.
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
    // Hypervisor (VMM) module is not loaded anymore
    //
    g_IsVmmModuleLoaded = FALSE;

    ShowMessages("you're not on HyperDbg's hypervisor anymore!\n");

    return 0;
}

/**
 * @brief Unload HyperTrace module
 *
 * @return INT return zero if it was successful or non-zero if there
 * was error
 */
INT
HyperDbgUnloadHyperTrace()
{
    BOOL  Status;
    DWORD BytesReturned;

    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnOne);

    ShowMessages("start terminating trace module...\n");

    //
    // Send IOCTL to unload HyperTrace module
    //
    Status = DeviceIoControl(g_DeviceHandle,                  // Handle to device
                             IOCTL_PERFORM_HYPERTRACE_UNLOAD, // IO Control Code (IOCTL)
                             NULL,                            // Input Buffer to driver.
                             0,                               // Length of input buffer in bytes. (x 2 is bcuz
                                                              // as the driver is x64 and has 64 bit values)
                             NULL,                            // Output Buffer from driver.
                             0,                               // Length of output buffer in bytes.
                             &BytesReturned,                  // Bytes placed in buffer.
                             NULL                             // synchronous call
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
    // HyperTrace module is not loaded anymore
    //
    g_IsHyperTraceModuleLoaded = FALSE;

    ShowMessages("the trace module is unloaded!\n");

    return 0;
}

/**
 * @brief Unload KD driver
 *
 * @return INT return zero if it was successful or non-zero if there was error
 */
INT
HyperDbgUnloadKd()
{
    BOOL  Status;
    DWORD BytesReturned;

    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnOne);

    //
    // Indicate that the message logging window is closed
    //
    g_IsMessageLoggingWindowClosed = TRUE;

    //
    // Send IOCTL to mark complete all IRP Pending
    //
    Status = DeviceIoControl(
        g_DeviceHandle,                                      // Handle to device
        IOCTL_RETURN_IRP_PENDING_PACKETS_AND_DISALLOW_IOCTL, // IO Control Code (IOCTL)
        NULL,                                                // Input Buffer to driver.
        0,                                                   // Length of input buffer in bytes. (x 2 is bcuz as the
                                                             // driver is x64 and has 64 bit values)
        NULL,                                                // Output Buffer from driver.
        0,                                                   // Length of output buffer in bytes.
        &BytesReturned,                                      // Bytes placed in buffer.
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
    // Wait for a while to make sure that all IRP pending are completed and the driver is ready to be unloaded
    //
    Sleep(1000);

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
    g_IsKdModuleLoaded = FALSE;

    //
    // Check if we found an already built symbol table
    //
    SymbolDeleteSymTable();

    ShowMessages("the debugger module is unloaded!\n");

    return 0;
}

/**
 * @brief unload all modules (KD, VMM, HyperTrace, etc.)
 *
 * @return INT return zero if it was successful or non-zero if there
 * was error
 */
INT
HyperDbgUnloadAllModules()
{
    INT RetVal = 0;

    //
    // Unload HyperTrace module if loaded
    //
    if (g_IsHyperTraceModuleLoaded && HyperDbgUnloadHyperTrace() != 0)
    {
        return 1;
    }

    //
    // Unload VMM module if loaded
    //
    if (g_IsVmmModuleLoaded && HyperDbgUnloadVmm() != 0)
    {
        return 1;
    }

    //
    // Unload KD module if loaded
    //
    if (g_IsKdModuleLoaded && HyperDbgUnloadKd() != 0)
    {
        return 1;
    }

    return 0;
}

/**
 * @brief load kd module
 *
 * @return int return zero if it was successful or non-zero if there
 */
INT
HyperDbgLoadKdModule()
{
    //
    // Check if the module is already loaded, if that's the case, we don't
    // need to handle anymore
    //
    if (g_IsKdModuleLoaded)
    {
        //
        // Return zero to indicate that the module is loaded successfully,
        //  and we no need to re-load it anymore
        //
        return 0;
    }

    if (HyperDbgCreateHandleFromKdModule() == 1)
    {
        //
        // No need to handle anymore
        //
        return 1;
    }

    //
    // KD module is loaded at this point
    //

    //
    // If we reach here so the module are loaded
    //
    g_IsKdModuleLoaded = TRUE;

    return 0;
}

/**
 * @brief Get the vendor of the current processor
 *
 * @return GENERIC_PROCESSOR_VENDOR the vendor of the processor
 */
GENERIC_PROCESSOR_VENDOR
HyperDbgGetProcessorVendor()
{
    CHAR CpuId[13] = {0};

    //
    // Read the vendor string
    //
    CpuReadVendorString(CpuId);

    // ShowMessages("current processor vendor is : %s\n", CpuId);

    if (strcmp(CpuId, "GenuineIntel") == 0)
    {
        return GENERIC_PROCESSOR_VENDOR_INTEL;
    }
    else if (strcmp(CpuId, "AuthenticAMD") == 0)
    {
        return GENERIC_PROCESSOR_VENDOR_AMD;
    }
    else
    {
        return GENERIC_PROCESSOR_VENDOR_OTHERS;
    }
}

/**
 * @brief load vmm module
 *
 * @return int return zero if it was successful or non-zero if there
 */
INT
HyperDbgLoadVmmModule()
{
    //
    // Check if the module is already loaded, if that's the case, we don't
    // need to handle anymore
    //
    if (g_IsVmmModuleLoaded)
    {
        //
        // Return zero to indicate that the module is loaded successfully,
        //  and we no need to re-load it anymore
        //
        return 0;
    }

    //
    // Check if the HyperTrace module is loaded or not
    //
    if (g_IsHyperTraceModuleLoaded)
    {
        ShowMessages(
            "err, the trace module is currently loaded and should be unloaded before loading the vmm module\n"
            "Note that HyperTrace (trace) uses the hypervisor (vmm) features when it is loaded after the vmm module, "
            "however, HyperTrace can also operate without the vmm module, although hypervisor-specific features will not be available\n"
            "to solve this problem, first unload the trace module using the 'unload trace' command, next, load "
            "the vmm module and then load the trace module again. This way, the trace module is reloaded with hypervisor APIs\n");
        return 1;
    }

    //
    // Enable Debug privilege to the current token
    //
    if (!WindowsSetDebugPrivilege())
    {
        ShowMessages("err, couldn't set debug privilege\n");
        return 1;
    }

    //
    // Check if the processor is a genuine Intel processor (required for VT-x)
    //
    if (HyperDbgGetProcessorVendor() != GENERIC_PROCESSOR_VENDOR_INTEL)
    {
        ShowMessages("err, this program is not designed to run in a non-VT-x "
                     "environment. It needs an Intel processor.\n");
        return 1;
    }

    ShowMessages("virtualization technology is vt-x\n");

    if (VmxSupportDetection())
    {
        ShowMessages("vmx operation is supported by your processor\n");
    }
    else
    {
#ifdef HYPERDBG_ENV_WINDOWS
        ShowMessages("vmx operation is not supported by your processor "
                     "(if you are using an Intel processor, it might be because VBS is not disabled!)\n");
#endif
        return 1;
    }

    //
    // Load the KD module and create handle to it
    //
    if (HyperDbgLoadKdModule() == 1)
    {
        return 1;
    }

    //
    // Initialize VMM module
    //
    if (HyperDbgInitVmmModule() == 1)
    {
        ShowMessages("err, initializing VMM module\n");

        return 1;
    }

    //
    // If we reach here so the module are loaded
    //
    g_IsVmmModuleLoaded = TRUE;

    ShowMessages("vmm module is running...\n");

    return 0;
}

/**
 * @brief load hypertrace module
 *
 * @return int return zero if it was successful or non-zero if there
 * was error
 */
INT
HyperDbgLoadHyperTraceModule()
{
    //
    // Check if the module is already loaded, if that's the case, we don't
    // need to handle anymore
    //
    if (g_IsHyperTraceModuleLoaded)
    {
        //
        // Return zero to indicate that the module is loaded successfully,
        //  and we no need to re-load it anymore
        //
        return 0;
    }

    //
    // Enable Debug privilege to the current token
    //
    if (!WindowsSetDebugPrivilege())
    {
        ShowMessages("err, couldn't set debug privilege\n");
        return 1;
    }

    //
    // Check if the processor is a genuine Intel processor (required for HyperTrace)
    //
    if (HyperDbgGetProcessorVendor() != GENERIC_PROCESSOR_VENDOR_INTEL)
    {
        ShowMessages("err, this program is not designed to run in a non-Intel "
                     "environment as it needs Intel PT (Processor Trace) and Intel LBR "
                     "(Last Branch Record). It needs an Intel processor.\n");
        return 1;
    }

    //
    // Load the KD module and create handle to it
    //
    if (HyperDbgLoadKdModule() == 1)
    {
        return 1;
    }

    //
    // Initialize HyperTrace module
    //
    if (HyperDbgInitHyperTraceModule() == 1)
    {
        ShowMessages("err, initializing hypertrace module\n");

        return 1;
    }

    //
    // If we reach here so the module are loaded
    //
    g_IsHyperTraceModuleLoaded = TRUE;

    ShowMessages("hypertrace (trace) module is running...\n");

    return 0;
}
