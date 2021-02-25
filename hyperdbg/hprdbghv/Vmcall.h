/**
 * @file Vmcall.h
 * @author Sina Karvandi (sina@rayanfam.com)
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
#define VMCALL_TEST 0x1

/**
 * @brief VMCALL to Call VMXOFF to turn off the hypervisor
 * 
 */
#define VMCALL_VMXOFF 0x2

/**
 * @brief VMCALL to Hook Change the attribute bits of the EPT Table
 * 
 */
#define VMCALL_CHANGE_PAGE_ATTRIB 0x3

/**
 * @brief VMCALL to invalidate EPT (All Contexts)
 * 
 */
#define VMCALL_INVEPT_ALL_CONTEXTS 0x4

/**
 * @brief VMCALL to invalidate EPT (A Single Context)
 * 
 */
#define VMCALL_INVEPT_SINGLE_CONTEXT 0x5

/**
 * @brief VMCALL to remove a all physical addresses from hook list
 * 
 */
#define VMCALL_UNHOOK_ALL_PAGES 0x6

/**
 * @brief VMCALL to remove a single physical address from hook list
 * 
 */
#define VMCALL_UNHOOK_SINGLE_PAGE 0x7

/**
 * @brief VMCALL to enable syscall hook using EFER SCE bit
 * 
 */
#define VMCALL_ENABLE_SYSCALL_HOOK_EFER 0x8

/**
 * @brief VMCALL to disable syscall hook using EFER SCE bit
 * 
 */
#define VMCALL_DISABLE_SYSCALL_HOOK_EFER 0x9

/**
 * @brief VMCALL to change MSR Bitmap Read
 * 
 */
#define VMCALL_CHANGE_MSR_BITMAP_READ 0xa

/**
 * @brief VMCALL to change MSR Bitmap Write
 * 
 */
#define VMCALL_CHANGE_MSR_BITMAP_WRITE 0xb

/**
 * @brief VMCALL to enable rdtsc/rdtscp exiting in primary cpu-based controls
 * 
 */
#define VMCALL_SET_RDTSC_EXITING 0xc

/**
 * @brief VMCALL to enable rdpmc exiting in primary cpu-based controls
 * 
 */
#define VMCALL_SET_RDPMC_EXITING 0xd

/**
 * @brief VMCALL to set exception bitmap on VMCS
 * 
 */
#define VMCALL_SET_EXCEPTION_BITMAP 0xe

/**
 * @brief VMCALL to enable mov to debug registers exiting
 * 
 */
#define VMCALL_ENABLE_MOV_TO_DEBUG_REGS_EXITING 0xf

/**
 * @brief VMCALL to enable external interrupt exiting
 * 
 */
#define VMCALL_ENABLE_EXTERNAL_INTERRUPT_EXITING 0x10

/**
 * @brief VMCALL to change I/O Bitmaps (A & B)
 * 
 */
#define VMCALL_CHANGE_IO_BITMAP 0x11

/**
 * @brief VMCALL to put hidden breakpoints (using EPT)
 * 
 */
#define VMCALL_SET_HIDDEN_CC_BREAKPOINT 0x12

/**
 * @brief VMCALL to enable breakpoints on exception bitmaps
 * 
 */
#define VMCALL_ENABLE_BREAKPOINT_ON_EXCEPTION_BITMAP 0x13

/**
 * @brief VMCALL to disable breakpoints on exception bitmaps
 * 
 */
#define VMCALL_DISABLE_BREAKPOINT_ON_EXCEPTION_BITMAP 0x14

/**
 * @brief VMCALL to disable rdtsc/rdtscp exiting in primary cpu-based controls
 * 
 */
#define VMCALL_UNSET_RDTSC_EXITING 0x15

/**
 * @brief VMCALL to disable external interrupt exiting
 * 
 */
#define VMCALL_DISABLE_EXTERNAL_INTERRUPT_EXITING 0x16

/**
 * @brief VMCALL to disable rdpmc exiting in primary cpu-based controls
 * 
 */
#define VMCALL_UNSET_RDPMC_EXITING 0x17

/**
 * @brief VMCALL to disable mov to debug registers exiting
 * 
 */
#define VMCALL_DISABLE_MOV_TO_DEBUG_REGS_EXITING 0x18

/**
 * @brief VMCALL to reset MSR Bitmap Read
 * 
 */
#define VMCALL_RESET_MSR_BITMAP_READ 0x19

/**
 * @brief VMCALL to reset MSR Bitmap Write
 * 
 */
