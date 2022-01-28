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
#include "..\hprdbghv\pch.h"

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
