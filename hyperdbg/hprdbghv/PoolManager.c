/**
 * @file PoolManager.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief The pool manager used in vmx root
 * @details As we cannot allocate pools in vmx root, we need a pool
 * manager to manage the pools
 * 
 * @version 0.1
 * @date 2020-04-11
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "pch.h"

/**
 * @brief Initializes the pool manager
 * 
 * @return BOOLEAN 
 */
BOOLEAN
PoolManagerInitialize()
{
    //
    // Allocate global requesting variable
    //
    g_RequestNewAllocation = ExAllocatePoolWithTag(NonPagedPool, MaximumRequestsQueueDepth * sizeof(REQUEST_NEW_ALLOCATION), POOLTAG);

    if (!g_RequestNewAllocation)
    {
        LogError("Insufficient memory");
        return FALSE;
    }
    RtlZeroMemory(g_RequestNewAllocation, MaximumRequestsQueueDepth * sizeof(REQUEST_NEW_ALLOCATION));

    //
    // Initialize list head
    //
    InitializeListHead(&g_ListOfAllocatedPoolsHead);

    //
    // Request pages to be allocated for converting 2MB to 4KB pages
    //
    PoolManagerRequestAllocation(sizeof(VMM_EPT_DYNAMIC_SPLIT), 5, SPLIT_2MB_PAGING_TO_4KB_PAGE);

    //
    // Request pages to be allocated for paged hook details
    //
    PoolManagerRequestAllocation(sizeof(EPT_HOOKED_PAGE_DETAIL), 5, TRACKING_HOOKED_PAGES);

    //
    // Request pages to be allocated for Trampoline of Executable hooked pages
    //
    PoolManagerRequestAllocation(MAX_EXEC_TRAMPOLINE_SIZE, 5, EXEC_TRAMPOLINE);

    //
    // Request pages to be allocated for detour hooked pages details
    //
    PoolManagerRequestAllocation(sizeof(HIDDEN_HOOKS_DETOUR_DETAILS), 5, DETOUR_HOOK_DETAILS);

    //
    // Request pages for thread steppings detail
    //
    PoolManagerRequestAllocation(sizeof(DEBUGGER_STEPPING_THREAD_DETAILS), 2, THREAD_STEPPINGS_DETAIIL);

    //
    // Request pages for breakpoint detail
    //
    PoolManagerRequestAllocation(sizeof(DEBUGGEE_BP_DESCRIPTOR),
                                 MAXIMUM_BREAKPOINTS_WITHOUT_CONTINUE,
                                 BREAKPOINT_DEFINITION_STRUCTURE);

    //
    // Nothing to deallocate
    //
    g_IsNewRequestForDeAllocation = FALSE;

    //
    // Let's start the allocations
    //
    return PoolManagerCheckAndPerformAllocationAndDeallocation();
}

/**
 * @brief Uninitialize the pool manager (free the buffers, etc.)
 * 
 * @return VOID 
 */
VOID
PoolManagerUninitialize()
{
    PLIST_ENTRY ListTemp = 0;
    UINT64      Address  = 0;
    ListTemp             = &g_ListOfAllocatedPoolsHead;

    while (&g_ListOfAllocatedPoolsHead != ListTemp->Flink)
    {
        ListTemp = ListTemp->Flink;

        //
        // Get the head of the record
        //
        PPOOL_TABLE PoolTable = (PPOOL_TABLE)CONTAINING_RECORD(ListTemp, POOL_TABLE, PoolsList);

        //
        // Free the alloocated buffer (if not already changed)
        //
        if (!PoolTable->AlreadyFreed)
        {
            ExFreePoolWithTag(PoolTable->Address, POOLTAG);
        }

        //
        // Unlink the PoolTable
        //
        RemoveEntryList(&PoolTable->PoolsList);

        //
        // Free the record itself
        //
        ExFreePoolWithTag(PoolTable, POOLTAG);
    }

    ExFreePoolWithTag(g_RequestNewAllocation, POOLTAG);
}

/**
 * @brief This function set a pool flag to be freed, and it will be freed
 * on the next IOCTL when it's safe to remove
 * 
 * @param AddressToFree The pool address that was previously obtained from the pool manager
 * @return BOOLEAN If the address was already in the list of allocated pools by pool 
 * manager then it returns TRUE; otherwise, FALSE
 */
