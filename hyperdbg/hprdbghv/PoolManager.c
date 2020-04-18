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
#include <ntddk.h>
#include "PoolManager.h"
#include "Logging.h"
#include "GlobalVariables.h"
#include "PoolManager.h"
#include "Common.h"
#include "Hooks.h"

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
    RequestNewAllocation = ExAllocatePoolWithTag(NonPagedPool, sizeof(REQUEST_NEW_ALLOCATION), POOLTAG);

    if (!RequestNewAllocation)
    {
        LogError("Insufficient memory");
        return FALSE;
    }
    RtlZeroMemory(RequestNewAllocation, sizeof(REQUEST_NEW_ALLOCATION));

    ListOfAllocatedPoolsHead = ExAllocatePoolWithTag(NonPagedPool, sizeof(LIST_ENTRY), POOLTAG);

    if (!ListOfAllocatedPoolsHead)
    {
        LogError("Insufficient memory");
        return FALSE;
    }
    RtlZeroMemory(ListOfAllocatedPoolsHead, sizeof(LIST_ENTRY));

    //
    // Initialize list head
    //
    InitializeListHead(ListOfAllocatedPoolsHead);

    //
    // Request pages to be allocated for converting 2MB to 4KB pages
    //
    PoolManagerRequestAllocation(sizeof(VMM_EPT_DYNAMIC_SPLIT), 10, SPLIT_2MB_PAGING_TO_4KB_PAGE);

    //
    // Request pages to be allocated for paged hook details
    //
    PoolManagerRequestAllocation(sizeof(EPT_HOOKED_PAGE_DETAIL), 10, TRACKING_HOOKED_PAGES);

    //
    // Request pages to be allocated for Trampoline of Executable hooked pages
    //
    PoolManagerRequestAllocation(MAX_EXEC_TRAMPOLINE_SIZE, 10, EXEC_TRAMPOLINE);

    //
    // Request pages to be allocated for detour hooked pages details
    //
    PoolManagerRequestAllocation(sizeof(HIDDEN_HOOKS_DETOUR_DETAILS), 10, DETOUR_HOOK_DETAILS);

    //
    // Let's start the allocations
    //
    return PoolManagerCheckAndPerformAllocation();
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
    ListTemp             = ListOfAllocatedPoolsHead;

    while (ListOfAllocatedPoolsHead != ListTemp->Flink)
    {
        ListTemp = ListTemp->Flink;

        //
        // Get the head of the record
        //
        PPOOL_TABLE PoolTable = (PPOOL_TABLE)CONTAINING_RECORD(ListTemp, POOL_TABLE, PoolsList);

        //
        // Free the alloocated buffer
        //
        ExFreePoolWithTag(PoolTable->Address, POOLTAG);

        //
        // Free the record itself
        //
        ExFreePoolWithTag(PoolTable, POOLTAG);
    }

    ExFreePoolWithTag(ListOfAllocatedPoolsHead, POOLTAG);
    ExFreePoolWithTag(RequestNewAllocation, POOLTAG);
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
    ListTemp             = ListOfAllocatedPoolsHead;

    SpinlockLock(&LockForReadingPool);

    while (ListOfAllocatedPoolsHead != ListTemp->Flink)
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
        SinglePool->Size          = Size;

        //
        // Add it to the list
        //
        InsertHeadList(ListOfAllocatedPoolsHead, &(SinglePool->PoolsList));
    }
}

/**
 * @brief This function performs allocations from VMX non-root based on RequestNewAllocation
 * 
 * @return BOOLEAN If the the pool manager allocates buffer or there was no buffer to allocate
 * then it returns true, if there was any error then it returns false
 */
