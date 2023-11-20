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
//				     Functions		      		//
//////////////////////////////////////////////////

VOID
IdtEmulationHandleExceptionAndNmi(_Inout_ VIRTUAL_MACHINE_STATE *   VCpu,
                                  _In_ VMEXIT_INTERRUPT_INFORMATION InterruptExit);

VOID
IdtEmulationHandleExternalInterrupt(_Inout_ VIRTUAL_MACHINE_STATE *   VCpu,
                                    _In_ VMEXIT_INTERRUPT_INFORMATION InterruptExit);

VOID
IdtEmulationHandleNmiWindowExiting(_Inout_ VIRTUAL_MACHINE_STATE * VCpu);

VOID
IdtEmulationHandleInterruptWindowExiting(_Inout_ VIRTUAL_MACHINE_STATE * VCpu);
