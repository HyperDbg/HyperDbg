/**
 * @file Vmcall.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief VMCALL Headers
 * @details
 * @version 0.1
 * @date 2020-04-11
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//				    Constants					//
//////////////////////////////////////////////////

/**
 * @brief VMCALL to test hypervisor
 * 
 */
#define VMCALL_TEST 0x00000001

/**
 * @brief VMCALL to Call VMXOFF to turn off the hypervisor
 * 
 */
#define VMCALL_VMXOFF 0x00000002

/**
 * @brief VMCALL to Hook Change the attribute bits of the EPT Table
 * 
 */
#define VMCALL_CHANGE_PAGE_ATTRIB 0x00000003

/**
 * @brief VMCALL to invalidate EPT (All Contexts)
 * 
 */
#define VMCALL_INVEPT_ALL_CONTEXTS 0x00000004

/**
 * @brief VMCALL to invalidate EPT (A Single Context)
 * 
 */
#define VMCALL_INVEPT_SINGLE_CONTEXT 0x00000005

/**
 * @brief VMCALL to remove a all physical addresses from hook list
 * 
 */
#define VMCALL_UNHOOK_ALL_PAGES 0x00000006

/**
 * @brief VMCALL to remove a single physical address from hook list
 * 
 */
#define VMCALL_UNHOOK_SINGLE_PAGE 0x00000007

/**
 * @brief VMCALL to enable syscall hook using EFER SCE bit
 * 
 */
#define VMCALL_ENABLE_SYSCALL_HOOK_EFER 0x00000008

/**
 * @brief VMCALL to disable syscall hook using EFER SCE bit
 * 
 */
#define VMCALL_DISABLE_SYSCALL_HOOK_EFER 0x00000009

/**
 * @brief VMCALL to change MSR Bitmap Read
 * 
 */
#define VMCALL_CHANGE_MSR_BITMAP_READ 0x0000000A

/**
 * @brief VMCALL to change MSR Bitmap Write
 * 
 */
#define VMCALL_CHANGE_MSR_BITMAP_WRITE 0x0000000B

/**
 * @brief VMCALL to enable rdtsc/rdtscp exiting in primary cpu-based controls
 * 
 */
#define VMCALL_SET_RDTSC_EXITING 0x0000000C

/**
 * @brief VMCALL to enable rdpmc exiting in primary cpu-based controls
 * 
 */
#define VMCALL_SET_RDPMC_EXITING 0x0000000D

/**
 * @brief VMCALL to set exception bitmap on VMCS
 * 
 */
#define VMCALL_SET_EXCEPTION_BITMAP 0x0000000E

/**
 * @brief VMCALL to enable mov to debug registers exiting
 * 
 */
#define VMCALL_ENABLE_MOV_TO_DEBUG_REGS_EXITING 0x0000000F

/**
 * @brief VMCALL to enable external interrupt exiting
 * 
 */
#define VMCALL_ENABLE_EXTERNAL_INTERRUPT_EXITING 0x00000010

/**
 * @brief VMCALL to change I/O Bitmaps (A & B)
 * 
 */
#define VMCALL_CHANGE_IO_BITMAP 0x00000011

/**
 * @brief VMCALL to put hidden breakpoints (using EPT)
 * 
 */
#define VMCALL_SET_HIDDEN_CC_BREAKPOINT 0x00000012

/**
 * @brief VMCALL to disable rdtsc/rdtscp exiting in primary cpu-based controls
 * 
 */
#define VMCALL_UNSET_RDTSC_EXITING 0x00000013

/**
 * @brief VMCALL to disable external interrupt exiting only to clear !interrupt commands
 * 
 */
#define VMCALL_DISABLE_EXTERNAL_INTERRUPT_EXITING_ONLY_TO_CLEAR_INTERRUPT_COMMANDS 0x00000014

/**
 * @brief VMCALL to disable rdpmc exiting in primary cpu-based controls
 * 
 */
#define VMCALL_UNSET_RDPMC_EXITING 0x00000015

/**
 * @brief VMCALL to disable mov to debug registers exiting
 * 
 */
#define VMCALL_DISABLE_MOV_TO_DEBUG_REGS_EXITING 0x00000016

/**
 * @brief VMCALL to reset MSR Bitmap Read
 * 
 */
#define VMCALL_RESET_MSR_BITMAP_READ 0x00000017

/**
 * @brief VMCALL to reset MSR Bitmap Write
 * 
 */
#define VMCALL_RESET_MSR_BITMAP_WRITE 0x00000018

/**
 * @brief VMCALL to reset exception bitmap on VMCS
 * @details THIS VMCALL SHOULD BE USED ONLY IN
 * RESETING (CLEARING) EXCEPTION EVENTS
 * 
 */
#define VMCALL_RESET_EXCEPTION_BITMAP_ONLY_ON_CLEARING_EXCEPTION_EVENTS 0x00000019

/**
 * @brief VMCALL to reset I/O Bitmaps (A & B)
 * 
 */
#define VMCALL_RESET_IO_BITMAP 0x0000001A

/**
 * @brief VMCALL to enable cr3 exiting
 * 
 */
#define VMCALL_ENABLE_MOV_TO_CR3_EXITING 0x0000001B

/**
 * @brief VMCALL to disable cr3 exiting
 * 
 */
#define VMCALL_DISABLE_MOV_TO_CR3_EXITING 0x0000001C

