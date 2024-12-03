/**
 * @file Idt.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers relating to Interrupt Descriptor Table
 * @details
 *
 * @version 0.11
 * @date 2024-11-20
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				   Definition					//
//////////////////////////////////////////////////

typedef union _KIDTENTRY64
{
    struct
    {
        USHORT OffsetLow;
        USHORT Selector;
        USHORT IstIndex : 3;
        USHORT Reserved0 : 5;
        USHORT Type : 5;
        USHORT Dpl : 2;
        USHORT Present : 1;
        USHORT OffsetMiddle;
        ULONG  OffsetHigh;
        ULONG  Reserved1;
    };
    UINT64 Alignment;
} KIDTENTRY64, *PKIDTENTRY64;

typedef struct _KDESCRIPTOR64
{
    USHORT Pad[3];
    USHORT Limit;
    PVOID  Base;
} KDESCRIPTOR64, *PKDESCRIPTOR64;

//////////////////////////////////////////////////
//				   Functions					//
//////////////////////////////////////////////////

VOID
IdtDumpInterruptEntries();
