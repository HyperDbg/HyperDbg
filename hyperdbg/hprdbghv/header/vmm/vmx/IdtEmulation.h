/**
 * @file IdtEmulation.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Header for Handlers of Guest's IDT Emulator
 * @details
 * @version 0.1
 * @date 2020-06-10
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//				     Structures		      		//
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//				     Functions		      		//
//////////////////////////////////////////////////

VOID
IdtEmulationHandleExceptionAndNmi(_In_ UINT32                          CurrentProcessorIndex,
                                  _Inout_ VMEXIT_INTERRUPT_INFORMATION InterruptExit,
                                  _Inout_ PGUEST_REGS                  GuestRegs);

VOID
IdtEmulationHandleExternalInterrupt(_In_ UINT32                          CurrentProcessorIndex,
                                    _Inout_ VMEXIT_INTERRUPT_INFORMATION InterruptExit,
                                    _Inout_ PGUEST_REGS                  GuestRegs);

VOID
IdtEmulationHandleNmiWindowExiting(_In_ UINT32         CurrentProcessorIndex,
                                   _Inout_ PGUEST_REGS GuestRegs);

VOID
IdtEmulationHandleInterruptWindowExiting(_In_ UINT32 CurrentProcessorIndex);

BOOLEAN
IdtEmulationHandlePageFaults(_In_ UINT32                       CurrentProcessorIndex,
                             _In_ VMEXIT_INTERRUPT_INFORMATION InterruptExit,
                             _In_ UINT64                       Address,
                             _In_ ULONG                        ErrorCode);
