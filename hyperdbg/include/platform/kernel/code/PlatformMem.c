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
#endif // defined(__linux__)

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
