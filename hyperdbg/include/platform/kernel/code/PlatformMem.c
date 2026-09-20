/**
 * @file PlatformMem.c
 * @author Behrooz Abbassi (BehroozAbbassi@hyperdbg.org)
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @author alireza moradi (alish014)
 * @brief Implementation of cross APIs for different platforms for memory allocation
 * @details
 * @version 0.1
 * @date 2022-01-17
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#if defined(__linux__)
#    include "../header/PlatformMem.h"
#    include <linux/io.h> // ioremap / iounmap / virt_to_phys / phys_to_virt
#endif // defined(__linux__)

#if defined(_WIN32) || defined(_WIN64)
//
// MmUnmapViewOfSection is a semi-documented ntoskrnl export that the WDK headers
// (ntddk.h/ntifs.h) do not declare. hyperkd declares it privately in its
// Common.h, but PlatformMemUnmapViewOfSection below is compiled into every kernel
// project (hyperlog/hyperperf/hypertrace/...), which do not see that header — so
// declare it here too, matching hyperkd/header/common/Common.h.
//
NTSTATUS
MmUnmapViewOfSection(PEPROCESS Process, PVOID BaseAddress);
#endif // defined(_WIN32) || defined(_WIN64)

/////////////////////////////////////////////////
/// ...  New Unified API ...
/////////////////////////////////////////////////

/**
 * @brief Allocates a block of memory in the kernel pool.
 * @details On Windows: Allocates from NonPagedPool and zeroes it.
 *          On Linux: Uses kzalloc (GFP_KERNEL) which zeroes memory.
 *
 * @param Size The number of bytes to allocate.
 * @return PVOID Pointer to the allocated memory, or NULL on failure.
 */
PVOID
PlatformAllocateMemory(
    SIZE_T Size)
{
#ifdef _WIN32
    PVOID Result = ExAllocatePool2(
        POOL_FLAG_NON_PAGED, // non-paged pool
        Size,
        POOLTAG);

    if (Result != NULL)
        RtlSecureZeroMemory(Result, Size);

    return Result;
#else
    // Linux Kernel: kzalloc allocates zeroed memory
    PVOID ptr = kzalloc(Size, GFP_KERNEL);

    if (ptr)
    {
        printk(KERN_INFO "MemAllocKernel: Allocated %zu bytes at %px\n", Size, ptr);
    }
    else
    {
        printk(KERN_ERR "MemAllocKernel: failed to allocate %zu bytes\n", Size);
    }

    return ptr;
#endif
}

/**
 * @brief Frees a previously allocated memory block.
 * @param Memory Pointer to the memory block. Handles NULL safely.
 */
VOID
PlatformFreeMemory(
    PVOID Memory)
{
    if (!Memory)
        return;

#ifdef _WIN32
    ExFreePoolWithTag(Memory, POOLTAG);
#else
    kfree(Memory);
#endif
}

/**
 * @brief Writes data from a buffer to a memory address.
 * @param Process Reserved (unused).
 * @param Address Destination address.
 * @param Buffer Source buffer.
 * @param Size Number of bytes to copy.
 * @return VOID
 */
VOID
PlatformWriteMemory(
    PVOID  Address, // Destination
    PVOID  Buffer,  // Source
    SIZE_T Size)
{
#ifdef _WIN32
    RtlCopyMemory(Address, Buffer, Size);
#else
    memcpy(Address, Buffer, Size);
#endif
}

/**
 * @brief Sets a memory block to a specific value.
 * @param Destination Memory address.
 * @param Value Value to set.
 * @param Size Number of bytes.
 */
VOID
PlatformSetMemory(
    PVOID  Destination,
    int    Value,
    SIZE_T Size)
{
    if (!Destination)
        return;

#ifdef _WIN32
    RtlFillMemory(Destination, Size, Value);
#else
    memset(Destination, Value, Size);
#endif
}

/**
 * @brief Zeros a memory block.
 * @param Destination Memory address.
 * @param Size Number of bytes.
 */
