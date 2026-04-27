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
typedef SIZE_T    SIZE_T;
typedef NTSTATUS  PLAT_STATUS;

#    define PLAT_SUCCESS STATUS_SUCCESS
#    define PLAT_FAIL    STATUS_UNSUCCESSFUL

#else // Linux Kernel
#    include "../../general/header/GeneralTypes.h"

#endif

//////////////////////////////////////////////////
//               Prototypes                     //
//////////////////////////////////////////////////

PVOID
PlatformAllocateMemory(SIZE_T Size);

VOID
PlatformFreeMemory(PVOID Memory);

PLAT_STATUS
PlatformWriteMemory(PVOID Process, PVOID Address, PVOID Buffer, SIZE_T Size);

VOID
PlatformSetMemory(PVOID Destination, int Value, SIZE_T Size);