BOOLEAN
PoolManagerFreePool(UINT64 AddressToFree)
{
    PLIST_ENTRY ListTemp = 0;
    BOOLEAN     Result   = FALSE;
    ListTemp             = &g_ListOfAllocatedPoolsHead;

    SpinlockLock(&LockForReadingPool);

    while (&g_ListOfAllocatedPoolsHead != ListTemp->Flink)
    {
        ListTemp = ListTemp->Flink;

        //
        // Get the head of the record
        //
        PPOOL_TABLE PoolTable = (PPOOL_TABLE)CONTAINING_RECORD(ListTemp, POOL_TABLE, PoolsList);

        if (PoolTable->Address == AddressToFree)
        {
            //
            // We found an entry that matched the detailed from
            // previously allocated pools
            //
            PoolTable->ShouldBeFreed = TRUE;
            Result                   = TRUE;

            g_IsNewRequestForDeAllocation = TRUE;
            break;
        }
    }

    SpinlockUnlock(&LockForReadingPool);
    return Result;
}

/**
 * @brief This function should be called from vmx-root in order to get a pool from the list
 * @details If RequestNewPool is TRUE then Size is used, otherwise Size is useless
 * Note : Most of the times this function called from vmx root but not all the time
 * 
 * @param Intention The intention why we need this pool for (buffer tag)
 * @param RequestNewPool Create a request to allocate a new pool with the same size, next time
 * that it's safe to allocate (this way we never ran out of pools for this "Intention")
 * @param Size If the RequestNewPool is true the we should specify a size for the new pool
 * @return UINT64 Returns a pool address or retuns null if there was an error
 */
UINT64
PoolManagerRequestPool(POOL_ALLOCATION_INTENTION Intention, BOOLEAN RequestNewPool, UINT32 Size)
{
    PLIST_ENTRY ListTemp = 0;
    UINT64      Address  = 0;
    ListTemp             = &g_ListOfAllocatedPoolsHead;

    SpinlockLock(&LockForReadingPool);

    while (&g_ListOfAllocatedPoolsHead != ListTemp->Flink)
    {
        ListTemp = ListTemp->Flink;

        //
        // Get the head of the record
        //
        PPOOL_TABLE PoolTable = (PPOOL_TABLE)CONTAINING_RECORD(ListTemp, POOL_TABLE, PoolsList);

        if (PoolTable->Intention == Intention && PoolTable->IsBusy == FALSE)
        {
            PoolTable->IsBusy = TRUE;
            Address           = PoolTable->Address;
            break;
        }
    }

    SpinlockUnlock(&LockForReadingPool);

    //
    // Check if we need additional pools e.g another pool or the pool
    // will be available for the next use blah blah
    //
    if (RequestNewPool)
    {
        PoolManagerRequestAllocation(Size, 1, Intention);
    }

    //
    // return Address might be null indicating there is no valid pools
    //
    return Address;
}

/**
 * @brief Allocate the new pools and add them to pool table
 * @details This function doesn't need lock as it just calls once from PASSIVE_LEVEL
 * 
 * @param Size Size of each chunk
 * @param Count Count of chunks
 * @param Intention The Intention of the buffer (buffer tag)
 * @return BOOLEAN If the allocation was successfull it returns true and if it was
 * unsuccessful then it returns false
 */
BOOLEAN
PoolManagerAllocateAndAddToPoolTable(SIZE_T Size, UINT32 Count, POOL_ALLOCATION_INTENTION Intention)
{
    for (size_t i = 0; i < Count; i++)
    {
        POOL_TABLE * SinglePool = ExAllocatePoolWithTag(NonPagedPool, sizeof(POOL_TABLE), POOLTAG);

        if (!SinglePool)
        {
            LogError("Insufficient memory");
            return FALSE;
        }

        RtlZeroMemory(SinglePool, sizeof(POOL_TABLE));

        //
        // Allocate the buffer
        //
        SinglePool->Address = ExAllocatePoolWithTag(NonPagedPool, Size, POOLTAG);

        if (!SinglePool->Address)
        {
            LogError("Insufficient memory");
            return FALSE;
        }

        RtlZeroMemory(SinglePool->Address, Size);

        SinglePool->Intention     = Intention;
        SinglePool->IsBusy        = FALSE;
        SinglePool->ShouldBeFreed = FALSE;
        SinglePool->AlreadyFreed  = FALSE;
        SinglePool->Size          = Size;

        //
        // Add it to the list
        //
        InsertHeadList(&g_ListOfAllocatedPoolsHead, &(SinglePool->PoolsList));
    }
}

