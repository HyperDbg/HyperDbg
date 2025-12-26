#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include "module_info.h"
#include "mem.h"

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

