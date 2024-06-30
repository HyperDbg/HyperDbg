/**
 * @file DebuggerEvents.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of debugger VMCALLs
 * @details
 * @version 0.2
 * @date 2023-01-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				 Debugger Vmcalls	      		//
//////////////////////////////////////////////////

/**
 * @brief VMCALL to cause vm-exit and halt the system
 *
 */
#define DEBUGGER_VMCALL_VM_EXIT_HALT_SYSTEM (TOP_LEVEL_DRIVERS_VMCALL_STARTING_NUMBER + 0x00000001)

/**
 * @brief VMCALL to cause vm-exit and halt the system because
 * of triggering an event
 *
 */
#define DEBUGGER_VMCALL_VM_EXIT_HALT_SYSTEM_AS_A_RESULT_OF_TRIGGERING_EVENT (TOP_LEVEL_DRIVERS_VMCALL_STARTING_NUMBER + 0x00000002)

/**
 * @brief VMCALL to signal debugger that debuggee finished
 * execution of the command
 *
 */
#define DEBUGGER_VMCALL_SIGNAL_DEBUGGER_EXECUTION_FINISHED (TOP_LEVEL_DRIVERS_VMCALL_STARTING_NUMBER + 0x00000003)

/**
 * @brief VMCALL to send messages to the debugger
 *
 */
#define DEBUGGER_VMCALL_SEND_MESSAGES_TO_DEBUGGER (TOP_LEVEL_DRIVERS_VMCALL_STARTING_NUMBER + 0x00000004)

/**
 * @brief VMCALL to send general buffers from debuggee user-mode
 * to the debugger
 *
 */
#define DEBUGGER_VMCALL_SEND_GENERAL_BUFFER_TO_DEBUGGER (TOP_LEVEL_DRIVERS_VMCALL_STARTING_NUMBER + 0x00000005)

//////////////////////////////////////////////////
//				     Functions		      		//
//////////////////////////////////////////////////

BOOLEAN
DebuggerVmcallHandler(UINT32 CoreId,
                      UINT64 VmcallNumber,
                      UINT64 OptionalParam1,
                      UINT64 OptionalParam2,
                      UINT64 OptionalParam3);