/**
 * @brief This function performs allocations from VMX non-root based on g_RequestNewAllocation
 * 
 * @return BOOLEAN If the the pool manager allocates buffer or there was no buffer to allocate
 * then it returns true, if there was any error then it returns false
 */
BOOLEAN
PoolManagerCheckAndPerformAllocationAndDeallocation()
{
    BOOLEAN     Result   = TRUE;
    PLIST_ENTRY ListTemp = 0;
    UINT64      Address  = 0;

    //
    // let's make sure we're on vmx non-root and also we have new allocation
    //
    if (g_GuestState[KeGetCurrentProcessorNumber()].IsOnVmxRootMode)
    {
        //
        // allocation's can't be done from vmx root
        //
        return FALSE;
    }

    PAGED_CODE();

    //
    // Check for new allocation
    //
    if (g_IsNewRequestForAllocationRecieved)
    {
        for (size_t i = 0; i < MaximumRequestsQueueDepth; i++)
        {
            if (g_RequestNewAllocation[i].Size != 0)
            {
                Result = PoolManagerAllocateAndAddToPoolTable(g_RequestNewAllocation[i].Size, g_RequestNewAllocation[i].Count, g_RequestNewAllocation[i].Intention);

                //
                // Free the data for future use
                //
                g_RequestNewAllocation[i].Count     = 0;
                g_RequestNewAllocation[i].Intention = 0;
                g_RequestNewAllocation[i].Size      = 0;
            }
        }
    }

    //
    // Check for deallocation
    //
    if (g_IsNewRequestForDeAllocation)
    {
        ListTemp = &g_ListOfAllocatedPoolsHead;

        SpinlockLock(&LockForReadingPool);

        while (&g_ListOfAllocatedPoolsHead != ListTemp->Flink)
        {
            ListTemp = ListTemp->Flink;

            //
            // Get the head of the record
            //
            PPOOL_TABLE PoolTable = (PPOOL_TABLE)CONTAINING_RECORD(ListTemp, POOL_TABLE, PoolsList);

            //
            // Check whther this pool should be freed or not and
            // also check whether it's already freed or not
            //
            if (PoolTable->ShouldBeFreed && !PoolTable->AlreadyFreed)
            {
                //
                // Set the flag to indicate that we freed
                //
                PoolTable->AlreadyFreed = TRUE;

                //
                // This item should be freed
                //
                ExFreePoolWithTag(PoolTable->Address, POOLTAG);

                //
                // Now we should remove the entry from the g_ListOfAllocatedPoolsHead
                //
                RemoveEntryList(&PoolTable->PoolsList);

                //
                // Free the structure pool
                //
                ExFreePoolWithTag(PoolTable, POOLTAG);
            }
        }

        SpinlockUnlock(&LockForReadingPool);
    }

    //
    // All allocation and deallocation are preformed
    //
    g_IsNewRequestForDeAllocation       = FALSE;
    g_IsNewRequestForAllocationRecieved = FALSE;

    return Result;
}

/**
 * @brief Request to allocate new buffers
 * 
 * @param Size Request new buffer to allocate 
 * @param Count Count of chunks
 * @param Intention The intention of chunks (buffer tag)
 * @return BOOLEAN If the request is save it returns true otherwise it returns false
 */
BOOLEAN
PoolManagerRequestAllocation(SIZE_T Size, UINT32 Count, POOL_ALLOCATION_INTENTION Intention)
{
    BOOLEAN FoundAPlace = FALSE;

    //
    // ******** We check to find a free place to store ********
    //

    SpinlockLock(&LockForRequestAllocation);

    for (size_t i = 0; i < MaximumRequestsQueueDepth; i++)
    {
        if (g_RequestNewAllocation[i].Size == 0)
        {
            g_RequestNewAllocation[i].Count     = Count;
            g_RequestNewAllocation[i].Intention = Intention;
            g_RequestNewAllocation[i].Size      = Size;

            FoundAPlace = TRUE;

            break;
        }
    }

    if (!FoundAPlace)
    {
        SpinlockUnlock(&LockForRequestAllocation);
        return FALSE;
    }

    //
    // Signals to show that we have new allocations
    //
    g_IsNewRequestForAllocationRecieved = TRUE;

    SpinlockUnlock(&LockForRequestAllocation);
    return TRUE;
}