VOID
PlatformZeroMemory(
    PVOID  Destination,
    SIZE_T Size)
{
    if (!Destination)
        return;

#ifdef _WIN32
    RtlZeroMemory(Destination, Size);
#elif defined(__linux__)
    memset(Destination, 0, Size);
#else
#    error "Unsupported platform"
#endif
}

/////////////////////////////////////////////////
/// ...  Backward Compatibility / Specific APIs ...
/////////////////////////////////////////////////

/**
 * @brief Allocates contiguous zeroed physical memory.
 * @details On Windows: Uses MmAllocateContiguousMemory.
 *          On Linux: Uses kmalloc (which is usually physically contiguous) or dma_alloc_coherent.
 *                    For simplicity in this driver, we map to kzalloc.
 * @param NumberOfBytes Size in bytes.
 * @return PVOID Pointer to memory or NULL.
 */
PVOID
PlatformMemAllocateContiguousZeroedMemory(SIZE_T NumberOfBytes)
{
#ifdef _WIN32
    PVOID            Result          = NULL;
    PHYSICAL_ADDRESS MaxPhysicalAddr = {0};
    MaxPhysicalAddr.QuadPart         = MAXULONG64;

    Result = MmAllocateContiguousMemory(NumberOfBytes, MaxPhysicalAddr);
    if (Result != NULL)
        RtlSecureZeroMemory(Result, NumberOfBytes);
    return Result;
#else
    // In Linux, kmalloc/kzalloc returns physically contiguous memory
    // (unless vmalloc is used, which we aren't using here).
    return kzalloc(NumberOfBytes, GFP_KERNEL);
#endif
}

/**
 * @brief Allocates non-paged pool memory.
 * @param NumberOfBytes Size in bytes.
 * @return PVOID Pointer to memory.
 */
PVOID
PlatformMemAllocateNonPagedPool(SIZE_T NumberOfBytes)
{
#ifdef _WIN32
    return ExAllocatePool2(
        POOL_FLAG_NON_PAGED,
        NumberOfBytes,
        POOLTAG);
#else
    // Linux kernel memory is non-paged by default (except vmalloc)
    return kmalloc(NumberOfBytes, GFP_KERNEL);
#endif
}

/**
 * @brief Allocates non-paged pool memory with quota charging.
 * @param NumberOfBytes Size in bytes.
 * @return PVOID Pointer to memory.
 */
PVOID
PlatformMemAllocateNonPagedPoolWithQuota(SIZE_T NumberOfBytes)
{
#ifdef _WIN32
    // POOL_FLAG_USE_QUOTA is used with ExAllocatePool2
    // Note: Ensure your WDK supports ExAllocatePool2, otherwise use ExAllocatePoolWithQuotaTag
    return ExAllocatePool2(
        POOL_FLAG_NON_PAGED | POOL_FLAG_USE_QUOTA,
        NumberOfBytes,
        POOLTAG);
#else
    // Quotas are not explicitly managed in simple Linux kernel allocations like this
    return kmalloc(NumberOfBytes, GFP_KERNEL);
#endif
}

/**
 * @brief Allocates zeroed non-paged pool memory.
 * @param NumberOfBytes Size in bytes.
 * @return PVOID Pointer to memory.
 */
PVOID
PlatformMemAllocateZeroedNonPagedPool(SIZE_T NumberOfBytes)
{
#ifdef _WIN32
    PVOID Result = ExAllocatePool2(
        POOL_FLAG_NON_PAGED,
        NumberOfBytes,
        POOLTAG);
    if (Result != NULL)
        RtlSecureZeroMemory(Result, NumberOfBytes);
    return Result;
#else
    return kzalloc(NumberOfBytes, GFP_KERNEL);
#endif
}

/**
 * @brief Frees a memory pool.
 * @param BufferAddress Pointer to the memory to free.
 * @return PVOID (Void pointer in original API, usually ignored).
 */
PVOID
PlatformMemFreePool(PVOID BufferAddress)
{
    if (!BufferAddress)
        return NULL;

#ifdef _WIN32
    ExFreePoolWithTag(BufferAddress, POOLTAG);
#else
    kfree(BufferAddress);
#endif
    return NULL;
}

