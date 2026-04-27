/**
 * @file GeneralTypes.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Cross platform general types definitions
 * @details used both in kernel and user modes
 * @version 0.19
 * @date 2026-04-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//                    Types                     //
//////////////////////////////////////////////////

#ifdef _WIN32 // Windows

#elif defined(__linux__) // Linux

#    include <linux/slab.h>

typedef long PLAT_STATUS;

typedef void   VOID;
typedef void * PVOID;

typedef size_t SIZE_T;

typedef signed char        INT8, *PINT8;
typedef signed short       INT16, *PINT16;
typedef signed int         INT32, *PINT32;
typedef signed long long   INT64, *PINT64;
typedef unsigned char      UINT8, *PUINT8;
typedef unsigned short     UINT16, *PUINT16;
typedef unsigned int       UINT32, *PUINT32;
typedef unsigned long long UINT64, *PUINT64;

typedef unsigned char  UCHAR;
typedef unsigned short USHORT;
typedef unsigned long  ULONG;

typedef char  CHAR;
typedef short SHORT;
typedef long  LONG;
typedef int   INT;

#    define PLAT_SUCCESS 0
#    define PLAT_FAIL    -1

#endif
