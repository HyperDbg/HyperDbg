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

/**
 * @brief Maximum Pool Requests (while not allocated)
 * 
 */
#define MaximumRequestsQueueDepth   100
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
    DETOUR_HOOK_DETAILS,
    THREAD_STEPPINGS_DETAIIL,
    BREAKPOINT_DEFINITION_STRUCTURE,

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
    BOOLEAN                   AlreadyFreed;

} POOL_TABLE, *PPOOL_TABLE;

/**
 * @brief Manage the requests for new allocations
 * 
 */
typedef struct _REQUEST_NEW_ALLOCATION
{
    SIZE_T                    Size;
    UINT32                    Count;
    POOL_ALLOCATION_INTENTION Intention;

} REQUEST_NEW_ALLOCATION, *PREQUEST_NEW_ALLOCATION;

//////////////////////////////////////////////////
//                   Variables	    			//
//////////////////////////////////////////////////

/**
 * @brief If sb wants allocation from vmx root, adds it's request to this structure
 * 
 */
REQUEST_NEW_ALLOCATION * g_RequestNewAllocation;

/**
 * @brief Request allocation Spinlock
 * 
 */
volatile LONG LockForRequestAllocation;

/**
 * @brief Spinlock for reading pool
 * 
 */
volatile LONG LockForReadingPool;

/**
 * @brief We set it when there is a new allocation
 * 
 */
BOOLEAN g_IsNewRequestForAllocationRecieved;

/**
 * @brief We set it when there is a new allocation
 * 
 */
BOOLEAN g_IsNewRequestForDeAllocation;

/**
 * @brief Create a list from all pools
 * 
 */
LIST_ENTRY g_ListOfAllocatedPoolsHead;

//////////////////////////////////////////////////
//                   Functions		  			//
//////////////////////////////////////////////////

/**
 * @brief Initializes the Pool Manager and pre-allocate some pools
 * 
 * @return BOOLEAN 
 */
BOOLEAN
PoolManagerInitialize();

/**
 * @brief Should be called in PASSIVE_LEVEL (vmx non-root), it tries to see whether
 * a new pool request is available, if availabe then allocates it 
 * 
 * @return BOOLEAN 
 */
BOOLEAN
PoolManagerCheckAndPerformAllocationAndDeallocation();

/**
 * @brief If we have request to allocate new pool
 * @details we can call this function (should be called from vmx-root), it stores the requests
 * somewhere then when it's safe (IRQL PASSIVE_LEVEL) it allocates the requested pool
 * 
 * @param Size 
 * @param Count 
 * @param Intention 
 * @return BOOLEAN 
 */
BOOLEAN
PoolManagerRequestAllocation(SIZE_T Size, UINT32 Count, POOL_ALLOCATION_INTENTION Intention);

/**
 * @brief From vmx-root if we need a safe pool address immediately we call it
 * it also request a new pool if we set RequestNewPool to TRUE
 * next time it's safe the pool will be allocated
 * 
 * @param Intention 
 * @param RequestNewPool 
 * @param Size 
 * @return UINT64 
 */
UINT64
PoolManagerRequestPool(POOL_ALLOCATION_INTENTION Intention, BOOLEAN RequestNewPool, UINT32 Size);

/**
 * @brief De-allocate all the allocated pools
 * 
 * @return VOID 
 */
VOID
PoolManagerUninitialize();

/**
 * @brief Deallocate a special pool in the next IOCTL
 * 
 * @param AddressToFree 
 * @return BOOLEAN 
 */
BOOLEAN
PoolManagerFreePool(UINT64 AddressToFree);
