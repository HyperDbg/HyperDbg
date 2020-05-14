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

//
// Global Variables
//
HANDLE DeviceHandle;
using namespace std;
BOOLEAN IsVmxOffProcessStart; // Show whether the vmxoff process start or not
Callback Handler = 0;
TCHAR driverLocation[MAX_PATH] = {0};

/**
 * @brief Set the function callback that will be called if anything received
 * from the kernel
 *
 * @param handler Function that handles the messages
 */
void __stdcall HyperdbgSetTextMessageCallback(Callback handler) {
  Handler = handler;
}

/**
 * @brief Show messages received from kernel driver
 *
 * @param Fmt
 */
void ShowMessages(const char *Fmt, ...) {

  va_list ArgList;
  char TempMessage[PacketChunkSize];

  va_start(ArgList, Fmt);

  int sprintfresult =
      vsprintf_s(TempMessage, PacketChunkSize - 1, Fmt, ArgList);
  va_end(ArgList);

  if (sprintfresult != -1) {
    if (Handler != NULL) {
      Handler(TempMessage);
    } else {
      printf(TempMessage);
    }
  } else {
    MessageBoxA(0, "Error occured in send date to managed code !", "error", 0);
  }
}

/**
 * @brief Detect VMX support
 *
 * @return true if vmx is supported
 * @return false if vmx is not supported
 */
bool VmxSupportDetection() { return AsmVmxSupportDetection(); }

/**
 * @brief SetPrivilege enables/disables process token privilege
 *
 * @param hToken
 * @param lpszPrivilege
 * @param bEnablePrivilege
 * @return BOOL
 */
BOOL SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege) {
  LUID luid;
  BOOL bRet = FALSE;

  if (LookupPrivilegeValue(NULL, lpszPrivilege, &luid)) {
    TOKEN_PRIVILEGES tp;

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = (bEnablePrivilege) ? SE_PRIVILEGE_ENABLED : 0;
    //
    //  Enable the privilege or disable all privileges.
    //
    if (AdjustTokenPrivileges(hToken, FALSE, &tp, NULL, (PTOKEN_PRIVILEGES)NULL,
                              (PDWORD)NULL)) {
      //
      //  Check to see if you have proper access.
      //  You may get "ERROR_NOT_ALL_ASSIGNED".
      //
      bRet = (GetLastError() == ERROR_SUCCESS);
    }
  }
  return bRet;
}

#if !UseDbgPrintInsteadOfUsermodeMessageTracking

/**
 * @brief Read kernel buffers using IRP Pending
 *
 * @param Device Driver handle
 */
