/**
 * @file mem.c
 * @author Behrooz Abbassi (BehroozAbbassi@hyperdbg.org)
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of cross APIs for different platforms for memory allocation
 * @details
 * @version 0.1
 * @date 2022-01-17
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "../header/Mem.h"
#ifdef __linux__

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include "../header/module_info.h"

void* MemAllocKernel(size_t Size)
{
    void* ptr = kzalloc(Size, GFP_KERNEL);

    if (ptr)
        printk(KERN_INFO "MemAllocKernel: Allocated %zu bytes at %px\n", Size, ptr);
    else
        printk(KERN_ERR "MemAllocKernel: failed to allocate %zu bytes\n", Size);

    return ptr;
}

void MemFree(void* Ptr)
{
    if (Ptr) {
        printk(KERN_INFO "MemFree: Freeing memory at %px\n", Ptr);
        kfree(Ptr);
    }
}

void MemCopy(void* Destination, const void* Source, size_t Size)
{
    memcpy(Destination, Source, Size);
}

void MemSet(void* Destination, int Value, size_t Size)
{
    memset(Destination, Value, Size);
}




#elif defined(_WIN32)
#include "pch.h"

/**
 * @brief Allocate a contiguous zeroed memory
 *
 * @param NumberOfBytes
 * @return PVOID
 */
PVOID
PlatformMemAllocateContiguousZeroedMemory(SIZE_T NumberOfBytes)
{
    PVOID            Result          = NULL;
    PHYSICAL_ADDRESS MaxPhysicalAddr = {.QuadPart = MAXULONG64};

    Result = MmAllocateContiguousMemory(NumberOfBytes, MaxPhysicalAddr);
    if (Result != NULL)
        RtlSecureZeroMemory(Result, NumberOfBytes);

    return Result;
}

/**
 * @brief Allocate a non-paged buffer
 *
 * @param NumberOfBytes
 * @return PVOID
 */
PVOID
PlatformMemAllocateNonPagedPool(SIZE_T NumberOfBytes)
{
    PVOID Result = ExAllocatePoolWithTag(NonPagedPool, NumberOfBytes, POOLTAG);

    return Result;
}

/**
 * @brief Allocate a non-paged buffer (use QUOTA)
 *
 * @param NumberOfBytes
 * @return PVOID
 */
PVOID
PlatformMemAllocateNonPagedPoolWithQuota(SIZE_T NumberOfBytes)
{
    PVOID Result = ExAllocatePool2(POOL_FLAG_NON_PAGED | POOL_FLAG_USE_QUOTA, NumberOfBytes, POOLTAG);

    return Result;
}

/**
 * @brief Allocate a non-paged buffer (zeroed)
 *
 * @param NumberOfBytes
 * @return PVOID
 */
PVOID
PlatformMemAllocateZeroedNonPagedPool(SIZE_T NumberOfBytes)
{
    PVOID Result = ExAllocatePoolWithTag(NonPagedPool, NumberOfBytes, POOLTAG);

    if (Result != NULL)
        RtlSecureZeroMemory(Result, NumberOfBytes);

    return Result;
}

/**
 * @brief Free (dellocate) a non-paged buffer
 *
 * @param BufferAddress
 * @return VOID
 */
VOID
PlatformMemFreePool(PVOID BufferAddress)
{
    ExFreePoolWithTag(BufferAddress, POOLTAG);
}

#else
#error "Unsupported platform"
#endif
