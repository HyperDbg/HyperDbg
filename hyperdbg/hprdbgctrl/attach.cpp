/**
 * @file attach.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief .attach command
 * @details
 * @version 0.1
 * @date 2020-08-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern DEBUGGING_STATE g_DebuggingState;

/**
 * @brief print error messages relating to the finding thread id
 * @details this function is used only in the scope of Thread32First
 */
void
PrintError()
{
    DWORD   eNum;
    TCHAR   sysMsg[256];
    TCHAR * p;

    eNum = GetLastError();
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  eNum,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                  sysMsg,
                  256,
                  NULL);

    //
    // Trim the end of the line and terminate it with a null
    //
    p = sysMsg;
    while ((*p > 31) || (*p == 9))
        ++p;
    do
    {
        *p-- = 0;
    } while ((p >= sysMsg) && ((*p == '.') || (*p < 33)));

    //
    // Display the message
    //
    ShowMessages("\n  WARNING: Thread32First failed with error %d (%s)", eNum, sysMsg);
}

/**
 * @brief List of threads by owner process id
 *
 * @param dwOwnerPID
 * @return BOOL if there was an error then returns false, otherwise return true
 */
BOOL
ListProcessThreads(DWORD dwOwnerPID)
{
    HANDLE        hThreadSnap = INVALID_HANDLE_VALUE;
    THREADENTRY32 te32;

    //
    // Take a snapshot of all running threads
    //
    hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnap == INVALID_HANDLE_VALUE)
        return FALSE;

    //
    // Fill in the size of the structure before using it
    //
    te32.dwSize = sizeof(THREADENTRY32);

    //
    // Retrieve information about the first thread and exit if unsuccessful
    //
    if (!Thread32First(hThreadSnap, &te32))
    {
        PrintError();             // Show cause of failure
        CloseHandle(hThreadSnap); // Must clean up the snapshot object!
        return FALSE;
    }
    ShowMessages("\nThread's of pid\t= 0x%08X\n", dwOwnerPID);

    //
    // Now walk the thread list of the system, and display information
    // about each thread associated with the specified process
    //
    do
    {
        if (te32.th32OwnerProcessID == dwOwnerPID)
        {
            ShowMessages("\n     Thread Id\t= 0x%08X", te32.th32ThreadID);
            // ShowMessages("\n     base priority  = %d", te32.tpBasePri);
            // ShowMessages("\n     delta priority = %d", te32.tpDeltaPri);
        }
    } while (Thread32Next(hThreadSnap, &te32));

    ShowMessages("\n");

    //
    // Don't forget to clean up the snapshot object
    //
    CloseHandle(hThreadSnap);
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
CheckThreadByProcessId(DWORD Pid, DWORD Tid)
{
    HANDLE        hThreadSnap = INVALID_HANDLE_VALUE;
    THREADENTRY32 te32;
    BOOLEAN       Result = FALSE;

    //
    // Take a snapshot of all running threads
    //
    hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnap == INVALID_HANDLE_VALUE)
        return FALSE;

    //
    // Fill in the size of the structure before using it
    //
    te32.dwSize = sizeof(THREADENTRY32);

    //
    // Retrieve information about the first thread and exit if unsuccessful
    //
    if (!Thread32First(hThreadSnap, &te32))
    {
        PrintError();             // Show cause of failure
        CloseHandle(hThreadSnap); // Must clean up the snapshot object!
        return FALSE;
    }

    //
    // Now walk the thread list of the system, and display information
    // about each thread associated with the specified process
    //
    do
    {
        if (te32.th32OwnerProcessID == Pid)
        {
            if (te32.th32ThreadID == Tid)
            {
                //
                // The thread found in target process
                //
                Result = TRUE;
                break;
            }
        }
    } while (Thread32Next(hThreadSnap, &te32));

    //
    // Don't forget to clean up the snapshot object
    //
    CloseHandle(hThreadSnap);
    return Result;
}

/**
 * @brief help of .attach command
 *
 * @return VOID
 */
