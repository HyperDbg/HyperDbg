/**
 * @file PlatformMem.h
 * @author Behrooz Abbassi (BehroozAbbassi@hyperdbg.org)
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @author Alirez Moradi (alish014)
 * @brief Cross platform APIs for memory allocation
 * @details
 * @version 0.1
 * @date 2022-01-17
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#if defined(__linux__)
// #    include "../../general/header/GeneralTypes.h"
#    include "../../../../include/SDK/HyperDbgSdk.h"
#endif // defined(__linux__)

//////////////////////////////////////////////////
//                 Functions                    //
//////////////////////////////////////////////////

VOID
PlatformFreeMemory(PVOID Memory);

VOID
PlatformWriteMemory(PVOID Address, PVOID Buffer, SIZE_T Size);

VOID
PlatformSetMemory(PVOID Destination, int Value, SIZE_T Size);

VOID
PlatformFreeMemory(PVOID Memory);

PVOID
PlatformAllocateMemory(SIZE_T Size);

PVOID
PlatformMemAllocateContiguousZeroedMemory(SIZE_T NumberOfBytes);

PVOID
PlatformMemAllocateNonPagedPool(SIZE_T NumberOfBytes);

PVOID
PlatformMemAllocateNonPagedPoolWithQuota(SIZE_T NumberOfBytes);

PVOID
PlatformMemAllocateZeroedNonPagedPool(SIZE_T NumberOfBytes);

PVOID
PlatformMemFreePool(PVOID BufferAddress);

PVOID
PlatformMemAllocateContiguousZeroedMemory(SIZE_T NumberOfBytes);

PVOID
PlatformMemAllocateNonPagedPool(SIZE_T NumberOfBytes);

PVOID
PlatformMemAllocateNonPagedPoolWithQuota(SIZE_T NumberOfBytes);

PVOID
PlatformMemAllocateZeroedNonPagedPool(SIZE_T NumberOfBytes);

PVOID
PlatformMemFreePool(PVOID BufferAddress);
