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

#include "../../general/header/GeneralTypes.h"

//////////////////////////////////////////////////
//                 Functions                    //
//////////////////////////////////////////////////

PVOID
PlatformAllocateMemory(SIZE_T Size);

VOID
PlatformFreeMemory(PVOID Memory);

PLAT_STATUS
PlatformWriteMemory(PVOID Process, PVOID Address, PVOID Buffer, SIZE_T Size);

VOID
PlatformSetMemory(PVOID Destination, int Value, SIZE_T Size);

PLAT_STATUS
PlatformReadMemory(
    PVOID  Process,
    PVOID  Address,
    PVOID  Buffer,
    SIZE_T Size);

PLAT_STATUS
PlatformWriteMemory(
    PVOID  Process,
    PVOID  Address,
    PVOID  Buffer,
    SIZE_T Size);

PVOID
PlatformAllocMemory(SIZE_T Size);

VOID
PlatformFreeMemory(PVOID Memory);

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

//////////////////////////////////////////////////
//    Backward-compatible / legacy functions    //
//////////////////////////////////////////////////

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