VOID
CommandAttachHelp()
{
    ShowMessages(".attach : attach to debug a user-mode process.\n\n");
    ShowMessages(
        "syntax : \t.attach pid [process id (hex)] tid [thread id (hex)]\n");
    ShowMessages("Note : if you don't specify the thread id (id), then it shows "
                 "the list of active threads on the target process (it won't "
                 "attach to the target thread).\n");
    ShowMessages("\t\te.g : .attach pid b60 \n");
    ShowMessages("\t\te.g : .attach pid b60 tid 220 \n");
}

/**
 * @brief Attach to target process
 * @details this function will not check whether the process id and
 * thread id is valid or not
 *
 * @param TargetPid
 * @param TargetTid
 * @return VOID
 */
VOID
AttachToProcess(UINT32 TargetPid, UINT32 TargetTid)
{
    BOOLEAN                                  Status;
    ULONG                                    ReturnedLength;
    DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS AttachRequest = {0};

    ShowMessages("This command is not supported on this version.\n");
    return;

    //
    // Check if debugger is loaded or not
    //
    if (!g_DeviceHandle)
    {
        ShowMessages("Handle not found, probably the driver is not loaded. Did you "
                     "use 'load' command?\n");
        return;
    }

    //
    // We wanna attach to a remote process
    //
    AttachRequest.IsAttach = TRUE;

    //
    // Set the process id and thread id
    //
    AttachRequest.ProcessId = TargetPid;
    AttachRequest.ThreadId  = TargetTid;

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
        return;
    }

    //
    // Check if attaching was successful then we can set the attached to true
    //
    if (AttachRequest.Result == DEBUGEER_OPERATION_WAS_SUCCESSFULL)
    {
        g_DebuggingState.IsAttachedToUsermodeProcess = TRUE;
        g_DebuggingState.ConnectedProcessId          = TargetPid;
        g_DebuggingState.ConnectedThreadId           = TargetTid;
    }
}

/**
 * @brief .attach command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandAttach(vector<string> SplittedCommand, string Command)
{
    UINT64  TargetPid = 0;
    UINT64  TargetTid = 0;
    BOOLEAN NextIsPid = FALSE;
    BOOLEAN NextIsTid = FALSE;

    if (SplittedCommand.size() >= 6)
    {
        ShowMessages("incorrect use of '.attach'\n\n");
        CommandAttachHelp();
        return;
    }

    for (auto item : SplittedCommand)
    {
        //
        // Find out whether the user enters pid or not
        //
        if (NextIsPid)
        {
            NextIsPid = FALSE;

            if (!ConvertStringToUInt64(item, &TargetPid))
            {
                ShowMessages("please specify a correct hex value for process id\n\n");
                CommandAttachHelp();
                return;
            }
        }
        else if (NextIsTid)
        {
            NextIsTid = FALSE;

            if (!ConvertStringToUInt64(item, &TargetTid))
            {
                ShowMessages("please specify a correct hex value for thread id\n\n");
                CommandAttachHelp();
                return;
            }
        }
        else if (!item.compare("pid"))
        {
            //
            // next item is a pid for the process
            //
            NextIsPid = TRUE;
        }
        else if (!item.compare("tid"))
        {
            //
            // next item is a tid for the thread
            //
            NextIsTid = TRUE;
        }
    }

    //
    // Check if the process id is empty or not
    //
    if (TargetPid == 0)
    {
        ShowMessages("please specify a hex value for process id\n\n");
        CommandAttachHelp();
        return;
    }

    //
    // Check if the thread id is specified or not, if not then
    // we should just show the thread of the target process
    //
    if (TargetTid == 0)
    {
        ListProcessThreads(TargetPid);
        return;
    }
    else
    {
        //
        // Check if the process id and thread id is correct or not
        //
        if (!CheckThreadByProcessId(TargetPid, TargetTid))
        {
            ShowMessages("err, the thread or the process not found or the thread not "
                         "belongs to the process.\n");
            return;
        }
    }

    //
    // Perform attach to target process
    //
    AttachToProcess(TargetPid, TargetTid);
}
