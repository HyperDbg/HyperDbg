/**
 * @file Idt.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Routines for Interrupt Descriptor Table
 * @details
 *
 * @version 0.11
 * @date 2024-11-20
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Dump IDT Table based on current core
 *
 * @return VOID
 */
VOID
IdtDumpInterruptEntries()
{
    KDESCRIPTOR64 descr;
    __sidt(&descr.Limit);

    ULONG n = (descr.Limit + 1) / sizeof(KIDTENTRY64);

    if (n > 0)
    {
        int           i     = 0;
        KIDTENTRY64 * pidte = (KIDTENTRY64 *)descr.Base;

        do
        {
            ULONG_PTR addr = ((ULONG_PTR)pidte->OffsetHigh << 32) +
                             ((ULONG_PTR)pidte->OffsetMiddle << 16) + pidte->OffsetLow;

            LogInfo("Interrupt %u -> %p\n", i++, addr);

        } while (pidte++, --n);
    }
}