#define VMCALL_RESET_MSR_BITMAP_WRITE 0x1a

/**
 * @brief VMCALL to reset exception bitmap on VMCS
 * 
 */
#define VMCALL_RESET_EXCEPTION_BITMAP 0x1b

/**
 * @brief VMCALL to reset I/O Bitmaps (A & B)
 * 
 */
#define VMCALL_RESET_IO_BITMAP 0x1c

/**
 * @brief VMCALL to enable cr3 exiting
 * 
 */
#define VMCALL_ENABLE_MOV_TO_CR3_EXITING 0x1d

/**
 * @brief VMCALL to disable cr3 exiting
 * 
 */
#define VMCALL_DISABLE_MOV_TO_CR3_EXITING 0x1e

/**
 * @brief VMCALL to unset exception bitmap on VMCS
 * 
 */
#define VMCALL_UNSET_EXCEPTION_BITMAP 0x1f

/**
 * @brief VMCALL to set VM-entry LOAD DEBUG CONTROLS
 * @details This control determines whether DR7 and the
 * IA32_DEBUGCTL MSR are loaded on VM entry.
 * 
 */
#define VMCALL_SET_VM_ENTRY_LOAD_DEBUG_CONTROLS 0x20

/**
 * @brief VMCALL to unset VM-entry LOAD DEBUG CONTROLS
 * @details This control determines whether DR7 and the
 * IA32_DEBUGCTL MSR are loaded on VM entry.
 * 
 */
#define VMCALL_UNSET_VM_ENTRY_LOAD_DEBUG_CONTROLS 0x21

/**
 * @brief VMCALL to set VM-exit SAVE DEBUG CONTROLS
 * @details This control determines whether DR7 and the
 * IA32_DEBUGCTL MSR are saved on VM exit.
 * 
 */
#define VMCALL_SET_VM_EXIT_SAVE_DEBUG_CONTROLS 0x22

/**
 * @brief VMCALL to unset VM-exit SAVE DEBUG CONTROLS
 * @details This control determines whether DR7 and the
 * IA32_DEBUGCTL MSR are saved on VM exit.
 * 
 */
#define VMCALL_UNSET_VM_EXIT_SAVE_DEBUG_CONTROLS 0x23

/**
 * @brief VMCALL to cause vm-exit and halt the system
 * 
 */
#define VMCALL_VM_EXIT_HALT_SYSTEM 0x24

/**
 * @brief VMCALL to cause vm-exit on NMIs
 * 
 */
#define VMCALL_SET_VM_EXIT_ON_NMIS 0x25

/**
 * @brief VMCALL to not cause vm-exit on NMIs
 * 
 */
#define VMCALL_UNSET_VM_EXIT_ON_NMIS 0x26

/**
 * @brief VMCALL to cause vm-exit and set the cr3
 * 
 */
#define VMCALL_VM_EXIT_HALT_SYSTEM_AND_CHANGE_CR3 0x27

/**
 * @brief VMCALL to signal debugger that debuggee finished
 * execution of the command
 * 
 */
#define VMCALL_SIGNAL_DEBUGGER_EXECUTION_FINISHED 0x28

/**
 * @brief VMCALL to send messages to the debugger
 * 
 */
#define VMCALL_SEND_MESSAGES_TO_DEBUGGER 0x29

/**
 * @brief VMCALL to send general buffers from debuggee user-mode
 * to the debugger
 * 
 */
#define VMCALL_SEND_GENERAL_BUFFER_TO_DEBUGGER 0x2a

//////////////////////////////////////////////////
//				    Functions					//
//////////////////////////////////////////////////

/**
 * @brief Handle vm-exits of VMCALLs
 * 
 * @param GuestRegs 
 * @return NTSTATUS 
 */
NTSTATUS
VmxHandleVmcallVmExit(PGUEST_REGS GuestRegs, UINT32 CoreIndex);

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
VmxVmcallHandler(UINT64      VmcallNumber,
                 UINT64      OptionalParam1,
                 UINT64      OptionalParam2,
                 UINT64      OptionalParam3,
                 PGUEST_REGS GuestRegs,
                 UINT32      CurrentCoreIndex);

/**
 * @brief Test function which shows a message to test a successfull VMCALL
 * 
 * @param Param1 
 * @param Param2 
 * @param Param3 
 * @return NTSTATUS 
 */
NTSTATUS
VmcallTest(UINT64 Param1, UINT64 Param2, UINT64 Param3);
