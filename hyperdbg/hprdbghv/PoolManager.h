/**
 * @file PoolManager.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Headers of pool manager
 * @details
 * @version 0.1
 * @date 2020-04-11
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//                   Definition	    			//
//////////////////////////////////////////////////
#define NumberOfPreAllocatedBuffers 10

//////////////////////////////////////////////////
//                    Enums		    			//
//////////////////////////////////////////////////

/**
 * @brief Inum of intentions for buffers (buffer tag)
 * 
 */
typedef enum
{
    TRACKING_HOOKED_PAGES,
    EXEC_TRAMPOLINE,
    SPLIT_2MB_PAGING_TO_4KB_PAGE,
    DETOUR_HOOK_DETAILS

} POOL_ALLOCATION_INTENTION;

//////////////////////////////////////////////////
//                   Structures		   			//
//////////////////////////////////////////////////

/**
 * @brief Table of holding pools detail structure
 * 
 */
typedef struct _POOL_TABLE
{
    UINT64                    Address; // Should be the start of the list as we compute it as the start address
    SIZE_T                    Size;
    POOL_ALLOCATION_INTENTION Intention;
    LIST_ENTRY                PoolsList;
    BOOLEAN                   IsBusy;
    BOOLEAN                   ShouldBeFreed;

} POOL_TABLE, *PPOOL_TABLE;

/**
 * @brief Manage the requests for new allocations
 * 
 */
typedef struct _REQUEST_NEW_ALLOCATION
{
    SIZE_T                    Size0;
    UINT32                    Count0;
    POOL_ALLOCATION_INTENTION Intention0;

    SIZE_T                    Size1;
    UINT32                    Count1;
    POOL_ALLOCATION_INTENTION Intention1;

    SIZE_T                    Size2;
    UINT32                    Count2;
    POOL_ALLOCATION_INTENTION Intention2;

    SIZE_T                    Size3;
    UINT32                    Count3;
    POOL_ALLOCATION_INTENTION Intention3;

    SIZE_T                    Size4;
    UINT32                    Count4;
    POOL_ALLOCATION_INTENTION Intention4;

    SIZE_T                    Size5;
    UINT32                    Count5;
    POOL_ALLOCATION_INTENTION Intention5;

    SIZE_T                    Size6;
    UINT32                    Count6;
    POOL_ALLOCATION_INTENTION Intention6;

    SIZE_T                    Size7;
    UINT32                    Count7;
    POOL_ALLOCATION_INTENTION Intention7;

    SIZE_T                    Size8;
    UINT32                    Count8;
    POOL_ALLOCATION_INTENTION Intention8;

    SIZE_T                    Size9;
    UINT32                    Count9;
    POOL_ALLOCATION_INTENTION Intention9;

} REQUEST_NEW_ALLOCATION, *PREQUEST_NEW_ALLOCATION;

//////////////////////////////////////////////////
//                   Variables	    			//
//////////////////////////////////////////////////

/**
 * @brief If sb wants allocation from vmx root, adds it's request to this structure
 * 
 */
REQUEST_NEW_ALLOCATION * RequestNewAllocation;

volatile LONG LockForRequestAllocation;
volatile LONG LockForReadingPool;

/**
 * @brief We set it when there is a new allocation
 * 
 */
BOOLEAN IsNewRequestForAllocationRecieved;
/**
 * @brief Create a list from all pools
 * 
 */
PLIST_ENTRY ListOfAllocatedPoolsHead;

//////////////////////////////////////////////////
//                   Functions		  			//
//////////////////////////////////////////////////

/* Initializes the Pool Manager and pre-allocate some pools */
BOOLEAN
PoolManagerInitialize();
/* Should be called in PASSIVE_LEVEL (vmx non-root), it tries to see whether a new pool request is available, if availabe then allocates it */
BOOLEAN
PoolManagerCheckAndPerformAllocation();
/* If we have request to allocate new pool, we can call this function (should be called from vmx-root), it stores the requests */
/* somewhere then when it's safe (IRQL PASSIVE_LEVEL) it allocates the requested pool */
BOOLEAN
PoolManagerRequestAllocation(SIZE_T Size, UINT32 Count, POOL_ALLOCATION_INTENTION Intention);
/* From vmx-root if we need a safe pool address immediately we call it, it also request a new pool if we set RequestNewPool to TRUE */
/* next time it's safe the pool will be allocated */
UINT64
PoolManagerRequestPool(POOL_ALLOCATION_INTENTION Intention, BOOLEAN RequestNewPool, UINT32 Size);
/* De-allocate all the allocated pools */
VOID
PoolManagerUninitialize();
