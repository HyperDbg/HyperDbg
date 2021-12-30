/**
 * @file usermode-debugging.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief control the user-mode debugging affairs
 * @details
 * @version 0.1
 * @date 2021-12-24
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "..\hprdbgctrl\pch.h"

//
// Global Variables
//
extern DEBUGGING_STATE g_DebuggingState;

/**
 * @brief print error messages relating to the finding thread id
 * @details this function is used only in the scope of Thread32First
 * 
 * @return VOID
 */
VOID
UsermodeDebuggingPrintError()
{
    DWORD   ErrNum;
    TCHAR   SysMsg[256];
    TCHAR * p;

    ErrNum = GetLastError();
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  ErrNum,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                  SysMsg,
                  256,
                  NULL);

    //
    // Trim the end of the line and terminate it with a null
    //
    p = SysMsg;
    while ((*p > 31) || (*p == 9))
        ++p;
    do
    {
        *p-- = 0;
    } while ((p >= SysMsg) && ((*p == '.') || (*p < 33)));

    //
    // Display the message
    //
    ShowMessages("\n  WARNING: Thread32First failed with error (%x:%s)", ErrNum, SysMsg);
}

/**
 * @brief List of threads by owner process id
 *
 * @param OwnerPID
 * @return BOOL if there was an error then returns false, otherwise return true
 */
BOOL
UsermodeDebuggingListProcessThreads(DWORD OwnerPID)
{
    HANDLE        ThreadSnap = INVALID_HANDLE_VALUE;
    THREADENTRY32 Te32;

    //
    // Take a snapshot of all running threads
    //
    ThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (ThreadSnap == INVALID_HANDLE_VALUE)
        return FALSE;

    //
    // Fill in the size of the structure before using it
    //
    Te32.dwSize = sizeof(THREADENTRY32);

    //
    // Retrieve information about the first thread and exit if unsuccessful
    //
    if (!Thread32First(ThreadSnap, &Te32))
    {
        UsermodeDebuggingPrintError(); // Show cause of failure
        CloseHandle(ThreadSnap);       // Must clean up the snapshot object!
        return FALSE;
    }
    ShowMessages("\nThread's of pid\t= 0x%08X\n", OwnerPID);

    //
    // Now walk the thread list of the system, and display information
    // about each thread associated with the specified process
    //
    do
    {
        if (Te32.th32OwnerProcessID == OwnerPID)
        {
            ShowMessages("\n     Thread Id\t= 0x%08X", Te32.th32ThreadID);
            // ShowMessages("\n     base priority  = %x", Te32.tpBasePri);
            // ShowMessages("\n     delta priority = %x", Te32.tpDeltaPri);
        }
    } while (Thread32Next(ThreadSnap, &Te32));

    ShowMessages("\n");

    //
    // Don't forget to clean up the snapshot object
    //
    CloseHandle(ThreadSnap);
    return TRUE;
}

/**
 * @brief Check if a thread belongs to special process
 *
 * @param Pid Process id
 * @param Tid Thread id
 * @return BOOLEAN if the thread belongs to process then return true; otherwise
 * returns false
 */
BOOLEAN
UsermodeDebuggingCheckThreadByProcessId(DWORD Pid, DWORD Tid)
{
    HANDLE        ThreadSnap = INVALID_HANDLE_VALUE;
    THREADENTRY32 Te32;
    BOOLEAN       Result = FALSE;

    //
    // Take a snapshot of all running threads
    //
    ThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (ThreadSnap == INVALID_HANDLE_VALUE)
        return FALSE;

    //
    // Fill in the size of the structure before using it
    //
    Te32.dwSize = sizeof(THREADENTRY32);

    //
    // Retrieve information about the first thread and exit if unsuccessful
    //
    if (!Thread32First(ThreadSnap, &Te32))
    {
        UsermodeDebuggingPrintError(); // Show cause of failure
        CloseHandle(ThreadSnap);       // Must clean up the snapshot object!
        return FALSE;
    }

    //
    // Now walk the thread list of the system, and display information
    // about each thread associated with the specified process
    //
    do
    {
        if (Te32.th32OwnerProcessID == Pid)
        {
            if (Te32.th32ThreadID == Tid)
            {
                //
                // The thread found in target process
                //
                Result = TRUE;
                break;
            }
        }
    } while (Thread32Next(ThreadSnap, &Te32));

    //
    // Don't forget to clean up the snapshot object
    //
    CloseHandle(ThreadSnap);
    return Result;
}

