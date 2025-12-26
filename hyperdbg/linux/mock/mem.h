#ifndef MEM_LINUX_H
#define MEM_LINUX_H

#if !defined(__linux__) || !defined(__KERNEL__)
#error "This code must be compiled for Linux kernel only"
#endif

#include <linux/types.h>

void* MemAllocKernel(size_t Size);
void  MemFree(void* Ptr);
void  MemCopy(void* Destination, const void* Source, size_t Size);
void  MemSet(void* Destination, int Value, size_t Size);

#endif

