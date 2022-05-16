/**
 * @file objects.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Routines related to objects
 * @details
 * @version 0.1
 * @date 2022-05-06
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;

/**
 * @brief Get details about processes or threads
 * @param IsProcess
 *
 * @return BOOLEAN
 */
BOOLEAN
ObjectShowProcessesOrThreadDetails(BOOLEAN IsProcess)
{
    BOOLEAN                                    Status;
    ULONG                                      ReturnedLength;
    DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PACKET GetInformationProcess = {0};
    DEBUGGEE_DETAILS_AND_SWITCH_THREAD_PACKET  GetInformationThread  = {0};

    if (IsProcess)
    {
        //
        // *** Show process details ***
        //

        //
        // Send the request to the kernel
        //
        Status = DeviceIoControl(
            g_DeviceHandle,                                    // Handle to device
            IOCTL_QUERY_CURRENT_PROCESS,                       // IO Control
                                                               // code
            &GetInformationProcess,                            // Input Buffer to driver.
            SIZEOF_DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PACKET, // Input buffer length
            &GetInformationProcess,                            // Output Buffer from driver.
            SIZEOF_DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PACKET, // Length of output
                                                               // buffer in bytes.
            &ReturnedLength,                                   // Bytes placed in buffer.
            NULL                                               // synchronous call
        );

        if (!Status)
        {
            ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
            return FALSE;
        }

        //
        // Query was successful
        //
        if (GetInformationProcess.Result == DEBUGGER_OPERATION_WAS_SUCCESSFULL)
        {
            ShowMessages("process id: %x\nprocess (_EPROCESS): %s\nprocess name (16-Byte): %s\n",
                         GetInformationProcess.ProcessId,
                         SeparateTo64BitValue(GetInformationProcess.Process).c_str(),
                         GetInformationProcess.ProcessName);

            return TRUE;
        }
        else
        {
            ShowErrorMessage(GetInformationProcess.Result);
            return FALSE;
        }
    }
    else
    {
        //
        // *** Show threads details ***
        //

        //
        // Send the request to the kernel
        //
        Status = DeviceIoControl(
            g_DeviceHandle,                                   // Handle to device
            IOCTL_QUERY_CURRENT_THREAD,                       // IO Control
                                                              // code
            &GetInformationThread,                            // Input Buffer to driver.
            SIZEOF_DEBUGGEE_DETAILS_AND_SWITCH_THREAD_PACKET, // Input buffer length
            &GetInformationThread,                            // Output Buffer from driver.
            SIZEOF_DEBUGGEE_DETAILS_AND_SWITCH_THREAD_PACKET, // Length of output
                                                              // buffer in bytes.
            &ReturnedLength,                                  // Bytes placed in buffer.
            NULL                                              // synchronous call
        );

        if (!Status)
        {
            ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
            return FALSE;
        }

        //
        // Query was successful
        //
        if (GetInformationThread.Result == DEBUGGER_OPERATION_WAS_SUCCESSFULL)
        {
            ShowMessages("thread id: %x (pid: %x)\nthread (_ETHREAD): %s\nprocess (_EPROCESS): %s\nprocess name (16-Byte): %s\n",
                         GetInformationThread.ThreadId,
                         GetInformationThread.ProcessId,
                         SeparateTo64BitValue(GetInformationThread.Thread).c_str(),
                         SeparateTo64BitValue(GetInformationThread.Process).c_str(),
                         GetInformationThread.ProcessName);
            return TRUE;
        }
        else
        {
            ShowErrorMessage(GetInformationThread.Result);
            return FALSE;
        }
    }
}

/**
 * @brief Get details about processes or threads
 * @param IsProcess
 * @param SymDetailsForProcessList
 * @param Eprocess
 * @param SymDetailsForThreadList
 *
 * @return BOOLEAN
 */
