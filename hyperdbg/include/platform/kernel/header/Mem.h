/**
 * @file Mem.h
 * @author Behrooz Abbassi (BehroozAbbassi@hyperdbg.org)
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Cross platform APIs for memory allocation
 * @details
 * @version 0.1
 * @date 2022-01-17
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				     Functions		      		//
//////////////////////////////////////////////////

//
// Some functions are globally defined in SDK
//

PVOID
PlatformMemAllocateContiguousZeroedMemory(SIZE_T NumberOfBytes);

// ----------------------------------------------------------------------------
// Cross Platform Memory Allocate/Free Functions
//
PVOID
PlatformMemAllocateNonPagedPool(SIZE_T NumberOfBytes);

PVOID
PlatformMemAllocateNonPagedPoolWithQuota(SIZE_T NumberOfBytes);

PVOID
PlatformMemAllocateZeroedNonPagedPool(SIZE_T NumberOfBytes);

VOID
PlatformMemFreePool(PVOID BufferAddress);
