/**
 * @file Hv.h
 * @author Sina Karvandi (sina@hyperdbg.org)
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

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

/**
 * @brief Set Guest Selector Registers
 * 
 * @param GdtBase 
 * @param SegmentRegister 
 * @param Selector 
 * @return BOOLEAN 
 */
BOOLEAN
HvSetGuestSelector(PVOID GdtBase, ULONG SegmentRegister, UINT16 Selector);

/**
 * @brief Returns the Cpu Based and Secondary Processor Based Controls and other 
 * controls based on hardware support
 * 
 * @param Ctl 
 * @param Msr 
 * @return ULONG 
 */
ULONG
HvAdjustControls(ULONG Ctl, ULONG Msr);

/**
 * @brief Handle Cpuid
 * 
 * @param RegistersState 
 * @return VOID 
 */
VOID
HvHandleCpuid(PGUEST_REGS RegistersState);

/**
 * @brief Fill guest selector data
 * 
 * @param GdtBase 
 * @param SegmentRegister 
 * @param Selector 
 * @return VOID 
 */
VOID
HvFillGuestSelectorData(PVOID GdtBase, ULONG SegmentRegister, UINT16 Selector);

/**
 * @brief Handle Guest's Control Registers Access
 * 
 * @param GuestState 
 * @return VOID 
 */
VOID
HvHandleControlRegisterAccess(PGUEST_REGS GuestState, UINT32 ProcessorIndex);

/**
 * @brief Resume GUEST_RIP to next instruction
 * 
 * @return VOID 
 */
VOID
HvResumeToNextInstruction();

/**
 * @brief Set or unset the monitor trap flags
 * 
 * @param Set 
 * @return VOID 
 */
VOID
HvSetMonitorTrapFlag(BOOLEAN Set);

/**
 * @brief Set LOAD DEBUG CONTROLS on Vm-entry controls
 * 
 * @param Set Set or unset 
 * @return VOID 
 */
VOID
HvSetLoadDebugControls(BOOLEAN Set);

/**
 * @brief Set SAVE DEBUG CONTROLS on Vm-exit controls
 * 
 * @param Set Set or unset 
 * @return VOID 
 */
VOID
HvSetSaveDebugControls(BOOLEAN Set);

/**
 * @brief Reset GDTR/IDTR and other old when you do vmxoff as the patchguard 
 * will detect them left modified
 * 
 * @return VOID 
 */
VOID
HvRestoreRegisters();

/**
 * @brief Set vm-exit for rdpmc instructions
 * 
 * @param Set 
 * @return VOID 
 */
VOID
HvSetPmcVmexit(BOOLEAN Set);

/**
 * @brief Set vm-exit for mov-to-cr0/4
 *
 * @param Set
 * @param ControlRegister
 * @param MaskRegister
 * @return VOID
 */
VOID
HvSetMovControlRegsExiting(BOOLEAN Set, UINT64 ControlRegister, UINT64 MaskRegister);

/**
 * @brief Set vm-exit for mov-to-cr3
 * 
 * @param Set 
 * @return VOID 
 */
VOID
HvSetMovToCr3Vmexit(BOOLEAN Set);

/**
 * @brief Write to the exception bitmap
 * 
 * @param BitmapMask 
 * @return VOID 
 */
VOID
HvWriteExceptionBitmap(UINT32 BitmapMask);

/**
 * @brief Read the exception bitmap
 * 
 * @return UINT32 
 */
UINT32
HvReadExceptionBitmap();

/**
 * @brief Set Interrupt-window exiting
 * 
 * @param Set 
 * @return VOID 
 */
VOID
HvSetInterruptWindowExiting(BOOLEAN Set);

/**
 * @brief Set NMI-window exiting
 * 
 * @param Set 
 * @return VOID 
 */
VOID
HvSetNmiWindowExiting(BOOLEAN Set);

/**
 * @brief Handle Mov to Debug Registers Exitings
 * 
 * @param ProcessorIndex 
 * @param Regs 
 * @return VOID 
 */
VOID
HvHandleMovDebugRegister(UINT32 ProcessorIndex, PGUEST_REGS Regs);

/**
 * @brief Set the Mov to Debug Registers Exiting
 * 
 * @param Set 
 * @return VOID 
 */
VOID
HvSetMovDebugRegsExiting(BOOLEAN Set);

/**
 * @brief Set the NMI Exiting
 * 
 * @param Set 
 * @return VOID 
 */
VOID
HvSetNmiExiting(BOOLEAN Set);

/**
 * @brief Set the VMX Preemptiom Timer
 * 
 * @param Set 
 * @return VOID 
 */
VOID
HvSetVmxPreemptionTimerExiting(BOOLEAN Set);

/**
 * @brief Set exception bitmap in VMCS 
 * @details Should be called in vmx-root
 * 
 * @param IdtIndex 
 * @return VOID 
 */
VOID
HvSetExceptionBitmap(UINT32 IdtIndex);

/**
 * @brief Unset exception bitmap in VMCS 
 * @details Should be called in vmx-root
 * 
 * @param IdtIndex  
 * @return VOID 
 */
VOID
HvUnsetExceptionBitmap(UINT32 IdtIndex);

/**
 * @brief Set the External Interrupt Exiting
 * 
 * @param Set 
 * @return VOID 
 */
VOID
HvSetExternalInterruptExiting(BOOLEAN Set);

/**
 * @brief Set the RDTSC/P Exiting
 * 
 * @param Set 
 * @return VOID 
 */
VOID
HvSetRdtscExiting(BOOLEAN Set);
