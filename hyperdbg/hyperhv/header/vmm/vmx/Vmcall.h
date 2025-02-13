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
 * @brief VMCALL to restore a single EPT entry and invalidate EPT cache
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
 * RESETTING (CLEARING) EXCEPTION EVENTS
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
#define VMCALL_SET_VM_EXIT_SAVE_DEBUG_CONTROLS 0x00000020

/**
 * @brief VMCALL to unset VM-exit SAVE DEBUG CONTROLS
 * @details This control determines whether DR7 and the
 * IA32_DEBUGCTL MSR are saved on VM exit.
 *
 */
#define VMCALL_UNSET_VM_EXIT_SAVE_DEBUG_CONTROLS 0x00000021

/**
 * @brief VMCALL to cause vm-exit on NMIs
 *
 */
#define VMCALL_SET_VM_EXIT_ON_NMIS 0x00000022

/**
 * @brief VMCALL to not cause vm-exit on NMIs
 *
 */
#define VMCALL_UNSET_VM_EXIT_ON_NMIS 0x00000023

/**
 * @brief VMCALL to clear rdtsc exiting bit ONLY in the case of disabling
 * the events for !tsc command
 *
 */
#define VMCALL_DISABLE_RDTSC_EXITING_ONLY_FOR_TSC_EVENTS 0x00000024

/**
 * @brief VMCALL to clear mov 2 hw dr exiting bit ONLY in the case of disabling
 * the events for !dr command
 *
 */
#define VMCALL_DISABLE_MOV_TO_HW_DR_EXITING_ONLY_FOR_DR_EVENTS 0x00000025

/**
 * @brief VMCALL to enable mov to CR exiting
 */
#define VMCALL_ENABLE_MOV_TO_CONTROL_REGS_EXITING 0x00000026

/**
 * @brief VMCALL to disable mov to CR exiting
 *
 */
#define VMCALL_DISABLE_MOV_TO_CONTROL_REGS_EXITING 0x00000027

/**
 * @brief VMCALL to clear mov 2 cr exiting bit ONLY in the case of disabling
 * the events for !crwrite command
 *
 */
#define VMCALL_DISABLE_MOV_TO_CR_EXITING_ONLY_FOR_CR_EVENTS 0x00000028

/**
 * @brief VMCALL to enable dirty logging (PML) mechanism
 *
 */
#define VMCALL_ENABLE_DIRTY_LOGGING_MECHANISM 0x00000029

/**
 * @brief VMCALL to disable dirty logging (PML) mechanism
 *
 */
#define VMCALL_DISABLE_DIRTY_LOGGING_MECHANISM 0x0000002a

/**
 * @brief VMCALL to change EPTP to an MBEC-supported EPTP
 *
 */
#define VMCALL_CHANGE_TO_MBEC_SUPPORTED_EPTP 0x0000002b

/**
 * @brief VMCALL to restore EPTP to normal EPTP
 *
 */
#define VMCALL_RESTORE_TO_NORMAL_EPTP 0x0000002c

/**
 * @brief VMCALL to enable/disable MBEC
 *
 */
#define VMCALL_DISABLE_OR_ENABLE_MBEC 0x0000002d

/**
 * @brief VMCALL to bypass caching policies to read MMIO
 *
 */
#define VMCALL_READ_PHYSICAL_MEM_BYPASS_CACHING_POLICIES 0x0000002e // not used

/**
 * @brief VMCALL to bypass caching policies to write MMIO
 *
 */
#define VMCALL_WRITE_PHYSICAL_MEM_BYPASS_CACHING_POLICIES 0x0000002f // not used

/**
 * @brief VMCALL to read physical memory
 *
 */
#define VMCALL_READ_PHYSICAL_MEMORY 0x00000030

/**
 * @brief VMCALL to write physical memory
 *
 */
#define VMCALL_WRITE_PHYSICAL_MEMORY 0x00000031

//////////////////////////////////////////////////
//				    Functions					//
//////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// Private Interfaces
//

static NTSTATUS
VmxHypervVmcallHandler(_Inout_ VIRTUAL_MACHINE_STATE * VCpu, _Inout_ PGUEST_REGS GuestRegs);

/**
 * @brief Handle vm-exits of VMCALLs
 *
 * @param VCpu
 * @return NTSTATUS
 */
NTSTATUS
VmxHandleVmcallVmExit(_Inout_ VIRTUAL_MACHINE_STATE * VCpu);

/**
 * @brief Direct Vmcall Handler
 *
 * @param VCpu The virtual processor's state
 * @param VmcallNumber The number of the VMCALL
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
VmxVmcallDirectVmcallHandler(VIRTUAL_MACHINE_STATE *    VCpu,
                             UINT64                     VmcallNumber,
                             DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions);

/**
 * @brief Main handler for VMCALLs
 *
 * @param VCpu
 * @param VmcallNumber
 * @param OptionalParam1
 * @param OptionalParam2
 * @param OptionalParam3
 * @return NTSTATUS
 */
NTSTATUS
VmxVmcallHandler(_Inout_ VIRTUAL_MACHINE_STATE * VCpu,
                 _In_ UINT64                     VmcallNumber,
                 _In_ UINT64                     OptionalParam1,
                 _In_ UINT64                     OptionalParam2,
                 _In_ UINT64                     OptionalParam3);

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