BOOLEAN
PoolManagerCheckAndPerformAllocation()
{
    BOOLEAN Result = TRUE;

    //
    // let's make sure we're on vmx non-root and also we have new allocation
    //
    if (!IsNewRequestForAllocationRecieved || g_GuestState[KeGetCurrentProcessorNumber()].IsOnVmxRootMode)
    {
        //
        // allocation's can't be done from vmx root
        //
        return FALSE;
    }

    PAGED_CODE();

    if (RequestNewAllocation->Size0 != 0)
    {
        Result = PoolManagerAllocateAndAddToPoolTable(RequestNewAllocation->Size0, RequestNewAllocation->Count0, RequestNewAllocation->Intention0);

        //
        // Free the data for future use
        //
        RequestNewAllocation->Count0     = 0;
        RequestNewAllocation->Intention0 = 0;
        RequestNewAllocation->Size0      = 0;
    }
    if (RequestNewAllocation->Size1 != 0)
    {
        Result = PoolManagerAllocateAndAddToPoolTable(RequestNewAllocation->Size1, RequestNewAllocation->Count1, RequestNewAllocation->Intention1);

        //
        // Free the data for future use
        //
        RequestNewAllocation->Count1     = 0;
        RequestNewAllocation->Intention1 = 0;
        RequestNewAllocation->Size1      = 0;
    }
    if (RequestNewAllocation->Size2 != 0)
    {
        Result = PoolManagerAllocateAndAddToPoolTable(RequestNewAllocation->Size2, RequestNewAllocation->Count2, RequestNewAllocation->Intention2);

        //
        // Free the data for future use
        //
        RequestNewAllocation->Count2     = 0;
        RequestNewAllocation->Intention2 = 0;
        RequestNewAllocation->Size2      = 0;
    }

    if (RequestNewAllocation->Size3 != 0)
    {
        Result = PoolManagerAllocateAndAddToPoolTable(RequestNewAllocation->Size3, RequestNewAllocation->Count3, RequestNewAllocation->Intention3);

        //
        // Free the data for future use
        //

        RequestNewAllocation->Count3     = 0;
        RequestNewAllocation->Intention3 = 0;
        RequestNewAllocation->Size3      = 0;
    }
    if (RequestNewAllocation->Size4 != 0)
    {
        Result = PoolManagerAllocateAndAddToPoolTable(RequestNewAllocation->Size4, RequestNewAllocation->Count4, RequestNewAllocation->Intention4);

        //
        // Free the data for future use
        //
        RequestNewAllocation->Count4     = 0;
        RequestNewAllocation->Intention4 = 0;
        RequestNewAllocation->Size4      = 0;
    }
    if (RequestNewAllocation->Size5 != 0)
    {
        Result = PoolManagerAllocateAndAddToPoolTable(RequestNewAllocation->Size5, RequestNewAllocation->Count5, RequestNewAllocation->Intention5);

        //
        // Free the data for future use
        //
        RequestNewAllocation->Count5     = 0;
        RequestNewAllocation->Intention5 = 0;
        RequestNewAllocation->Size5      = 0;
    }
    if (RequestNewAllocation->Size6 != 0)
    {
        Result = PoolManagerAllocateAndAddToPoolTable(RequestNewAllocation->Size6, RequestNewAllocation->Count6, RequestNewAllocation->Intention6);

        //
        // Free the data for future use
        //
        RequestNewAllocation->Count6     = 0;
        RequestNewAllocation->Intention6 = 0;
        RequestNewAllocation->Size6      = 0;
    }
    if (RequestNewAllocation->Size7 != 0)
    {
        Result = PoolManagerAllocateAndAddToPoolTable(RequestNewAllocation->Size7, RequestNewAllocation->Count7, RequestNewAllocation->Intention7);

        //
        // Free the data for future use
        //
        RequestNewAllocation->Count7     = 0;
        RequestNewAllocation->Intention7 = 0;
        RequestNewAllocation->Size7      = 0;
    }
    if (RequestNewAllocation->Size8 != 0)
    {
        Result = PoolManagerAllocateAndAddToPoolTable(RequestNewAllocation->Size8, RequestNewAllocation->Count8, RequestNewAllocation->Intention8);

        //
        // Free the data for future use
        //
        RequestNewAllocation->Count8     = 0;
        RequestNewAllocation->Intention8 = 0;
        RequestNewAllocation->Size8      = 0;
    }
    if (RequestNewAllocation->Size9 != 0)
    {
        Result = PoolManagerAllocateAndAddToPoolTable(RequestNewAllocation->Size9, RequestNewAllocation->Count9, RequestNewAllocation->Intention9);

        //
        // Free the data for future use
        //
        RequestNewAllocation->Count9     = 0;
        RequestNewAllocation->Intention9 = 0;
        RequestNewAllocation->Size9      = 0;
    }
    IsNewRequestForAllocationRecieved = FALSE;
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
    //
    // ******** We check to find a free place to store ********
    //

    SpinlockLock(&LockForRequestAllocation);

    if (RequestNewAllocation->Size0 == 0)
    {
        RequestNewAllocation->Count0     = Count;
        RequestNewAllocation->Intention0 = Intention;
        RequestNewAllocation->Size0      = Size;
    }
    else if (RequestNewAllocation->Size1 == 0)
    {
        RequestNewAllocation->Count1     = Count;
        RequestNewAllocation->Intention1 = Intention;
        RequestNewAllocation->Size1      = Size;
    }
    else if (RequestNewAllocation->Size2 == 0)
    {
        RequestNewAllocation->Count2     = Count;
        RequestNewAllocation->Intention2 = Intention;
        RequestNewAllocation->Size2      = Size;
    }
    else if (RequestNewAllocation->Size3 == 0)
    {
        RequestNewAllocation->Count3     = Count;
        RequestNewAllocation->Intention3 = Intention;
        RequestNewAllocation->Size3      = Size;
    }
    else if (RequestNewAllocation->Size4 == 0)
    {
        RequestNewAllocation->Count4     = Count;
        RequestNewAllocation->Intention4 = Intention;
        RequestNewAllocation->Size4      = Size;
    }
    else if (RequestNewAllocation->Size5 == 0)
    {
        RequestNewAllocation->Count5     = Count;
        RequestNewAllocation->Intention5 = Intention;
        RequestNewAllocation->Size5      = Size;
    }
    else if (RequestNewAllocation->Size6 == 0)
    {
        RequestNewAllocation->Count6     = Count;
        RequestNewAllocation->Intention6 = Intention;
        RequestNewAllocation->Size6      = Size;
    }
    else if (RequestNewAllocation->Size7 == 0)
    {
        RequestNewAllocation->Count7     = Count;
        RequestNewAllocation->Intention7 = Intention;
        RequestNewAllocation->Size7      = Size;
    }
    else if (RequestNewAllocation->Size8 == 0)
    {
        RequestNewAllocation->Count8     = Count;
        RequestNewAllocation->Intention8 = Intention;
        RequestNewAllocation->Size8      = Size;
    }
    else if (RequestNewAllocation->Size9 == 0)
    {
        RequestNewAllocation->Count9     = Count;
        RequestNewAllocation->Intention9 = Intention;
        RequestNewAllocation->Size9      = Size;
    }

    else
    {
        SpinlockUnlock(&LockForRequestAllocation);
        return FALSE;
    }

    //
    // Signals to show that we have new allocations
    //
    IsNewRequestForAllocationRecieved = TRUE;

    SpinlockUnlock(&LockForRequestAllocation);
    return TRUE;
}
