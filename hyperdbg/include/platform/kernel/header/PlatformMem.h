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

//////////////////////////////////////////////////
//   Cross-platform memory-manager / pool APIs  //
//   (Windows -> WDK; Linux arm stubbed, see .c)//
//////////////////////////////////////////////////

PVOID
PlatformMemMapIoSpace(PHYSICAL_ADDRESS PhysicalAddress, SIZE_T NumberOfBytes, MEMORY_CACHING_TYPE CacheType);

PVOID
PlatformMemMapIoSpaceEx(PHYSICAL_ADDRESS PhysicalAddress, SIZE_T NumberOfBytes, ULONG Protect);

VOID
PlatformMemUnmapIoSpace(PVOID BaseAddress, SIZE_T NumberOfBytes);

PHYSICAL_ADDRESS
PlatformMemGetPhysicalAddress(PVOID BaseAddress);

PVOID
PlatformMemGetVirtualForPhysical(PHYSICAL_ADDRESS PhysicalAddress);

PPHYSICAL_MEMORY_RANGE
PlatformMemGetPhysicalMemoryRanges(VOID);

PVOID
PlatformMemAllocateMappingAddress(SIZE_T NumberOfBytes, ULONG PoolTag);

VOID
PlatformMemFreeMappingAddress(PVOID BaseAddress, ULONG PoolTag);

VOID
PlatformMemFreeContiguousMemory(PVOID BaseAddress);

NTSTATUS
PlatformMemCopyMemory(PVOID TargetAddress, MM_COPY_ADDRESS SourceAddress, SIZE_T NumberOfBytes, ULONG Flags, PSIZE_T NumberOfBytesTransferred);

VOID
PlatformMemFreePoolUntagged(PVOID P);

NTSTATUS
PlatformMemAllocateVirtualMemory(HANDLE    ProcessHandle,
                                 PVOID *   BaseAddress,
                                 ULONG_PTR ZeroBits,
                                 PSIZE_T   RegionSize,
                                 ULONG     AllocationType,
                                 ULONG     Protect);

NTSTATUS
PlatformMemFreeVirtualMemory(HANDLE ProcessHandle, PVOID * BaseAddress, PSIZE_T RegionSize, ULONG FreeType);

NTSTATUS
PlatformMemUnmapViewOfSection(PEPROCESS Process, PVOID BaseAddress);
