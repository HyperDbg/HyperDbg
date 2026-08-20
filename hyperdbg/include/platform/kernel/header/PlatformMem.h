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
PlatformZeroMemory(PVOID Destination, SIZE_T Size);

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

#if defined(__linux__)

//////////////////////////////////////////////////
//     Linux stand-ins for WDK memory APIs      //
//        (placeholder stubs, see the .c)       //
//////////////////////////////////////////////////

PVOID
MmMapIoSpace(PHYSICAL_ADDRESS PhysicalAddress, SIZE_T NumberOfBytes, MEMORY_CACHING_TYPE CacheType);

PVOID
MmMapIoSpaceEx(PHYSICAL_ADDRESS PhysicalAddress, SIZE_T NumberOfBytes, ULONG Protect);

VOID
MmUnmapIoSpace(PVOID BaseAddress, SIZE_T NumberOfBytes);

PHYSICAL_ADDRESS
MmGetPhysicalAddress(PVOID BaseAddress);

PVOID
MmGetVirtualForPhysical(PHYSICAL_ADDRESS PhysicalAddress);

PPHYSICAL_MEMORY_RANGE
MmGetPhysicalMemoryRanges(VOID);

PVOID
MmAllocateMappingAddress(SIZE_T NumberOfBytes, ULONG PoolTag);

VOID
MmFreeMappingAddress(PVOID BaseAddress, ULONG PoolTag);

VOID
MmFreeContiguousMemory(PVOID BaseAddress);

VOID
ExFreePool(PVOID P);

NTSTATUS
ZwAllocateVirtualMemory(HANDLE    ProcessHandle,
                        PVOID *   BaseAddress,
                        ULONG_PTR ZeroBits,
                        PSIZE_T   RegionSize,
                        ULONG     AllocationType,
                        ULONG     Protect);

NTSTATUS
ZwFreeVirtualMemory(HANDLE ProcessHandle, PVOID * BaseAddress, PSIZE_T RegionSize, ULONG FreeType);

#endif // defined(__linux__)