void ReadIrpBasedBuffer() {

  BOOL Status;
  ULONG ReturnedLength;
  REGISTER_NOTIFY_BUFFER RegisterEvent;
  UINT32 OperationCode;
  DWORD ErrorNum;
  HANDLE Handle;

  ShowMessages(" =============================== Kernel-Mode Logs (Driver) "
               "===============================\n");

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
      "\\\\.\\HyperdbgHypervisorDevice", GENERIC_READ | GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE,
      NULL, /// lpSecurityAttirbutes
      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
      NULL); /// lpTemplateFile

  if (Handle == INVALID_HANDLE_VALUE) {
    ErrorNum = GetLastError();
    if (ErrorNum == 5) {
      ShowMessages("Error: Access denied! Are you sure you have administrator "
                   "rights?\n");

    } else {
      ShowMessages("CreateFile failed with error: 0x%x\n", ErrorNum);
    }
    return;
  }

  //
  // allocate buffer for transfering messages
  //
  char *OutputBuffer = (char *)malloc(UsermodeBufferSize);

  try {

    while (TRUE) {
      if (!IsVmxOffProcessStart) {
        ZeroMemory(OutputBuffer, UsermodeBufferSize);

        Sleep(200); // we're not trying to eat all of the CPU ;)

        Status = DeviceIoControl(
            Handle,               // Handle to device
            IOCTL_REGISTER_EVENT, // IO Control code
            &RegisterEvent,       // Input Buffer to driver.
            SIZEOF_REGISTER_EVENT *
                2, // Length of input buffer in bytes. (x 2 is bcuz as the
                   // driver is x64 and has 64 bit values)
            OutputBuffer,       // Output Buffer from driver.
            UsermodeBufferSize, // Length of output buffer in bytes.
            &ReturnedLength,    // Bytes placed in buffer.
            NULL                // synchronous call
        );

        if (!Status) {
          ShowMessages("Ioctl failed with code 0x%x\n", GetLastError());
          break;
        }
        ShowMessages("========================= Kernel Mode (Buffer) "
                     "=========================\n");

        OperationCode = 0;
        memcpy(&OperationCode, OutputBuffer, sizeof(UINT32));

        ShowMessages("Returned Length : 0x%x \n", ReturnedLength);
        ShowMessages("Operation Code : 0x%x \n", OperationCode);

        switch (OperationCode) {
        case OPERATION_LOG_NON_IMMEDIATE_MESSAGE:

          ShowMessages(
              "A buffer of messages (OPERATION_LOG_NON_IMMEDIATE_MESSAGE) :\n");
          ShowMessages("\n\nfucccckkk 1 \n\n");

          ShowMessages("%s\n", OutputBuffer + sizeof(UINT32));
          ShowMessages("\n\nfucccckkk 2 \n\n");

          break;
        case OPERATION_LOG_INFO_MESSAGE:
          ShowMessages("Information log (OPERATION_LOG_INFO_MESSAGE) :\n");
          ShowMessages("%s\n", OutputBuffer + sizeof(UINT32));
          break;
        case OPERATION_LOG_ERROR_MESSAGE:
          ShowMessages("Error log (OPERATION_LOG_ERROR_MESSAGE) :\n");
          ShowMessages("%s\n", OutputBuffer + sizeof(UINT32));
          break;
        case OPERATION_LOG_WARNING_MESSAGE:
          ShowMessages("Warning log (OPERATION_LOG_WARNING_MESSAGE) :\n");
          ShowMessages("%s\n", OutputBuffer + sizeof(UINT32));
          break;

        default:
          break;
        }
      } else {
        //
        // the thread should not work anymore
        //
        return;
      }
    }
  } catch (const std::exception &) {
    ShowMessages(" Exception !\n");
  }
}

/**
 * @brief Create a thread for pending buffers
 *
 * @param Data
 * @return DWORD Device Handle
 */
DWORD WINAPI ThreadFunc(void *data) {
  //
  // Do stuff.  This will be the first function called on the new thread.
  // When this function returns, the thread goes away.  See MSDN for more
  // details. Test Irp Based Notifications
  //
  ReadIrpBasedBuffer();

  return 0;
}
#endif

/**
 * @brief Install the driver
 *
 * @return int return zero if it was successful or non-zero if there was error
 */
HPRDBGCTRL_API int HyperdbgInstallDriver() {
  ReadVendorString();
  //
  // The driver is not started yet so let us the install driver.
  // First setup full path to driver name.
  //

  if (!SetupDriverName(driverLocation, sizeof(driverLocation))) {

    return 1;
  }

  if (!ManageDriver(DRIVER_NAME, driverLocation, DRIVER_FUNC_INSTALL)) {

    ShowMessages("Unable to install driver\n");

    //
    // Error - remove driver.
    //

    ManageDriver(DRIVER_NAME, driverLocation, DRIVER_FUNC_REMOVE);

    return 1;
  }
  return 0;
}

/**
 * @brief Uninstall the driver
 *
 * @return int return zero if it was successful or non-zero if there was error
 */
HPRDBGCTRL_API int HyperdbgUninstallDriver() {
  //
  // Unload the driver if loaded.  Ignore any errors.
  //
  if (driverLocation[0] != (TCHAR)0) {
    ManageDriver(DRIVER_NAME, driverLocation, DRIVER_FUNC_REMOVE);
  }
  return 0;
}

/**
 * @brief Load the driver
 *
 * @return int return zero if it was successful or non-zero if there was error
 */
