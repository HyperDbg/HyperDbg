/**
 * @file IdtEmulation.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Header for Handlers of Guest's IDT Emulator
 * @details
 * @version 0.1
 * @date 2020-06-10
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once
#include "Events.h"
#include "Common.h"

typedef struct _INTERRUPTIBILITY_STATE
{
    union
    {
        UINT32 Flags;

        struct
        {
            UINT32 BlockingBySti : 1;
            UINT32 BlockingByMovSs : 1;
            UINT32 BlockingBySmi : 1;
            UINT32 BlockingByNmi : 1;
            UINT32 EnclaveInterruption : 1;
            UINT32 Reserved : 27;
        };
    };
} INTERRUPTIBILITY_STATE, *PINTERRUPTIBILITY_STATE;

VOID
IdtEmulationHandleExceptionAndNmi(VMEXIT_INTERRUPT_INFO InterruptExit, UINT32 CurrentProcessorIndex, PGUEST_REGS GuestRegs);

BOOLEAN
IdtEmulationHandleExternalInterrupt(VMEXIT_INTERRUPT_INFO InterruptExit, UINT32 CurrentProcessorIndex);

VOID
IdtEmulationHandleInterruptWindowExiting(UINT32 CurrentProcessorIndex);
