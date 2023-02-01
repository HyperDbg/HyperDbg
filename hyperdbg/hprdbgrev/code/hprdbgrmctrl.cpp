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
 * @brief load reversing machine module
 *
 * @return BOOLEAN
 */
BOOLEAN
CommandLoadReversingMachineModule()
{
    BOOL Status;
    HANDLE hToken;

    //
    // Enable Debug privilege
    //
    Status = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);
    if (!Status) {
        ShowMessages("err, OpenProcessToken failed (%x)\n", GetLastError());
        return FALSE;
    }

    Status = SetPrivilege(hToken, SE_DEBUG_NAME, TRUE);
    if (!Status) {
        CloseHandle(hToken);
        return FALSE;
    }

    //
    // Install reversing machine's driver
    //
    if (HyperDbgInstallReversingMachineDriver() == 1) {
        return FALSE;
    }

    //
    // Create event to show if the hypervisor is loaded or not
    //
    g_IsDriverLoadedSuccessfully = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (HyperDbgLoadVmm() == 1) {
        //
        // No need to handle anymore
        //
        CloseHandle(g_IsDriverLoadedSuccessfully);
        return FALSE;
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
    g_IsReversingMachineModulesLoaded = TRUE;

    ShowMessages("reversing machine module is running...\n");

    return TRUE;
}

/**
 * @brief Install RM driver
 *
 * @return int return zero if it was successful or non-zero if there
 * was error
 */
int HyperDbgInstallReversingMachineDriver()
{
    //
    // The driver is not started yet so let us the install driver
    // First setup full path to driver name
    //

    if (!SetupDriverName(g_DriverLocation, sizeof(g_DriverLocation))) {
        return 1;
    }

    if (!ManageDriver(KERNEL_REVERSING_MACHINE_DRIVER_NAME, g_DriverLocation, DRIVER_FUNC_INSTALL)) {
        ShowMessages("unable to install driver\n");

        //
        // Error - remove driver
        //
        ManageDriver(KERNEL_REVERSING_MACHINE_DRIVER_NAME, g_DriverLocation, DRIVER_FUNC_REMOVE);

        return 1;
    }

    return 0;
}

/**
 * @brief Load the reversing machine driver
 *
 * @return int return zero if it was successful or non-zero if there
 * was error
 */
int HyperDbgLoadReversingMachine()
{
    string CpuID;
    DWORD ErrorNum;
    HANDLE hProcess;
    DWORD ThreadId;

    if (g_DeviceHandle) {
        ShowMessages("handle of the driver found, if you use 'load' before, please "
                     "unload it using 'unload'\n");
        return 1;
    }

    CpuID = ReadVendorString();

    ShowMessages("current processor vendor is : %s\n", CpuID.c_str());

    if (CpuID == "GenuineIntel") {
        ShowMessages("virtualization technology is vt-x\n");
    } else {
        ShowMessages("this program is not designed to run in a non-VT-x "
                     "environemnt !\n");
        return 1;
    }

    if (VmxSupportDetection()) {
        ShowMessages("vmx operation is supported by your processor\n");
    } else {
        ShowMessages("vmx operation is not supported by your processor\n");
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

#if !UseDbgPrintInsteadOfUsermodeMessageTracking
    HANDLE Thread = CreateThread(NULL, 0, ThreadFunc, NULL, 0, &ThreadId);

    // if (Thread)
    // {
    // ShowMessages("thread Created successfully\n");
    // }

#endif

    return 0;
}

int main()
{
    std::cout << "Hello World!\n";
}