/**
 * @brief VMCALL to unset exception bitmap on VMCS
 * 
 */
#define VMCALL_UNSET_EXCEPTION_BITMAP 0x0000001D

/**
 * @brief VMCALL to set VM-entry LOAD DEBUG CONTROLS
 * @details This control determines whether DR7 and the
 * IA32_DEBUGCTL MSR are loaded on VM entry.
 * 
 */
#define VMCALL_SET_VM_ENTRY_LOAD_DEBUG_CONTROLS 0x0000001E

/**
 * @brief VMCALL to unset VM-entry LOAD DEBUG CONTROLS
 * @details This control determines whether DR7 and the
 * IA32_DEBUGCTL MSR are loaded on VM entry.
 * 
 */
#define VMCALL_UNSET_VM_ENTRY_LOAD_DEBUG_CONTROLS 0x0000001F

/**
 * @brief VMCALL to set VM-exit SAVE DEBUG CONTROLS
 * @details This control determines whether DR7 and the
 * IA32_DEBUGCTL MSR are saved on VM exit.
 * 
 */
#define VMCALL_SET_VM_EXIT_SAVE_DEBUG_CONTROLS 0x0000020

/**
 * @brief VMCALL to unset VM-exit SAVE DEBUG CONTROLS
 * @details This control determines whether DR7 and the
 * IA32_DEBUGCTL MSR are saved on VM exit.
 * 
 */
#define VMCALL_UNSET_VM_EXIT_SAVE_DEBUG_CONTROLS 0x0000021

/**
 * @brief VMCALL to cause vm-exit and halt the system
 * 
 */
#define VMCALL_VM_EXIT_HALT_SYSTEM 0x0000022

/**
 * @brief VMCALL to cause vm-exit on NMIs
 * 
 */
#define VMCALL_SET_VM_EXIT_ON_NMIS 0x0000023

/**
 * @brief VMCALL to not cause vm-exit on NMIs
 * 
 */
#define VMCALL_UNSET_VM_EXIT_ON_NMIS 0x0000024

/**
 * @brief VMCALL to signal debugger that debuggee finished
 * execution of the command
 * 
 */
#define VMCALL_SIGNAL_DEBUGGER_EXECUTION_FINISHED 0x25

/**
 * @brief VMCALL to send messages to the debugger
 * 
 */
#define VMCALL_SEND_MESSAGES_TO_DEBUGGER 0x26

/**
 * @brief VMCALL to send general buffers from debuggee user-mode
 * to the debugger
 * 
 */
#define VMCALL_SEND_GENERAL_BUFFER_TO_DEBUGGER 0x27

/**
 * @brief VMCALL to cause vm-exit and halt the system because
 * of triggering an event
 * 
 */
#define VMCALL_VM_EXIT_HALT_SYSTEM_AS_A_RESULT_OF_TRIGGERING_EVENT 0x28

/**
 * @brief VMCALL to clear rdtsc exiting bit ONLY in the case of disabling
 * the events for !tsc command
 * 
 */
#define VMCALL_DISABLE_RDTSC_EXITING_ONLY_FOR_TSC_EVENTS 0x29

/**
 * @brief VMCALL to clear mov 2 hw dr exiting bit ONLY in the case of disabling
 * the events for !dr command
 * 
 */
#define VMCALL_DISABLE_MOV_TO_HW_DR_EXITING_ONLY_FOR_DR_EVENTS 0x2a

/**
 * @brief VMCALL to enable mov to CR exiting
 */
#define VMCALL_ENABLE_MOV_TO_CONTROL_REGS_EXITING 0x2b

/**
 * @brief VMCALL to disable mov to CR exiting
 *
 */
#define VMCALL_DISABLE_MOV_TO_CONTROL_REGS_EXITING 0x2c

/**
 * @brief VMCALL to clear mov 2 cr exiting bit ONLY in the case of disabling
 * the events for !crwrite command
 *
 */
#define VMCALL_DISABLE_MOV_TO_CR_EXITING_ONLY_FOR_CR_EVENTS 0x2d

//////////////////////////////////////////////////
//				    Functions					//
//////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// Private Interfaces
//

static NTSTATUS
VmxHypervVmcallHandler(_Inout_ PGUEST_REGS GuestRegs);

/**
 * @brief Handle vm-exits of VMCALLs
 * 
 * @param GuestRegs 
 * @return NTSTATUS 
 */
NTSTATUS
VmxHandleVmcallVmExit(_In_ UINT32         CoreIndex,
                      _Inout_ PGUEST_REGS GuestRegs);

/**
 * @brief Main handler for VMCALLs
 * 
 * @param VmcallNumber 
 * @param OptionalParam1 
 * @param OptionalParam2 
 * @param OptionalParam3 
 * @return NTSTATUS 
 */
NTSTATUS
VmxVmcallHandler(_In_ UINT32         CurrentCoreIndex,
                 _In_ UINT64         VmcallNumber,
                 _In_ UINT64         OptionalParam1,
                 _In_ UINT64         OptionalParam2,
                 _In_ UINT64         OptionalParam3,
                 _Inout_ PGUEST_REGS GuestRegs);

/**
 * @brief Test function which shows a message to test a successful VMCALL
 * 
 * @param Param1 
 * @param Param2 
 * @param Param3 
 * @return NTSTATUS 
 */
NTSTATUS
VmcallTest(_In_ UINT64 Param1,
           _In_ UINT64 Param2,
           _In_ UINT64 Param3);