BOOLEAN
ObjectShowProcessesOrThreadList(BOOLEAN                               IsProcess,
                                PDEBUGGEE_PROCESS_LIST_NEEDED_DETAILS SymDetailsForProcessList,
                                UINT64                                Eprocess,
                                PDEBUGGEE_THREAD_LIST_NEEDED_DETAILS  SymDetailsForThreadList)
{
    BOOLEAN                                    Status;
    ULONG                                      ReturnedLength;
    DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS QueryCountOfActiveThreadsOrProcessesRequest = {0};
    UINT32                                     SizeOfBufferForThreadsAndProcessDetails     = NULL;
    PVOID                                      Entries                                     = NULL;
    PDEBUGGEE_PROCESS_LIST_DETAILS_ENTRY       ProcessEntries                              = NULL;
    PDEBUGGEE_THREAD_LIST_DETAILS_ENTRY        ThreadEntries                               = NULL;

    //
    // Check if driver is loaded
    //
    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnFalse);

    //
    // We wanna query the count of active processes or threads
    //
    if (IsProcess)
    {
        //
        // It's a process
        //
        QueryCountOfActiveThreadsOrProcessesRequest.QueryType = DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_QUERY_PROCESS_COUNT;
    }
    else
    {
        //
        // It's a thread
        //
        QueryCountOfActiveThreadsOrProcessesRequest.QueryType = DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_QUERY_THREAD_COUNT;
    }

    //
    // The action is counting the process or thread
    //
    QueryCountOfActiveThreadsOrProcessesRequest.QueryAction = DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_ACTION_QUERY_COUNT;

    if (IsProcess)
    {
        //
        // Copy items needed for getting the details of processes
        //
        RtlCopyMemory(&QueryCountOfActiveThreadsOrProcessesRequest.ProcessListNeededDetails,
                      SymDetailsForProcessList,
                      sizeof(DEBUGGEE_PROCESS_LIST_NEEDED_DETAILS));
    }
    else
    {
        //
        // Copy items needed for getting the details of threads
        //
        RtlCopyMemory(&QueryCountOfActiveThreadsOrProcessesRequest.ThreadListNeededDetails,
                      SymDetailsForThreadList,
                      sizeof(DEBUGGEE_THREAD_LIST_NEEDED_DETAILS));
    }

    //
    // Send the request to the kernel
    //
    Status = DeviceIoControl(
        g_DeviceHandle,                                    // Handle to device
        IOCTL_QUERY_COUNT_OF_ACTIVE_PROCESSES_OR_THREADS,  // IO Control
                                                           // code
        &QueryCountOfActiveThreadsOrProcessesRequest,      // Input Buffer to driver.
        SIZEOF_DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS, // Input buffer length
        &QueryCountOfActiveThreadsOrProcessesRequest,      // Output Buffer from driver.
        SIZEOF_DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS, // Length of output
                                                           // buffer in bytes.
        &ReturnedLength,                                   // Bytes placed in buffer.
        NULL                                               // synchronous call
    );

    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        return FALSE;
    }

    //
    // Query was successful
    //
    if (QueryCountOfActiveThreadsOrProcessesRequest.Result == DEBUGGER_OPERATION_WAS_SUCCESSFULL)
    {
        if (QueryCountOfActiveThreadsOrProcessesRequest.Count == 0)
        {
            ShowMessages("err, unable to get count of active processes or threads\n");
        }
        else
        {
            //
            // *** We should send another IOCTL and get the list of processes or threads ***
            //

            //
            // Add some spaces for new processes or threads as new objects might be available
            //
            QueryCountOfActiveThreadsOrProcessesRequest.Count = QueryCountOfActiveThreadsOrProcessesRequest.Count + 5;

            //
            // Allocate the storage for the pull details of threads and processes
            //
            if (IsProcess)
            {
                SizeOfBufferForThreadsAndProcessDetails =
                    QueryCountOfActiveThreadsOrProcessesRequest.Count * sizeof(DEBUGGEE_PROCESS_LIST_DETAILS_ENTRY);
            }
            else
            {
                SizeOfBufferForThreadsAndProcessDetails =
                    QueryCountOfActiveThreadsOrProcessesRequest.Count * sizeof(DEBUGGEE_THREAD_LIST_DETAILS_ENTRY);
            }

            Entries = (PVOID)malloc(SizeOfBufferForThreadsAndProcessDetails);

            RtlZeroMemory(Entries, SizeOfBufferForThreadsAndProcessDetails);

            // ShowMessages("count of active processes/threads : %lld\n", QueryCountOfActiveThreadsOrProcessesRequest.Count);

            if (IsProcess)
            {
                QueryCountOfActiveThreadsOrProcessesRequest.QueryType = DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_QUERY_PROCESS_LIST;
            }
            else
            {
                QueryCountOfActiveThreadsOrProcessesRequest.QueryType = DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_QUERY_THREAD_LIST;
            }

            //
            // Send the request to the kernel
            //
            Status = DeviceIoControl(
                g_DeviceHandle,                                    // Handle to device
                IOCTL_GET_LIST_OF_THREADS_AND_PROCESSES,           // IO Control code
                &QueryCountOfActiveThreadsOrProcessesRequest,      // Input Buffer to driver.
                SIZEOF_DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS, // Input buffer length
                Entries,                                           // Output Buffer from driver.
                SizeOfBufferForThreadsAndProcessDetails,           // Length of output buffer in bytes.
                &ReturnedLength,                                   // Bytes placed in buffer.
                NULL                                               // synchronous call
            );

            if (!Status)
            {
                ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
                return FALSE;
            }

            if (IsProcess)
            {
                ProcessEntries = (PDEBUGGEE_PROCESS_LIST_DETAILS_ENTRY)Entries;
            }
            else
            {
                ThreadEntries = (PDEBUGGEE_THREAD_LIST_DETAILS_ENTRY)Entries;

                ShowMessages("PROCESS\t%llx\tIMAGE\t%s\n",
                             ThreadEntries->Eprocess,
                             ThreadEntries->ImageFileName);
            }

            //
            // Show list of active processes and threads
            //
            for (size_t i = 0; i < QueryCountOfActiveThreadsOrProcessesRequest.Count; i++)
            {
                //
                // Details of process/thread should be shown
                //
                if (IsProcess)
                {
                    if (ProcessEntries[i].Eprocess != NULL)
                    {
                        ShowMessages("PROCESS\t%llx\n\tProcess Id: %04x\tDirBase (Kernel Cr3): %016llx\tImage: %s\n\n",
                                     ProcessEntries[i].Eprocess,
                                     ProcessEntries[i].Pid,
                                     ProcessEntries[i].Cr3,
                                     ProcessEntries[i].ImageFileName);
                    }
                }
                else
                {
                    if (ThreadEntries[i].Ethread != NULL)
                    {
                        ShowMessages("\tTHREAD\t%llx (%llx.%llx)\n",
                                     ThreadEntries[i].Ethread,
                                     ThreadEntries[i].Pid,
                                     ThreadEntries[i].Tid);
                    }
                }
            }
        }

        //
        // The operation of attaching was successful
        //
        return TRUE;
    }
    else
    {
        ShowErrorMessage(QueryCountOfActiveThreadsOrProcessesRequest.Result);
        return FALSE;
    }

    //
    // No reason to reach here
    //
    return FALSE;
}
