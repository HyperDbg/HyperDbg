/**
 * @file PoolManager.h
 * @author Sina Karvandi (sina@hyperdbg.org)
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
#define MaximumRequestsQueueDepth   300
#define NumberOfPreAllocatedBuffers 10

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
BOOLEAN g_IsNewRequestForAllocationReceived;

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

// ----------------------------------------------------------------------------
// Private Interfaces
//

/// @brief Allocate global requesting variable
/// @param NumberOfBytes
/// @return
static BOOLEAN
PlmgrAllocateRequestNewAllocation(SIZE_T NumberOfBytes);

static VOID PlmgrFreeRequestNewAllocation(VOID);

// ----------------------------------------------------------------------------
// Public Interfaces
//

/**
 * @brief Initializes the Pool Manager and pre-allocate some pools
 *
 * @return BOOLEAN
 */
BOOLEAN
PoolManagerInitialize();

/**
 * @brief De-allocate all the allocated pools
 *
 * @return VOID
 */
VOID
PoolManagerUninitialize();