//
// -------------------------------------------------------------------------
// Cross-platform wrappers for the memory-manager / pool APIs the shared sources
// use. Windows forwards to the WDK; the Linux arm forwards to the equivalent
// kernel API where one exists, and is still a placeholder stub where it does not
// (see the remaining TODO(Linux) markers below).
// The #ifdef lives INSIDE each wrapper so the call sites stay OS-agnostic.
// -------------------------------------------------------------------------
//

PVOID
PlatformMemMapIoSpace(PHYSICAL_ADDRESS PhysicalAddress, SIZE_T NumberOfBytes, MEMORY_CACHING_TYPE CacheType)
{
#if defined(_WIN32) || defined(_WIN64)
    return MmMapIoSpace(PhysicalAddress, NumberOfBytes, CacheType);
#elif defined(__linux__)
    //
    // MmMapIoSpace takes the cacheability as an argument, so pick the ioremap
    // flavour that matches rather than defaulting everything to uncached —
    // plain ioremap() is UC- on x86, which would silently be the wrong
    // attribute for an MmCached or MmWriteCombined request.
    //
    switch (CacheType)
    {
    case MmCached:
        return (PVOID)ioremap_cache((phys_addr_t)PhysicalAddress.QuadPart, NumberOfBytes);

    case MmWriteCombined:
        return (PVOID)ioremap_wc((phys_addr_t)PhysicalAddress.QuadPart, NumberOfBytes);

    case MmNonCached:
    default:
        return (PVOID)ioremap((phys_addr_t)PhysicalAddress.QuadPart, NumberOfBytes);
    }
#endif
}

PVOID
PlatformMemMapIoSpaceEx(PHYSICAL_ADDRESS PhysicalAddress, SIZE_T NumberOfBytes, ULONG Protect)
{
#if defined(_WIN32) || defined(_WIN64)
    return MmMapIoSpaceEx(PhysicalAddress, NumberOfBytes, Protect);
#elif defined(__linux__)
    return NULL; // TODO(Linux): ioremap_prot()
#endif
}

VOID
PlatformMemUnmapIoSpace(PVOID BaseAddress, SIZE_T NumberOfBytes)
{
#if defined(_WIN32) || defined(_WIN64)
    MmUnmapIoSpace(BaseAddress, NumberOfBytes);
#elif defined(__linux__)
    //
    // iounmap() recovers the length from the vm_area it created, so the
    // NumberOfBytes the WDK needs is unused here.
    //
    UNREFERENCED_PARAMETER(NumberOfBytes);

    iounmap((void __iomem *)BaseAddress);
#endif
}

PHYSICAL_ADDRESS
PlatformMemGetPhysicalAddress(PVOID BaseAddress)
{
#if defined(_WIN32) || defined(_WIN64)
    return MmGetPhysicalAddress(BaseAddress);
#elif defined(__linux__)
    //
    // virt_to_phys() is valid for direct-map (kmalloc/kzalloc) addresses, which
    // is what every Platform* allocator in this file hands out. It is NOT valid
    // for vmalloc or ioremap addresses — MmGetPhysicalAddress resolves those on
    // Windows, so a caller that starts passing one will need vmalloc_to_pfn()
    // here instead.
    //
    PHYSICAL_ADDRESS Pa;
    Pa.QuadPart = (LONGLONG)virt_to_phys(BaseAddress);
    return Pa;
#endif
}

PVOID
PlatformMemGetVirtualForPhysical(PHYSICAL_ADDRESS PhysicalAddress)
{
#if defined(_WIN32) || defined(_WIN64)
    return MmGetVirtualForPhysical(PhysicalAddress);
#elif defined(__linux__)
    return phys_to_virt((phys_addr_t)PhysicalAddress.QuadPart);
#endif
}

PPHYSICAL_MEMORY_RANGE
PlatformMemGetPhysicalMemoryRanges(VOID)
{
#if defined(_WIN32) || defined(_WIN64)
    return MmGetPhysicalMemoryRanges();
#elif defined(__linux__)
    return NULL; // TODO(Linux): walk the memblock/e820 ranges
#endif
}