HPRDBGCTRL_API int HyperdbgLoad() {

  string CpuID;
  DWORD ErrorNum;
  BOOL Status;
  HANDLE hProcess;
  HANDLE hToken;

  if (DeviceHandle) {
    ShowMessages("Handle of driver found, if you use 'load' before, please "
                 "first unload it then call 'unload'.\n");
    return 1;
  }

  CpuID = ReadVendorString();

  ShowMessages("The CPU Vendor is : %s\n", CpuID.c_str());

  if (CpuID == "GenuineIntel") {
    ShowMessages("The Processor virtualization technology is VT-x.\n");
  } else {
    ShowMessages(
        "This program is not designed to run in a non-VT-x environemnt !\n");
    return 1;
  }

  if (VmxSupportDetection()) {
    ShowMessages("VMX Operation is supported by your processor .\n");
  } else {
    ShowMessages("VMX Operation is not supported by your processor .\n");
    return 1;
  }
  //
  // Enable Debug privilege
  //
  hProcess = GetCurrentProcess();

  if (OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken)) {
    SetPrivilege(hToken, SE_DEBUG_NAME, TRUE);
    CloseHandle(hToken);
  }

  DeviceHandle = CreateFileA(
      "\\\\.\\HyperdbgHypervisorDevice", GENERIC_READ | GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE,
      NULL, /// lpSecurityAttirbutes
      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
      NULL); /// lpTemplateFile

  if (DeviceHandle == INVALID_HANDLE_VALUE) {
    ErrorNum = GetLastError();
    if (ErrorNum == 5) {
      ShowMessages("Error: Access denied! Are you sure you have administrator "
                   "rights?\n");

    } else {
      ShowMessages("CreateFile failed with error: 0x%x\n", ErrorNum);
    }
    return 1;
  }

#if !UseDbgPrintInsteadOfUsermodeMessageTracking

  HANDLE Thread = CreateThread(NULL, 0, ThreadFunc, NULL, 0, NULL);
  if (Thread) {
    ShowMessages("Thread Created successfully !!!\n");
  }
#endif

  return 0;
}

/**
 * @brief Unload driver
 *
 * @return int return zero if it was successful or non-zero if there was error
 */
HPRDBGCTRL_API int HyperdbgUnload() {
  BOOL Status;

  if (!DeviceHandle) {
    ShowMessages("Handle not found, probably the driver is not initialized.\n");
    return 1;
  }

  ShowMessages("Terminating VMX !\n");

  //
  // Send IOCTL to mark complete all IRP Pending
  //
  Status = DeviceIoControl(DeviceHandle,        // Handle to device
                           IOCTL_TERMINATE_VMX, // IO Control code
                           NULL,                // Input Buffer to driver.
                           0, // Length of input buffer in bytes. (x 2 is bcuz
                              // as the driver is x64 and has 64 bit values)
                           NULL, // Output Buffer from driver.
                           0,    // Length of output buffer in bytes.
                           NULL, // Bytes placed in buffer.
                           NULL  // synchronous call
  );

  //
  // wait to make sure we don't use an invalid handle in another Ioctl
  //
  if (!Status) {
    ShowMessages("Ioctl failed with code 0x%x\n", GetLastError());
  }

  //
  // Send IOCTL to mark complete all IRP Pending
  //
  Status = DeviceIoControl(
      DeviceHandle,                                        // Handle to device
      IOCTL_RETURN_IRP_PENDING_PACKETS_AND_DISALLOW_IOCTL, // IO Control code
      NULL, // Input Buffer to driver.
      0, // Length of input buffer in bytes. (x 2 is bcuz as the driver is x64
         // and has 64 bit values)
      NULL, // Output Buffer from driver.
      0,    // Length of output buffer in bytes.
      NULL, // Bytes placed in buffer.
      NULL  // synchronous call
  );

  //
  // wait to make sure we don't use an invalid handle in another Ioctl
  //
  if (!Status) {
    ShowMessages("Ioctl failed with code 0x%x\n", GetLastError());
  }

  //
  // Indicate that the finish process start or not
  //
  IsVmxOffProcessStart = TRUE;

  Sleep(1000); // Wait so next thread can return from IRP Pending

  //
  // Send IRP_MJ_CLOSE to driver to terminate Vmxs
  //
  if (!CloseHandle(DeviceHandle)) {
    ShowMessages("Error : 0x%x\n", GetLastError());
  };

  //
  // Null the handle to indicate that the driver's device is not ready to use
  //
  DeviceHandle = NULL;

  ShowMessages("You're not on hypervisor anymore !\n");
}
