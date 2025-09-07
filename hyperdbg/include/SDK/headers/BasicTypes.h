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

#pragma warning(disable : 4201) // Suppress nameless struct/union warning

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

#define NULL_ZERO   0
#define NULL64_ZERO 0ull

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
#pragma pack(push, 1) // Ensure no padding
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
#pragma pack(pop)

typedef struct XMM_REG
{
    UINT64 XmmLow;  // Low 64 bits
    UINT64 XmmHigh; // High 64 bits

} XMM_REG;

#pragma pack(push, 1) // Ensure no padding
typedef struct GUEST_XMM_REGS
{
    XMM_REG xmm0; // 0x00
    XMM_REG xmm1; // 0x10
    XMM_REG xmm2; // 0x20
    XMM_REG xmm3; // 0x30
    XMM_REG xmm4; // 0x40
    XMM_REG xmm5; // 0x50

    //
    // As per Microsoft ABI documentation, the following registers are nonvolatile
    // So, MSVC compiler will save them on the stack if they are used in the function
    // Thus, for the sake of performance, we comment them out
    //
    XMM_REG xmm6_not_saved;  // 0x60
    XMM_REG xmm7_not_saved;  // 0x70
    XMM_REG xmm8_not_saved;  // 0x80
    XMM_REG xmm9_not_saved;  // 0x90
    XMM_REG xmm10_not_saved; // 0xA0
    XMM_REG xmm11_not_saved; // 0xB0
    XMM_REG xmm12_not_saved; // 0xC0
    XMM_REG xmm13_not_saved; // 0xD0
    XMM_REG xmm14_not_saved; // 0xE0
    XMM_REG xmm15_not_saved; // 0xF0

    UINT32 mxcsr; // 0x100

} GUEST_XMM_REGS, *PGUEST_XMM_REGS;
#pragma pack(pop)

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
 * @brief List of different variables
 */
typedef struct _SCRIPT_ENGINE_GENERAL_REGISTERS
{
    UINT64 * StackBuffer;
    UINT64 * GlobalVariablesList;
    UINT64   StackIndx;
    UINT64   StackBaseIndx;
    UINT64   ReturnValue;
} SCRIPT_ENGINE_GENERAL_REGISTERS, *PSCRIPT_ENGINE_GENERAL_REGISTERS;

/**
 * @brief CR3 Structure
 *
 */
typedef struct _CR3_TYPE
{
    union
    {
        UINT64 Flags;

        struct
        {
            UINT64 Pcid : 12;
            UINT64 PageFrameNumber : 36;
            UINT64 Reserved1 : 12;
            UINT64 Reserved_2 : 3;
            UINT64 PcidInvalidate : 1;
        } Fields;
    };
} CR3_TYPE, *PCR3_TYPE;
