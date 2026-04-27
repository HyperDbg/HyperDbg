/**
 * @file PlatformMem.h
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
#    include <ntddk.h>
#    include "pch.h"
// Define a Pool Tag for memory allocation (Required by ExAllocatePoolWithTag)
#    ifndef POOLTAG
#        define 0x48444247 // [H]yper[DBG] (HDBG)
#    endif

typedef PVOID     PLAT_PTR;
typedef ULONG     PLAT_U32;
typedef ULONGLONG PLAT_U64;
typedef SIZE_T    PLAT_SIZE;
typedef NTSTATUS  PLAT_STATUS;

#    define PLAT_SUCCESS STATUS_SUCCESS
#    define PLAT_FAIL    STATUS_UNSUCCESSFUL

#else // Linux Kernel
#    include <linux/kernel.h>
#    include <linux/slab.h>
#    include <linux/string.h>
#    include <linux/types.h>

typedef void   VOID;
typedef void * PVOID;
typedef u32    UINT32;
typedef u32    PLAT_U32;
typedef u64    UINT64;
typedef size_t PLAT_SIZE;
typedef long   PLAT_STATUS;

// typedef signed char      INT8, *PINT8;
// typedef signed short     INT16, *PINT16;
// typedef signed int       INT32, *PINT32;
// typedef signed __int64   INT64, *PINT64;
// typedef unsigned char    UINT8, *PUINT8;
// typedef unsigned short   UINT16, *PUINT16;
// typedef unsigned int     UINT32, *PUINT32;
// typedef unsigned __int64 UINT64, *PUINT64;

#    define PLAT_SUCCESS 0
#    define PLAT_FAIL    -1
#endif

//////////////////////////////////////////////////
//               Prototypes                     //
//////////////////////////////////////////////////

PVOID
PlatformAllocateMemory(PLAT_SIZE Size);

VOID
PlatformFreeMemory(PVOID Memory);

PLAT_STATUS
PlatformWriteMemory(PVOID Process, PVOID Address, PVOID Buffer, PLAT_SIZE Size);

VOID
PlatformSetMemory(PVOID Destination, int Value, PLAT_SIZE Size);
