/**
 * @file mem.c
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
#include "../header/platform_mem.h"

/**
 * @file mem.c
 * @brief Implementation of cross APIs for different platforms for memory allocation
 */
#include "platform_mem.h"

/**
 * @brief Allocates zeroed memory
 */
PLAT_PTR
PlatformAllocateMemory(
    PLAT_SIZE Size
)
{
#ifdef _WIN32
    // Windows Kernel: Allocate from NonPagedPool
    PLAT_PTR Result = ExAllocatePoolWithTag(NonPagedPool, Size, POOLTAG);

    if (Result != NULL)
        RtlSecureZeroMemory(Result, Size);

    return Result;
#else
    // Linux Kernel: kzalloc allocates zeroed memory
    PLAT_PTR ptr = kzalloc(Size, GFP_KERNEL);

    if (ptr) {
        // printk(KERN_INFO "MemAllocKernel: Allocated %zu bytes at %px\n", Size, ptr);
    } else {
        printk(KERN_ERR "MemAllocKernel: failed to allocate %zu bytes\n", Size);
    }

    return ptr;
#endif
}

/**
 * @brief Frees memory
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
    // printk(KERN_INFO "MemFree: Freeing memory at %px\n", Memory);
    kfree(Memory);
#endif
}

/**
 * @brief Copy memory (Write Buffer into Address)
 * @note Currently acts as a local memcpy. 'Process' arg is unused.
 */
PLAT_STATUS
PlatformWriteMemory(
    PLAT_PTR Process,
    PLAT_PTR Address,  // Destination
    PLAT_PTR Buffer,   // Source
    PLAT_SIZE Size
)
{
    // Check for null pointers
    if (!Address || !Buffer) return PLAT_FAIL;

#ifdef _WIN32
    RtlCopyMemory(Address, Buffer, Size);
    return PLAT_SUCCESS;
#else
    memcpy(Address, Buffer, Size);
    return PLAT_SUCCESS;
#endif
}

/**
 * @brief Set memory to a specific value (memset)
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