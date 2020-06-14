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
#include "Vmx.h"

//////////////////////////////////////////////////
//				    Constants					//
//////////////////////////////////////////////////

#define VMCALL_TEST                              0x1  // Test VMCALL
#define VMCALL_VMXOFF                            0x2  // Call VMXOFF to turn off the hypervisor
#define VMCALL_CHANGE_PAGE_ATTRIB                0x3  // VMCALL to Hook Change the attribute bits of the EPT Table
#define VMCALL_INVEPT_ALL_CONTEXTS               0x4  // VMCALL to invalidate EPT (All Contexts)
#define VMCALL_INVEPT_SINGLE_CONTEXT             0x5  // VMCALL to invalidate EPT (A Single Context)
#define VMCALL_UNHOOK_ALL_PAGES                  0x6  // VMCALL to remove a all physical addresses from hook list
#define VMCALL_UNHOOK_SINGLE_PAGE                0x7  // VMCALL to remove a single physical address from hook list
#define VMCALL_ENABLE_SYSCALL_HOOK_EFER          0x8  // VMCALL to enable syscall hook using EFER SCE bit
#define VMCALL_DISABLE_SYSCALL_HOOK_EFER         0x9  // VMCALL to disable syscall hook using EFER SCE bit
#define VMCALL_CHANGE_MSR_BITMAP_READ            0xa  // VMCALL to change MSR Bitmap Read
#define VMCALL_CHANGE_MSR_BITMAP_WRITE           0xb  // VMCALL to change MSR Bitmap Write
#define VMCALL_SET_RDTSC_EXITING                 0xc  // VMCALL to enables rdtsc/rdtscp exiting in primary cpu-based controls
#define VMCALL_SET_RDPMC_EXITING                 0xd  // VMCALL to enables rdpmc exiting in primary cpu-based controls
#define VMCALL_SET_EXCEPTION_BITMAP              0xe  // VMCALL to set exception bitmap on VMCS
#define VMCALL_ENABLE_MOV_TO_DEBUG_REGS_EXITING  0xf  // VMCALL to enable mov to debug registers exiting
#define VMCALL_ENABLE_EXTERNAL_INTERRUPT_EXITING 0x10 // VMCALL to enable external interrupt exiting
#define VMCALL_CHANGE_IO_BITMAP                  0x11 // VMCALL to I/O Bitmaps (A & B)

//////////////////////////////////////////////////
//				    Functions					//
//////////////////////////////////////////////////

/* Handle vm-exits of VMCALLs */
NTSTATUS
VmxHandleVmcallVmExit(PGUEST_REGS GuestRegs);

/* Main handler for VMCALLs */
NTSTATUS
VmxVmcallHandler(UINT64 VmcallNumber, UINT64 OptionalParam1, UINT64 OptionalParam2, UINT64 OptionalParam3);

/* Test function which shows a message to test a successfull VMCALL */
NTSTATUS
VmcallTest(UINT64 Param1, UINT64 Param2, UINT64 Param3);
