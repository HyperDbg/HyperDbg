/**
 * @file HypervisorRoutines.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief This file contains the headers for Hypervisor Routines which have to be called by external codes
 * @details DO NOT DIRECTLY CALL VMX FUNCTIONS, instead use these routines
 * 
 * @version 0.1
 * @date 2020-04-11
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */

#pragma once
#include "Msr.h"
#include "Vmx.h"

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

/* Detect whether Vmx is supported or not */
BOOLEAN
HvIsVmxSupported();
/* Initialize Vmx */
BOOLEAN
HvVmxInitialize();
/* Allocates Vmx regions for all logical cores (Vmxon region and Vmcs region) */
BOOLEAN
VmxDpcBroadcastAllocateVmxonRegions(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);
/* Set Guest Selector Registers */
BOOLEAN
HvSetGuestSelector(PVOID GdtBase, ULONG SegmentRegister, USHORT Selector);
/* Get Segment Descriptor */
BOOLEAN
HvGetSegmentDescriptor(PSEGMENT_SELECTOR SegmentSelector, USHORT Selector, PUCHAR GdtBase);
/* Set Msr Bitmap */
BOOLEAN
HvSetMsrBitmap(ULONG64 Msr, INT ProcessorID, BOOLEAN ReadDetection, BOOLEAN WriteDetection);
/* UnSet Msr Bitmap */
BOOLEAN
HvUnSetMsrBitmap(ULONG64 Msr, INT ProcessorID, BOOLEAN ReadDetection, BOOLEAN WriteDetection);
/* Returns the Cpu Based and Secondary Processor Based Controls and other controls based on hardware support */
ULONG
HvAdjustControls(ULONG Ctl, ULONG Msr);
/* Notify all cores about EPT Invalidation */
VOID
HvNotifyAllToInvalidateEpt();
/* Handle Cpuid */
VOID
HvHandleCpuid(PGUEST_REGS RegistersState);
/* Fill guest selector data */
VOID
HvFillGuestSelectorData(PVOID GdtBase, ULONG SegmentRegister, USHORT Selector);
/* Handle Guest's Control Registers Access */
VOID
HvHandleControlRegisterAccess(PGUEST_REGS GuestState);
/* Handle Guest's Msr read */
VOID
HvHandleMsrRead(PGUEST_REGS GuestRegs);
/* Handle Guest's Msr write */
VOID
HvHandleMsrWrite(PGUEST_REGS GuestRegs);
/* Resume GUEST_RIP to next instruction */
VOID
HvResumeToNextInstruction();
/* Invalidate EPT using Vmcall (should be called from Vmx non root mode) */
VOID
HvInvalidateEptByVmcall(UINT64 Context);
/* The broadcast function which initialize the guest */
VOID
HvDpcBroadcastInitializeGuest(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);
/* The broadcast function which terminate the guest */
VOID
HvDpcBroadcastTerminateGuest(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);
/* Terminate Vmx on all logical cores */
VOID
HvTerminateVmx();
/* Set or unset the monitor trap flags */
VOID
HvSetMonitorTrapFlag(BOOLEAN Set);
/* Set the vm-exit on cr3 for finding a process */
VOID
HvSetExitOnCr3Change(BOOLEAN Set);
/* Returns the stack pointer, to change in the case of Vmxoff */
UINT64
HvReturnStackPointerForVmxoff();
/* Returns the instruction pointer, to change in the case of Vmxoff */
UINT64
HvReturnInstructionPointerForVmxoff();
/* Reset GDTR/IDTR and other old when you do vmxoff as the patchguard will detect them left modified */
VOID
HvRestoreRegisters();
/* Remove single hook from the hooked pages list and invalidate TLB */
BOOLEAN
HvPerformPageUnHookSinglePage(UINT64 VirtualAddress);
/* Remove all hooks from the hooked pages list and invalidate TLB */
VOID
HvPerformPageUnHookAllPages();
/* Change MSR Bitmap for read */
VOID
HvPerformMsrBitmapReadChange(UINT64 MsrMask);
/* Change MSR Bitmap for write */
VOID
HvPerformMsrBitmapWriteChange(UINT64 MsrMask);
/* Set vm-exit for tsc instructions (rdtsc/rdtscp) */
VOID
HvSetTscVmexit(BOOLEAN Set);
/* Set vm-exit for rdpmc instructions */
VOID
HvSetPmcVmexit(BOOLEAN Set);
/* Set exception bitmap in VMCS */
VOID
HvSetExceptionBitmap(UINT32 IdtIndex);
/* Unset exception bitmap in VMCS */
VOID
HvUnsetExceptionBitmap(UINT32 IdtIndex);
/* Set Interrupt-window exiting */
VOID
HvSetInterruptWindowExiting(BOOLEAN Set);
/* Set the nmi-Window exiting */
VOID
HvSetNmiWindowExiting(BOOLEAN Set);
/* Handle Mov to Debug Registers Exitings */
VOID
HvHandleMovDebugRegister(UINT32 ProcessorIndex, PGUEST_REGS Regs);
/* Set the Mov to Debug Registers Exiting */
VOID
HvSetMovDebugRegsExiting(BOOLEAN Set);
/* Set the External Interrupt Exiting */
VOID
HvSetExternalInterruptExiting(BOOLEAN Set);
/* Set bits in I/O Bitmap */
BOOLEAN
HvSetIoBitmap(UINT64 Port, UINT32 ProcessorID);
/* Change I/O Bitmap */
VOID
HvPerformIoBitmapChange(UINT64 Port);