PVOID
PlatformMemAllocateMappingAddress(SIZE_T NumberOfBytes, ULONG PoolTag)
{
#if defined(_WIN32) || defined(_WIN64)
    return MmAllocateMappingAddress(NumberOfBytes, PoolTag);
#elif defined(__linux__)
    return NULL; // TODO(Linux): reserve a kernel VA window
#endif
}

VOID
PlatformMemFreeMappingAddress(PVOID BaseAddress, ULONG PoolTag)
{
#if defined(_WIN32) || defined(_WIN64)
    MmFreeMappingAddress(BaseAddress, PoolTag);
#elif defined(__linux__)
    // no-op
#endif
}

VOID
PlatformMemFreeContiguousMemory(PVOID BaseAddress)
{
#if defined(_WIN32) || defined(_WIN64)
    MmFreeContiguousMemory(BaseAddress);
#elif defined(__linux__)
    //
    // Pairs with PlatformMemAllocateContiguousZeroedMemory above, which backs
    // the contiguous allocation with kzalloc().
    //
    kfree(BaseAddress);
#endif
}

NTSTATUS
PlatformMemCopyMemory(PVOID TargetAddress, MM_COPY_ADDRESS SourceAddress, SIZE_T NumberOfBytes, ULONG Flags, PSIZE_T NumberOfBytesTransferred)
{
#if defined(_WIN32) || defined(_WIN64)
    return MmCopyMemory(TargetAddress, SourceAddress, NumberOfBytes, Flags, NumberOfBytesTransferred);
#elif defined(__linux__)
    if (NumberOfBytesTransferred != NULL)
        *NumberOfBytesTransferred = 0;

    return STATUS_UNSUCCESSFUL; // TODO(Linux): copy_from_kernel_nofault (virtual) / memremap+copy (physical)
#endif
}

VOID
PlatformMemFreePoolUntagged(PVOID P)
{
#if defined(_WIN32) || defined(_WIN64)
    ExFreePool(P);
#elif defined(__linux__)
    //
    // Pairs with the kmalloc/kzalloc-backed PlatformMemAllocate* wrappers above
    // (kfree tolerates NULL, matching the callers' expectations).
    //
    kfree(P);
#endif
}

NTSTATUS
PlatformMemAllocateVirtualMemory(HANDLE    ProcessHandle,
                                 PVOID *   BaseAddress,
                                 ULONG_PTR ZeroBits,
                                 PSIZE_T   RegionSize,
                                 ULONG     AllocationType,
                                 ULONG     Protect)
{
#if defined(_WIN32) || defined(_WIN64)
    return ZwAllocateVirtualMemory(ProcessHandle, BaseAddress, ZeroBits, RegionSize, AllocationType, Protect);
#elif defined(__linux__)
    (void)ProcessHandle;
    (void)BaseAddress;
    (void)ZeroBits;
    (void)RegionSize;
    (void)AllocationType;
    (void)Protect;

    return STATUS_UNSUCCESSFUL; // TODO(Linux): vm_mmap into the target process' mm
#endif
}

NTSTATUS
PlatformMemFreeVirtualMemory(HANDLE ProcessHandle, PVOID * BaseAddress, PSIZE_T RegionSize, ULONG FreeType)
{
#if defined(_WIN32) || defined(_WIN64)
    return ZwFreeVirtualMemory(ProcessHandle, BaseAddress, RegionSize, FreeType);
#elif defined(__linux__)
    (void)ProcessHandle;
    (void)BaseAddress;
    (void)RegionSize;
    (void)FreeType;

    return STATUS_UNSUCCESSFUL; // TODO(Linux): vm_munmap
#endif
}

/**
 * @brief Unmap a section previously mapped into a process. Windows:
 *        MmUnmapViewOfSection(). Linux: fail-closed stub.
 * TODO(Linux): vm_munmap() in the target process' mm.
 */
NTSTATUS
PlatformMemUnmapViewOfSection(PEPROCESS Process, PVOID BaseAddress)
{
#if defined(_WIN32) || defined(_WIN64)
    return MmUnmapViewOfSection(Process, BaseAddress);
#elif defined(__linux__)
    (void)Process;
    (void)BaseAddress;

    return STATUS_UNSUCCESSFUL; // TODO(Linux)
#endif
}
