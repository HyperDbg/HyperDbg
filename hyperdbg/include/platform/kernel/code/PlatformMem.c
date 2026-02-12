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

/**
 * @file PlatformMem.c
 * @brief Implementation of cross APIs for different platforms for memory allocation
 */
#ifdef _WIN32
#include "pch.h"
#endif
#include "../header/PlatformMem.h"


/////////////////////////////////////////////////
/// ...  New Unified API ...
/////////////////////////////////////////////////


/**
 * @brief Allocates a block of memory in the kernel pool.
 * @details On Windows: Allocates from NonPagedPool and zeroes it.
 *          On Linux: Uses kzalloc (GFP_KERNEL) which zeroes memory.
 *
 * @param Size The number of bytes to allocate.
 * @return PLAT_PTR Pointer to the allocated memory, or NULL on failure.
 */
PLAT_PTR
PlatformAllocateMemory(
    PLAT_SIZE Size
)
{
#ifdef _WIN32
        PLAT_PTR Result = ExAllocatePool2(
        POOL_FLAG_NON_PAGED, // non-paged pool
        Size,
        POOLTAG
    );


    if (Result != NULL)
        RtlSecureZeroMemory(Result, Size);

    return Result;
#else
    // Linux Kernel: kzalloc allocates zeroed memory
    PLAT_PTR ptr = kzalloc(Size, GFP_KERNEL);

    if (ptr) {
        printk(KERN_INFO "MemAllocKernel: Allocated %zu bytes at %px\n", Size, ptr);
    } else {
        printk(KERN_ERR "MemAllocKernel: failed to allocate %zu bytes\n", Size);
    }

    return ptr;
#endif
}

/**
 * @brief Frees a previously allocated memory block.
 * @param Memory Pointer to the memory block. Handles NULL safely.
 */
void
PlatformFreeMemory(
    PLAT_PTR Memory
)
{
    if (!Memory) return;

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
 * @return PLAT_STATUS PLAT_SUCCESS or PLAT_FAIL.
 */
PLAT_STATUS
PlatformWriteMemory(
    PLAT_PTR Process,
    PLAT_PTR Address,  // Destination
    PLAT_PTR Buffer,   // Source
    PLAT_SIZE Size
)
{
    // Process argument currently unused (local copy)
    (void)Process;

    if (!Address || !Buffer) return PLAT_FAIL;

#ifdef _WIN32
    RtlCopyMemory(Address, Buffer, Size);
#else
    memcpy(Address, Buffer, Size);
#endif
    return PLAT_SUCCESS;
}

/**
 * @brief Sets a memory block to a specific value.
 * @param Destination Memory address.
 * @param Value Value to set.
 * @param Size Number of bytes.
 */
void
PlatformSetMemory(
    PLAT_PTR Destination,
    int Value,
    PLAT_SIZE Size
)
{
    if (!Destination) return;

#ifdef _WIN32
    RtlFillMemory(Destination, Size, Value);
#else
    memset(Destination, Value, Size);
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
 * @return PLAT_PTR Pointer to memory or NULL.
 */
PLAT_PTR
PlatformMemAllocateContiguousZeroedMemory(PLAT_SIZE NumberOfBytes)
{
#ifdef _WIN32
    PLAT_PTR            Result          = NULL;
    PHYSICAL_ADDRESS    MaxPhysicalAddr = {0};
    MaxPhysicalAddr.QuadPart            = MAXULONG64;

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
 * @return PLAT_PTR Pointer to memory.
 */
PLAT_PTR
PlatformMemAllocateNonPagedPool(PLAT_SIZE NumberOfBytes)
{
#ifdef _WIN32
    return ExAllocatePool2(
        POOL_FLAG_NON_PAGED,
        NumberOfBytes,
        POOLTAG
    );
#else
    // Linux kernel memory is non-paged by default (except vmalloc)
    return kmalloc(NumberOfBytes, GFP_KERNEL);
#endif
}


/**
 * @brief Allocates non-paged pool memory with quota charging.
 * @param NumberOfBytes Size in bytes.
 * @return PLAT_PTR Pointer to memory.
 */
PLAT_PTR
PlatformMemAllocateNonPagedPoolWithQuota(PLAT_SIZE NumberOfBytes)
{
#ifdef _WIN32
    // POOL_FLAG_USE_QUOTA is used with ExAllocatePool2
    // Note: Ensure your WDK supports ExAllocatePool2, otherwise use ExAllocatePoolWithQuotaTag
    return ExAllocatePool2(
        POOL_FLAG_NON_PAGED | POOL_FLAG_USE_QUOTA,
        NumberOfBytes,
        POOLTAG
    );
#else
    // Quotas are not explicitly managed in simple Linux kernel allocations like this
    return kmalloc(NumberOfBytes, GFP_KERNEL);
#endif
}

/**
 * @brief Allocates zeroed non-paged pool memory.
 * @param NumberOfBytes Size in bytes.
 * @return PLAT_PTR Pointer to memory.
 */
PLAT_PTR
PlatformMemAllocateZeroedNonPagedPool(PLAT_SIZE NumberOfBytes)
{
#ifdef _WIN32
    PLAT_PTR Result = ExAllocatePool2(
        POOL_FLAG_NON_PAGED,
        NumberOfBytes,
        POOLTAG
    );
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
 * @return PLAT_PTR (Void pointer in original API, usually ignored).
 */
PLAT_PTR
PlatformMemFreePool(PLAT_PTR BufferAddress)
{
    if (!BufferAddress) return NULL;

#ifdef _WIN32
    ExFreePoolWithTag(BufferAddress, POOLTAG);
#else
    kfree(BufferAddress);
#endif
    return NULL;
}