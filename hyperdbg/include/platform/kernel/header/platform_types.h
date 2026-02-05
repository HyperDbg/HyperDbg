/**
 * @file platform_mem.h
 * @author Alireza Moradi (Alish)
 * @brief Cross platform types definitions
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//               Definitions                    //
//////////////////////////////////////////////////

#ifdef _WIN32
    #include <ntddk.h>

    // Define a Pool Tag for memory allocation (Required by ExAllocatePoolWithTag)
    #ifndef POOLTAG
    #define POOLTAG 'gmEM'
    #endif

    typedef PVOID       PLAT_PTR;
    typedef ULONG       PLAT_U32;
    typedef ULONGLONG   PLAT_U64;
    typedef SIZE_T      PLAT_SIZE;
    typedef NTSTATUS    PLAT_STATUS;

    #define PLAT_SUCCESS STATUS_SUCCESS
    #define PLAT_FAIL    STATUS_UNSUCCESSFUL

#else  // Linux Kernel
    #include <linux/kernel.h>
    #include <linux/slab.h>
    #include <linux/string.h>
    #include <linux/types.h>

    typedef void*       PLAT_PTR;
    typedef u32         PLAT_U32;
    typedef u64         PLAT_U64;
    typedef size_t      PLAT_SIZE;
    typedef long        PLAT_STATUS;

    #define PLAT_SUCCESS 0
    #define PLAT_FAIL    -1
#endif

//////////////////////////////////////////////////
//               Prototypes                     //
//////////////////////////////////////////////////

PLAT_PTR PlatformAllocateMemory(PLAT_SIZE Size);

void PlatformFreeMemory(PLAT_PTR Memory);

PLAT_STATUS PlatformWriteMemory(PLAT_PTR Process, PLAT_PTR Address, PLAT_PTR Buffer, PLAT_SIZE Size);

void PlatformSetMemory(PLAT_PTR Destination, int Value, PLAT_SIZE Size);