/**
 * @brief Attach to a target process
 * @param FileName
 * @param ProcessInformation
 * 
 * @return BOOLEAN
 */
BOOLEAN
UsermodeDebuggingCreateSuspendedProcess(const WCHAR * FileName, PPROCESS_INFORMATION ProcessInformation)
{
    STARTUPINFOW StartupInfo;
    BOOL         CreateProcessResult;

    memset(&StartupInfo, 0, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(STARTUPINFOA);

    //
    // Create process suspended
    //
    CreateProcessResult = CreateProcessW(FileName,
                                         NULL,
                                         NULL,
                                         NULL,
                                         FALSE,
                                         CREATE_SUSPENDED,
                                         NULL,
                                         NULL,
                                         &StartupInfo,
                                         ProcessInformation);

    if (!CreateProcessResult)
    {
        ShowMessages("err, start process failed (%x)", GetLastError());
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Attach to target process
 * @details this function will not check whether the process id and
 * thread id is valid or not
 *
 * @param TargetPid
 * @param TargetTid
 * @param TargetFileAddress
 * @return BOOLEAN
 */
BOOLEAN
UsermodeDebuggingAttachToProcess(UINT32 TargetPid, UINT32 TargetTid, const WCHAR * TargetFileAddress)
{
    BOOLEAN                                  Status;
    ULONG                                    ReturnedLength;
    DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS AttachRequest = {0};
    PROCESS_INFORMATION                      ProcInfo      = {0};

    //
    // Check if debugger is loaded or not
    //
    if (!g_DeviceHandle)
    {
        ShowMessages("handle of the driver not found, probably the driver is not loaded. Did you "
                     "use 'load' command?\n");
        return FALSE;
    }

    //
    // Check whether it's starting a new process or not
    //
    if (TargetFileAddress == NULL)
    {
        AttachRequest.IsStartingNewProcess = FALSE;
    }
    else
    {
        AttachRequest.IsStartingNewProcess = TRUE;
    }

    //
    // We wanna attach to a remote process
    //
    AttachRequest.IsAttach = TRUE;

    if (AttachRequest.IsStartingNewProcess)
    {
        //
        // Start the process in suspended state
        //
        UsermodeDebuggingCreateSuspendedProcess(TargetFileAddress, &ProcInfo);

        //
        // Set the process id and thread id
        //
        AttachRequest.ProcessId = ProcInfo.dwProcessId;
        AttachRequest.ThreadId  = ProcInfo.dwThreadId;
    }
    else
    {
        //
        // Set the process id and thread id
        //
        AttachRequest.ProcessId = TargetPid;
        AttachRequest.ThreadId  = TargetTid;
    }

    //
    // Send the request to the kernel
    //

    Status = DeviceIoControl(
        g_DeviceHandle,                                  // Handle to device
        IOCTL_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS,  // IO Control
                                                         // code
        &AttachRequest,                                  // Input Buffer to driver.
        SIZEOF_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS, // Input buffer length
        &AttachRequest,                                  // Output Buffer from driver.
        SIZEOF_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS, // Length of output
                                                         // buffer in bytes.
        &ReturnedLength,                                 // Bytes placed in buffer.
        NULL                                             // synchronous call
    );

    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        return FALSE;
    }

    //
    // Check if attaching was successful then we can set the attached to true
    //
    if (AttachRequest.Result == DEBUGGER_OPERATION_WAS_SUCCESSFULL)
    {
        g_DebuggingState.IsAttachedToUsermodeProcess = TRUE;
        g_DebuggingState.ConnectedProcessId          = TargetPid;
        g_DebuggingState.ConnectedThreadId           = TargetTid;

        ShowMessages("Base Address : %llx\n", AttachRequest.BaseAddressOfMainModule);
        ShowMessages("Entrypoint Address : %llx\n", AttachRequest.EntrypoinOfMainModule);
        ShowMessages("Is 32-bit : %s\n", AttachRequest.Is32Bit ? "true" : "false");
        Sleep(10000);
        ResumeThread(ProcInfo.hThread);

        //
        // The operation of attaching was successful
        //
        return TRUE;
    }
    else
    {
        ShowErrorMessage(AttachRequest.Result);
        return FALSE;
    }

    return FALSE;
}
