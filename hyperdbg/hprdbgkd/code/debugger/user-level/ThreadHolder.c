/**
 * @file ThreadHolder.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief User debugger's thread holder
 * @details 
 *
 * @version 0.1
 * @date 2022-01-28
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "pch.h"

/**
 * @brief Pre allocate buffer for thread holder 
 * 
 * @return VOID 
 */
VOID
ThreadHolderAllocateThreadHoldingBuffers()
{
    //
    // Request to allocate two buffer for holder of threads
    //
    PoolManagerRequestAllocation(sizeof(USERMODE_DEBUGGING_THREAD_HOLDER), 2, PROCESS_THREAD_HOLDER);

    //
    // As it might be called from an attaching request and never find a
    // a chance to allocate it, we allocate it here as it's safe at PASSIVE_LEVEL
    //
    PoolManagerCheckAndPerformAllocationAndDeallocation();
}

/**
 * @brief Assign a thread holder to process debugging details 
 * 
 * @param ProcessDebuggingDetail 
 * @return BOOLEAN 
 */
BOOLEAN
ThreadHolderAssignThreadHolderToProcessDebuggingDetails(PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetail)
{
    PUSERMODE_DEBUGGING_THREAD_HOLDER ThreadHolder;

    //
    // Initialize the list entry of threads
    //
    InitializeListHead(&ProcessDebuggingDetail->ThreadsListHead);

    //
    // Add a thread holder here
    //
    ThreadHolder = (USERMODE_DEBUGGING_THREAD_HOLDER *)
        PoolManagerRequestPool(PROCESS_THREAD_HOLDER, TRUE, sizeof(USERMODE_DEBUGGING_THREAD_HOLDER));

    if (!ThreadHolder)
    {
        return FALSE;
    }

    //
    // Add it to thread holder of the structure
    //
    InsertHeadList(&ProcessDebuggingDetail->ThreadsListHead, &(ThreadHolder->ThreadHolderList));

    return TRUE;
}

/**
 * @brief Check if there is any thread paused by checking process debugging details 
 * 
 * @param ProcessDebuggingDetail 
 * @return BOOLEAN 
 */
