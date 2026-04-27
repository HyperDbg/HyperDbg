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
#include "PlatformTypes.h"

//////////////////////////////////////////////////
//                 Functions                    //
//////////////////////////////////////////////////

PLAT_STATUS
PlatformReadMemory(
    PVOID     Process,
    PVOID     Address,
    PVOID     Buffer,
    PLAT_SIZE Size);

PLAT_STATUS
PlatformWriteMemory(
    PVOID     Process,
    PVOID     Address,
    PVOID     Buffer,
    PLAT_SIZE Size);

PVOID
PlatformAllocMemory(
    PLAT_SIZE Size);

VOID
PlatformFreeMemory(
    PVOID Memory);

PVOID
PlatformMemAllocateContiguousZeroedMemory(
    PLAT_SIZE NumberOfBytes);

PVOID
PlatformMemAllocateNonPagedPool(
    PLAT_SIZE NumberOfBytes);

PVOID
PlatformMemAllocateNonPagedPoolWithQuota(
    PLAT_SIZE NumberOfBytes);

PVOID
PlatformMemAllocateZeroedNonPagedPool(
    PLAT_SIZE NumberOfBytes);

PVOID
PlatformMemFreePool(
    PVOID BufferAddress);

//////////////////////////////////////////////////
//    Backward-compatible / legacy functions    //
//////////////////////////////////////////////////

PVOID
PlatformMemAllocateContiguousZeroedMemory(
    PLAT_SIZE NumberOfBytes);

PVOID
PlatformMemAllocateNonPagedPool(
    PLAT_SIZE NumberOfBytes);

PVOID
PlatformMemAllocateNonPagedPoolWithQuota(
    PLAT_SIZE NumberOfBytes);

PVOID
PlatformMemAllocateZeroedNonPagedPool(
    PLAT_SIZE NumberOfBytes);

PVOID
PlatformMemFreePool(
    PVOID BufferAddress);
