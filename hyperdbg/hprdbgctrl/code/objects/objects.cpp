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

/*

   ShowMessages("thread id: %x (pid: %x)\nthread (_ETHREAD): %s\nprocess (_EPROCESS): %s\nprocess name (16-Byte): %s\n",
                ChangeThreadPacket->ThreadId,
                ChangeThreadPacket->ProcessId,
                SeparateTo64BitValue(ChangeThreadPacket->Thread).c_str(),
                SeparateTo64BitValue(ChangeThreadPacket->Process).c_str(),
                &ChangeThreadPacket->ProcessName);


   ShowMessages("process id: %x\nprocess (_EPROCESS): %s\nprocess name (16-Byte): %s\n",
                ChangeProcessPacket->ProcessId,
                SeparateTo64BitValue(ChangeProcessPacket->Process).c_str(),
                &ChangeProcessPacket->ProcessName);

*/

/**
 * @brief Get details about processes or threads
 * @param IsProcess
 * @param IsOnlyTheCurrentDetails
 * @param SymDetailsForProcessList
 * @param Eprocess
 * @param SymDetailsForThreadList
 *
 * @return BOOLEAN
 */
BOOLEAN
ObjectShowProcessesOrThreadDetails(BOOLEAN                               IsProcess,
                                   BOOLEAN                               IsOnlyTheCurrentDetails,
                                   PDEBUGGEE_PROCESS_LIST_NEEDED_DETAILS SymDetailsForProcessList,
                                   UINT64                                Eprocess,
                                   PDEBUGGEE_THREAD_LIST_NEEDED_DETAILS  SymDetailsForThreadList)
{
    return TRUE;
}

/**
 * @brief Get details about processes or threads
 * @param IsProcess
 * @param IsOnlyTheCurrentDetails
 * @param SymDetailsForProcessList
 * @param Eprocess
 * @param SymDetailsForThreadList
 *
 * @return BOOLEAN
 */
BOOLEAN
ObjectShowProcessesOrThreadList(BOOLEAN                               IsProcess,
                                BOOLEAN                               IsOnlyTheCurrentDetails,
                                PDEBUGGEE_PROCESS_LIST_NEEDED_DETAILS SymDetailsForProcessList,
                                UINT64                                Eprocess,
                                PDEBUGGEE_THREAD_LIST_NEEDED_DETAILS  SymDetailsForThreadList)
{
    BOOLEAN                                    Status;
    ULONG                                      ReturnedLength;
    DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS QueryCountOfActiveThreadsOrProcessesRequest = {0};
    UINT32                                     SizeOfBufferForThreadsAndProcessDetails     = NULL;
    PDEBUGGEE_PROCESS_LIST_DETAILS_ENTRY       ProcessEntries;

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
                //
                // To be filled !
                //
            }

            ProcessEntries = (DEBUGGEE_PROCESS_LIST_DETAILS_ENTRY *)malloc(SizeOfBufferForThreadsAndProcessDetails);

            RtlZeroMemory(ProcessEntries, SizeOfBufferForThreadsAndProcessDetails);

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
                ProcessEntries,                                    // Output Buffer from driver.
                SizeOfBufferForThreadsAndProcessDetails,           // Length of output buffer in bytes.
                &ReturnedLength,                                   // Bytes placed in buffer.
                NULL                                               // synchronous call
            );

            if (!Status)
            {
                ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
                return FALSE;
            }

            //
            // Show list of active processes and threads
            //
            for (size_t i = 0; i < QueryCountOfActiveThreadsOrProcessesRequest.Count; i++)
            {
                //
                // Details of process should be shown
                //
                if (ProcessEntries[i].Eprocess != NULL)
                {
                    ShowMessages("PROCESS\t%llx\n\tProcess Id: %04x\tDirBase (Kernel Cr3): %016llx\tImage: %s\n\n",
                                 ProcessEntries[i].Eprocess,
                                 ProcessEntries[i].Pid,
                                 ProcessEntries[i].Cr3,
                                 ProcessEntries[i].ImageFileName);
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