BOOLEAN
ThreadHolderIsAnyPausedThreadInProcess(PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetail)
{
    PLIST_ENTRY TempList = 0;

    TempList = &ProcessDebuggingDetail->ThreadsListHead;

    while (&ProcessDebuggingDetail->ThreadsListHead != TempList->Flink)
    {
        TempList = TempList->Flink;
        PUSERMODE_DEBUGGING_THREAD_HOLDER ThreadHolder =
            CONTAINING_RECORD(TempList, USERMODE_DEBUGGING_THREAD_HOLDER, ThreadHolderList);

        for (size_t i = 0; i < MAX_THREADS_IN_A_PROCESS_HOLDER; i++)
        {
            if (ThreadHolder->Threads[i].ThreadId != NULL && ThreadHolder->Threads[i].IsPaused)
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

/**
 * @brief Find the active threads of the process from process id
 * 
 * @param ProcessId 
 * @param ThreadId 
 * @return PUSERMODE_DEBUGGING_THREAD_DETAILS 
 */
PUSERMODE_DEBUGGING_THREAD_DETAILS
ThreadHolderGetProcessThreadDetailsByProcessIdAndThreadId(UINT32 ProcessId, UINT32 ThreadId)
{
    PLIST_ENTRY                         TempList = 0;
    PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetail;

    //
    // First, find the process details
    //
    ProcessDebuggingDetail = AttachingFindProcessDebuggingDetailsByProcessId(ProcessId);

    if (ProcessDebuggingDetail == NULL)
    {
        return NULL;
    }

    TempList = &ProcessDebuggingDetail->ThreadsListHead;

    while (&ProcessDebuggingDetail->ThreadsListHead != TempList->Flink)
    {
        TempList = TempList->Flink;
        PUSERMODE_DEBUGGING_THREAD_HOLDER ThreadHolder =
            CONTAINING_RECORD(TempList, USERMODE_DEBUGGING_THREAD_HOLDER, ThreadHolderList);

        for (size_t i = 0; i < MAX_THREADS_IN_A_PROCESS_HOLDER; i++)
        {
            if (ThreadHolder->Threads[i].ThreadId == ThreadId)
            {
                //
                // The active thread's structure is found
                //
                return &ThreadHolder->Threads[i];
            }
        }
    }

    //
    // Active thread not found
    //
    return NULL;
}

/**
 * @brief Find the first active threads of the process from process id
 * 
 * @param ProcessId 
 * @return PUSERMODE_DEBUGGING_THREAD_DETAILS 
 */
PUSERMODE_DEBUGGING_THREAD_DETAILS
ThreadHolderGetProcessFirstThreadDetailsByProcessId(UINT32 ProcessId)
{
    PLIST_ENTRY                         TempList = 0;
    PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetail;

    //
    // First, find the process details
    //
    ProcessDebuggingDetail = AttachingFindProcessDebuggingDetailsByProcessId(ProcessId);

    if (ProcessDebuggingDetail == NULL)
    {
        return NULL;
    }

    TempList = &ProcessDebuggingDetail->ThreadsListHead;

    while (&ProcessDebuggingDetail->ThreadsListHead != TempList->Flink)
    {
        TempList = TempList->Flink;
        PUSERMODE_DEBUGGING_THREAD_HOLDER ThreadHolder =
            CONTAINING_RECORD(TempList, USERMODE_DEBUGGING_THREAD_HOLDER, ThreadHolderList);

        for (size_t i = 0; i < MAX_THREADS_IN_A_PROCESS_HOLDER; i++)
        {
            if (ThreadHolder->Threads[i].ThreadId != NULL)
            {
                //
                // The active thread's structure is found
                //
                return &ThreadHolder->Threads[i];
            }
        }
    }

    //
    // Active thread not found
    //
    return NULL;
}

/**
 * @brief Find the active process debugging detail from the thread id
 * 
 * @param ThreadId 
 * @return PUSERMODE_DEBUGGING_PROCESS_DETAILS 
 */
PUSERMODE_DEBUGGING_PROCESS_DETAILS
ThreadHolderGetProcessDebuggingDetailsByThreadId(UINT32 ThreadId)
{
    PLIST_ENTRY TempList  = 0;
    PLIST_ENTRY TempList2 = 0;

    TempList = &g_ProcessDebuggingDetailsListHead;

    while (&g_ProcessDebuggingDetailsListHead != TempList->Flink)
    {
        TempList = TempList->Flink;
        PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetails =
            CONTAINING_RECORD(TempList, USERMODE_DEBUGGING_PROCESS_DETAILS, AttachedProcessList);

        //
        // Search through all the active threads of this process
        //
        TempList2 = &ProcessDebuggingDetails->ThreadsListHead;

        while (&ProcessDebuggingDetails->ThreadsListHead != TempList2->Flink)
        {
            TempList2 = TempList2->Flink;
            PUSERMODE_DEBUGGING_THREAD_HOLDER ThreadHolder =
                CONTAINING_RECORD(TempList2, USERMODE_DEBUGGING_THREAD_HOLDER, ThreadHolderList);

            for (size_t i = 0; i < MAX_THREADS_IN_A_PROCESS_HOLDER; i++)
            {
                if (ThreadHolder->Threads[i].ThreadId == ThreadId)
                {
                    //
                    // The target thread is found, not let's return the process debugging
                    // details of this process
                    //
                    return ProcessDebuggingDetails;
                }
            }
        }
    }

    //
    // Active thread not found
    //
    return NULL;
}

/**
 * @brief Find or create user-mode debugging details for threads
 * 
 * @param ThreadId 
 * @param ProcessDebuggingDetail 
 * @return PUSERMODE_DEBUGGING_THREAD_DETAILS 
 */
PUSERMODE_DEBUGGING_THREAD_DETAILS
ThreadHolderFindOrCreateThreadDebuggingDetail(UINT32 ThreadId, PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetail)
{
    PLIST_ENTRY TempList = 0;

    TempList = &ProcessDebuggingDetail->ThreadsListHead;

    //
    // Let's see if we can find the thread
    //
    while (&ProcessDebuggingDetail->ThreadsListHead != TempList->Flink)
    {
        TempList = TempList->Flink;
        PUSERMODE_DEBUGGING_THREAD_HOLDER ThreadHolder =
            CONTAINING_RECORD(TempList, USERMODE_DEBUGGING_THREAD_HOLDER, ThreadHolderList);

        for (size_t i = 0; i < MAX_THREADS_IN_A_PROCESS_HOLDER; i++)
        {
            if (ThreadHolder->Threads[i].ThreadId == ThreadId)
            {
                //
                // We find a thread, let's return it's structure
                //
                return &ThreadHolder->Threads[i];
            }
        }
    }

    //
    // *** We're here, the thread is not found, let's create an entry for it ***
    //

    //
    // Two threads should not simultaneously reach here
    //
    SpinlockLock(&VmxRootThreadHoldingLock);

    TempList = &ProcessDebuggingDetail->ThreadsListHead;

    //
    // Let's see if we can find the thread
    //
    while (&ProcessDebuggingDetail->ThreadsListHead != TempList->Flink)
    {
        TempList = TempList->Flink;
        PUSERMODE_DEBUGGING_THREAD_HOLDER ThreadHolder =
            CONTAINING_RECORD(TempList, USERMODE_DEBUGGING_THREAD_HOLDER, ThreadHolderList);

        for (size_t i = 0; i < MAX_THREADS_IN_A_PROCESS_HOLDER; i++)
        {
            if (ThreadHolder->Threads[i].ThreadId == NULL)
            {
                //
                // We find a null thread place, let's return it's structure
                //
                ThreadHolder->Threads[i].ThreadId = ThreadId;

                SpinlockUnlock(&VmxRootThreadHoldingLock);
                return &ThreadHolder->Threads[i];
            }
        }
    }

    //
    // We didn't find an empty entry,
    // let's use another structure and link it to the thread holder
    //
    PUSERMODE_DEBUGGING_THREAD_HOLDER NewThreadHolder = (USERMODE_DEBUGGING_THREAD_HOLDER *)
        PoolManagerRequestPool(PROCESS_THREAD_HOLDER, TRUE, sizeof(USERMODE_DEBUGGING_THREAD_HOLDER));

    if (NewThreadHolder == NULL)
    {
        LogError("Err, enable to find a place to save the threads data, "
                 "please use 'prealloc' command to allocate more pre-allocated "
                 "buffer for the thread holder");

        SpinlockUnlock(&VmxRootThreadHoldingLock);
        return NULL;
    }

    //
    // Add the current thread as the first entry of the holder
    //
    NewThreadHolder->Threads[0].ThreadId = ThreadId;

    //
    // Link to the thread holding structure
    //
    InsertHeadList(&ProcessDebuggingDetail->ThreadsListHead, &(NewThreadHolder->ThreadHolderList));

    //
    // Other threads are now allowed to use the thread listing mechanism
    //
    SpinlockUnlock(&VmxRootThreadHoldingLock);

    return &NewThreadHolder->Threads[0];
}

/**
 * @brief Apply the action of the user debugger to a specific thread or 
 * all threads
 * 
 * @param ProcessDebuggingDetails 
 * @param ActionRequest 
 *
 * @return BOOLEAN Shows whether the command is applied or not 
 */
BOOLEAN
ThreadHolderApplyActionToPausedThreads(PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetails,
                                       PDEBUGGER_UD_COMMAND_PACKET         ActionRequest)
{
    PUSERMODE_DEBUGGING_THREAD_DETAILS ThreadDebuggingDetails;
    BOOLEAN                            CommandApplied = FALSE;

    if (!ActionRequest->ApplyToAllPausedThreads)
    {
        //
        // *** only the active thread ***
        //

        ThreadDebuggingDetails = ThreadHolderGetProcessThreadDetailsByProcessIdAndThreadId(ProcessDebuggingDetails->ProcessId,
                                                                                           ActionRequest->TargetThreadId);

        //
        // Apply the command
        //
        for (size_t i = 0; i < MAX_USER_ACTIONS_FOR_THREADS; i++)
        {
            if (ThreadDebuggingDetails->UdAction[i].ActionType == DEBUGGER_UD_COMMAND_ACTION_TYPE_NONE)
            {
                //
                // Set the action
                //
                ThreadDebuggingDetails->UdAction[i].OptionalParam1 = ActionRequest->UdAction.OptionalParam1;
                ThreadDebuggingDetails->UdAction[i].OptionalParam2 = ActionRequest->UdAction.OptionalParam2;
                ThreadDebuggingDetails->UdAction[i].OptionalParam3 = ActionRequest->UdAction.OptionalParam3;
                ThreadDebuggingDetails->UdAction[i].OptionalParam4 = ActionRequest->UdAction.OptionalParam4;

                //
                // At last we set the action type to make it valid
                //
                ThreadDebuggingDetails->UdAction[i].ActionType = ActionRequest->UdAction.ActionType;

                //
                // Command is applied
                //
                return TRUE;
            }
        }
    }
    else
    {
        //
        // *** apply to all paused threads ***
        //
        PLIST_ENTRY TempList = 0;

        TempList = &ProcessDebuggingDetails->ThreadsListHead;

        while (&ProcessDebuggingDetails->ThreadsListHead != TempList->Flink)
        {
            TempList = TempList->Flink;
            PUSERMODE_DEBUGGING_THREAD_HOLDER ThreadHolder =
                CONTAINING_RECORD(TempList, USERMODE_DEBUGGING_THREAD_HOLDER, ThreadHolderList);

            for (size_t i = 0; i < MAX_THREADS_IN_A_PROCESS_HOLDER; i++)
            {
                if (ThreadHolder->Threads[i].ThreadId != NULL &&
                    ThreadHolder->Threads[i].IsPaused)
                {
                    for (size_t j = 0; j < MAX_USER_ACTIONS_FOR_THREADS; j++)
                    {
                        if (ThreadHolder->Threads[i].UdAction[j].ActionType == DEBUGGER_UD_COMMAND_ACTION_TYPE_NONE)
                        {
                            //
                            // Set the action
                            //
                            ThreadHolder->Threads[i].UdAction[j].OptionalParam1 = ActionRequest->UdAction.OptionalParam1;
                            ThreadHolder->Threads[i].UdAction[j].OptionalParam2 = ActionRequest->UdAction.OptionalParam2;
                            ThreadHolder->Threads[i].UdAction[j].OptionalParam3 = ActionRequest->UdAction.OptionalParam3;
                            ThreadHolder->Threads[i].UdAction[j].OptionalParam4 = ActionRequest->UdAction.OptionalParam4;

                            //
                            // At last we set the action type to make it valid
                            //
                            ThreadHolder->Threads[i].UdAction[j].ActionType = ActionRequest->UdAction.ActionType;

                            CommandApplied = TRUE;
                            break;
                        }
                    }
                }
            }
        }
    }

    //
    // Return the result of applying the command
    //
    return CommandApplied;
}

/**
 * @brief Free all of thread holder structures 
 * 
 * @param ProcessDebuggingDetail 
 * @return VOID 
 */
VOID
ThreadHolderFreeHoldingStructures(PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetail)
{
    PLIST_ENTRY TempList = 0;

    TempList = &ProcessDebuggingDetail->ThreadsListHead;

    while (&ProcessDebuggingDetail->ThreadsListHead != TempList->Flink)
    {
        TempList = TempList->Flink;
        PUSERMODE_DEBUGGING_THREAD_HOLDER ThreadHolder =
            CONTAINING_RECORD(TempList, USERMODE_DEBUGGING_THREAD_HOLDER, ThreadHolderList);

        //
        // The thread is allocated from the pool management, so we'll
        // free it from there
        //
        PoolManagerFreePool(ThreadHolder);
    }
}

/**
 * @brief Query count of active debugging threads and processes
 * 
 * @return UINT32 
 */
UINT32
ThreadHolderQueryCountOfActiveDebuggingThreadsAndProcesses()
{
    PLIST_ENTRY TempList                   = 0;
    PLIST_ENTRY TempList2                  = 0;
    UINT32      CountOfThreadsAndProcesses = 0;

    TempList = &g_ProcessDebuggingDetailsListHead;

    while (&g_ProcessDebuggingDetailsListHead != TempList->Flink)
    {
        TempList = TempList->Flink;
        PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetails =
            CONTAINING_RECORD(TempList, USERMODE_DEBUGGING_PROCESS_DETAILS, AttachedProcessList);

        //
        // Each process is also counted (no matter if it has active paused thread or not)
        //
        CountOfThreadsAndProcesses++;

        //
        // Search through all the active threads of this process
        //
        TempList2 = &ProcessDebuggingDetails->ThreadsListHead;

        while (&ProcessDebuggingDetails->ThreadsListHead != TempList2->Flink)
        {
            TempList2 = TempList2->Flink;
            PUSERMODE_DEBUGGING_THREAD_HOLDER ThreadHolder =
                CONTAINING_RECORD(TempList2, USERMODE_DEBUGGING_THREAD_HOLDER, ThreadHolderList);

            for (size_t i = 0; i < MAX_THREADS_IN_A_PROCESS_HOLDER; i++)
            {
                if (ThreadHolder->Threads[i].IsPaused)
                {
                    //
                    // A paused thread should be counted
                    //
                    CountOfThreadsAndProcesses++;
                }
            }
        }
    }

    //
    // Return count of active threads and processes
    //
    return CountOfThreadsAndProcesses;
}

/**
 * @brief Query details of active debugging threads and processes
 * 
 * @param BufferToStoreDetails
 * @param MaxCount
 * 
 * @return VOID 
 */
VOID
ThreadHolderQueryDetailsOfActiveDebuggingThreadsAndProcesses(
    USERMODE_DEBUGGING_THREAD_OR_PROCESS_STATE_DETAILS * BufferToStoreDetails,
    UINT32                                               MaxCount)
{
    PLIST_ENTRY TempList     = 0;
    PLIST_ENTRY TempList2    = 0;
    UINT32      CurrentIndex = 0;

    if (MaxCount == 0)
    {
        //
        // Invalid query, storage is empty
        //
        return;
    }

    TempList = &g_ProcessDebuggingDetailsListHead;

    while (&g_ProcessDebuggingDetailsListHead != TempList->Flink)
    {
        TempList = TempList->Flink;
        PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetails =
            CONTAINING_RECORD(TempList, USERMODE_DEBUGGING_PROCESS_DETAILS, AttachedProcessList);

        //
        // Each process is also save (no matter if it has active paused thread or not)
        //
        BufferToStoreDetails[CurrentIndex].IsProcess = TRUE;
        BufferToStoreDetails[CurrentIndex].ProcessId = ProcessDebuggingDetails->ProcessId;
        CurrentIndex++;
        if (MaxCount == CurrentIndex)
        {
            //
            // Storage is full
            //
            return;
        }

        //
        // Search through all the active threads of this process
        //
        TempList2 = &ProcessDebuggingDetails->ThreadsListHead;

        while (&ProcessDebuggingDetails->ThreadsListHead != TempList2->Flink)
        {
            TempList2 = TempList2->Flink;
            PUSERMODE_DEBUGGING_THREAD_HOLDER ThreadHolder =
                CONTAINING_RECORD(TempList2, USERMODE_DEBUGGING_THREAD_HOLDER, ThreadHolderList);

            for (size_t i = 0; i < MAX_THREADS_IN_A_PROCESS_HOLDER; i++)
            {
                if (ThreadHolder->Threads[i].IsPaused)
                {
                    //
                    // A paused thread should be saved
                    //
                    BufferToStoreDetails[CurrentIndex].IsProcess = FALSE;
                    BufferToStoreDetails[CurrentIndex].ProcessId = ProcessDebuggingDetails->ProcessId;
                    BufferToStoreDetails[CurrentIndex].ThreadId  = ThreadHolder->Threads[i].ThreadId;
                    CurrentIndex++;
                    if (MaxCount == CurrentIndex)
                    {
                        //
                        // Storage is full
                        //
                        return;
                    }
                }
            }
        }
    }
}
