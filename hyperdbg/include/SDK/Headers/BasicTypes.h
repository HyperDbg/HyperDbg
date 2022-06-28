/**
 * @file BasicTypes.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief HyperDbg's SDK Headers For Basic Datatypes
 * @details This file contains definitions of basic datatypes
 * @version 0.2
 * @date 2022-06-28
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//               Basic Datatypes                //
//////////////////////////////////////////////////

typedef unsigned long long QWORD;
typedef unsigned __int64   UINT64, *PUINT64;
typedef unsigned long      DWORD;
typedef int                BOOL;
typedef unsigned char      BYTE;
typedef unsigned short     WORD;
typedef int                INT;
typedef unsigned int       UINT;
typedef unsigned int *     PUINT;
typedef unsigned __int64   ULONG64, *PULONG64;
typedef unsigned __int64   DWORD64, *PDWORD64;
typedef char               CHAR;
typedef wchar_t            WCHAR;
#define VOID void

typedef unsigned char  UCHAR;
typedef unsigned short USHORT;
typedef unsigned long  ULONG;

typedef UCHAR     BOOLEAN;  // winnt
typedef BOOLEAN * PBOOLEAN; // winnt

typedef signed char      INT8, *PINT8;
typedef signed short     INT16, *PINT16;
typedef signed int       INT32, *PINT32;
typedef signed __int64   INT64, *PINT64;
typedef unsigned char    UINT8, *PUINT8;
typedef unsigned short   UINT16, *PUINT16;
typedef unsigned int     UINT32, *PUINT32;
typedef unsigned __int64 UINT64, *PUINT64;

#define FALSE 0
#define TRUE  1

#define UPPER_56_BITS                  0xffffffffffffff00
#define UPPER_48_BITS                  0xffffffffffff0000
#define UPPER_32_BITS                  0xffffffff00000000
#define LOWER_32_BITS                  0x00000000ffffffff
#define LOWER_16_BITS                  0x000000000000ffff
#define LOWER_8_BITS                   0x00000000000000ff
#define SECOND_LOWER_8_BITS            0x000000000000ff00
#define UPPER_48_BITS_AND_LOWER_8_BITS 0xffffffffffff00ff

//
// DO NOT FUCKING TOUCH THIS STRUCTURE WITHOUT COORDINATION WITH SINA
//
typedef struct GUEST_REGS
{
    //
    // DO NOT FUCKING TOUCH THIS STRUCTURE WITHOUT COORDINATION WITH SINA
    //

    UINT64 rax; // 0x00
    UINT64 rcx; // 0x08
    UINT64 rdx; // 0x10
    UINT64 rbx; // 0x18
    UINT64 rsp; // 0x20
    UINT64 rbp; // 0x28
    UINT64 rsi; // 0x30
    UINT64 rdi; // 0x38
    UINT64 r8;  // 0x40
    UINT64 r9;  // 0x48
    UINT64 r10; // 0x50
    UINT64 r11; // 0x58
    UINT64 r12; // 0x60
    UINT64 r13; // 0x68
    UINT64 r14; // 0x70
    UINT64 r15; // 0x78

    //
    // DO NOT FUCKING TOUCH THIS STRUCTURE WITHOUT COORDINATION WITH SINA
    //

} GUEST_REGS, *PGUEST_REGS;

/**
 * @brief struct for extra registers
 *
 */
typedef struct GUEST_EXTRA_REGISTERS
{
    UINT16 CS;
    UINT16 DS;
    UINT16 FS;
    UINT16 GS;
    UINT16 ES;
    UINT16 SS;
    UINT64 RFLAGS;
    UINT64 RIP;
} GUEST_EXTRA_REGISTERS, *PGUEST_EXTRA_REGISTERS;

/**
 * @brief The structure of detail of a triggered event in HyperDbg
 *
 */
typedef struct _DEBUGGER_TRIGGERED_EVENT_DETAILS
{
    UINT64 Tag; /* in breakpoints Tag is breakpoint id, not event tag */
    PVOID  Context;

} DEBUGGER_TRIGGERED_EVENT_DETAILS, *PDEBUGGER_TRIGGERED_EVENT_DETAILS;

/**
 * @brief List of different variables
 */
typedef struct _SCRIPT_ENGINE_VARIABLES_LIST
{
    UINT64 * TempList;
    UINT64 * GlobalVariablesList;
    UINT64 * LocalVariablesList;

} SCRIPT_ENGINE_VARIABLES_LIST, *PSCRIPT_ENGINE_VARIABLES_LIST;
