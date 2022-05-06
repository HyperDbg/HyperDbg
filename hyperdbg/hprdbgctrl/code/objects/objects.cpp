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
 * @param ActionType
 * @param NewPid
 * @param NewProcess
 * @param SetChangeByClockInterrupt
 * @param SymDetailsForProcessList
 *
 * @return BOOLEAN
 */
BOOLEAN
ObjectShowProcessesOrThreadDetails(DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_TYPE ActionType,
                                   UINT32                                   NewPid,
                                   UINT64                                   NewProcess,
                                   BOOLEAN                                  SetChangeByClockInterrupt,
                                   PDEBUGGEE_PROCESS_LIST_NEEDED_DETAILS    SymDetailsForProcessList)
{
    BOOLEAN                                      Status;
    ULONG                                        ReturnedLength;
    DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS   QueryCountOfActiveThreadsOrProcessesRequest = {0};
    UINT32                                       SizeOfBufferForThreadsAndProcessDetails     = NULL;
    DEBUGGER_ACTIVE_PROCESS_OR_THREADS_DETAILS * ThreadsOrProcessDetails                     = NULL;

    //
    // Only support get the current process and list of processes as it's called in the VMI mode
    //
    if (ActionType != DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_GET_PROCESS_DETAILS &&
        ActionType != DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_GET_PROCESS_LIST)
    {
        ShowMessages("err, you're only allowed to get the current process and "
                     "list of processes in the VMI mode");
        return FALSE;
    }

    //
    // Check if driver is loaded
    //
    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnFalse);

    //
    // We wanna query the count of active processes or threads
    //
    QueryCountOfActiveThreadsOrProcessesRequest.QueryType = DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_QUERY_PROCESS_COUNT;

    //
    // The action is counting the process or thread
    //
    QueryCountOfActiveThreadsOrProcessesRequest.QueryAction = DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_ACTION_QUERY_COUNT;

    //
    // Copy items needed for getting the details of processes or threads
    //
    RtlCopyMemory(&QueryCountOfActiveThreadsOrProcessesRequest.ProcessListNeededDetails,
                  SymDetailsForProcessList,
                  sizeof(DEBUGGEE_PROCESS_LIST_NEEDED_DETAILS));

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
            SizeOfBufferForThreadsAndProcessDetails =
                QueryCountOfActiveThreadsOrProcessesRequest.Count * SIZEOF_DEBUGGER_ACTIVE_PROCESS_OR_THREADS_DETAILS;

            ThreadsOrProcessDetails = (DEBUGGER_ACTIVE_PROCESS_OR_THREADS_DETAILS *)malloc(SizeOfBufferForThreadsAndProcessDetails);

            RtlZeroMemory(ThreadsOrProcessDetails, SizeOfBufferForThreadsAndProcessDetails);

            ShowMessages("count of active processes : %llx\n", QueryCountOfActiveThreadsOrProcessesRequest.Count);
            return TRUE;

            //
            // Send the request to the kernel
            //
            Status = DeviceIoControl(
                g_DeviceHandle,                          // Handle to device
                IOCTL_GET_LIST_OF_THREADS_AND_PROCESSES, // IO Control
                                                         // code
                NULL,                                    // Input Buffer to driver.
                0,                                       // Input buffer length.
                ThreadsOrProcessDetails,                 // Output Buffer from driver.
                SizeOfBufferForThreadsAndProcessDetails, // Length of output buffer in bytes.
                &ReturnedLength,                         // Bytes placed in buffer.
                NULL                                     // synchronous call
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
