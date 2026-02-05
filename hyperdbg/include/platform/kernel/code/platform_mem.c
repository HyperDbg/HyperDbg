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

/**
 * @file paltform_mem.c
 * @brief Implementation of cross APIs for different platforms for memory allocation
 */
#include "../header/platform_mem.h"

/**
 * @brief @brief Allocates a contiguous block of zeroed memory in the kernel pool.
 * @param Size The number of bytes to allocate.
 * @return PLAT_PTR
 *         - Pointer to the allocated memory block on success.
 *         - NULL if the allocation failed.
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
 * @brief  Frees a previously allocated memory block.
 *
 * @param Memory Pointer to the memory block to be freed. If NULL, the function
 * returns immediately.
 * @return void
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
 * @brief Writes data from a source buffer to a destination address.
 * @param Process Reserved for target process context (currently unused/NULL).
 * @param Address The destination memory address to write to.
 * @param Buffer  The source buffer containing data to copy.
 * @param Size    The number of bytes to copy.
 *
 * @return PLAT_STATUS
 *         - PLAT_SUCCESS (0) on success.
 *         - PLAT_FAIL if Address or Buffer is NULL.
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
 * @brief Sets a block of memory to a specific value.
 *
 * @param Destination Pointer to the memory block to be set.
 * @param Value       The value to set (interpreted as an unsigned char).
 * @param Size        The number of bytes to be set to the value.
 *
 * @return void
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