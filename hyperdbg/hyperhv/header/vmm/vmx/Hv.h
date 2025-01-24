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
HvSetGuestSelector(PVOID GdtBase, UINT32 SegmentRegister, UINT16 Selector);

/**
 * @brief Returns the Cpu Based and Secondary Processor Based Controls and other
 * controls based on hardware support
 *
 * @param Ctl
 * @param Msr
 * @return UINT32
 */
UINT32
HvAdjustControls(UINT32 Ctl, UINT32 Msr);

/**
 * @brief Handle Cpuid
 *
 * @param VCpu
 * @return VOID
 */
VOID
HvHandleCpuid(VIRTUAL_MACHINE_STATE * VCpu);

/**
 * @brief Fill guest selector data
 *
 * @param GdtBase
 * @param SegmentRegister
 * @param Selector
 * @return VOID
 */
VOID
HvFillGuestSelectorData(PVOID GdtBase, UINT32 SegmentRegister, UINT16 Selector);

/**
 * @brief Handle Guest's Control Registers Access
 *
 * @param VCpu
 * @return VOID
 */
VOID
HvHandleControlRegisterAccess(VIRTUAL_MACHINE_STATE *         VCpu,
                              VMX_EXIT_QUALIFICATION_MOV_CR * CrExitQualification);

/**
 * @brief Resume GUEST_RIP to next instruction
 *
 * @return VOID
 */
VOID
HvResumeToNextInstruction();

/**
 * @brief Suppress the incrementation of RIP
 *
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
extern inline VOID
HvSuppressRipIncrement(VIRTUAL_MACHINE_STATE * VCpu);

/**
 * @brief Perform the incrementation of RIP
 *
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
extern inline VOID
HvPerformRipIncrement(VIRTUAL_MACHINE_STATE * VCpu);

/**
 * @brief Set or unset the monitor trap flags
 *
 * @param Set
 * @return VOID
 */
VOID
HvSetMonitorTrapFlag(BOOLEAN Set);

/**
 * @brief Set the rflag's trap flag
 *
 * @param Set Set or unset the TF
 *
 * @return VOID
 */
VOID
HvSetRflagTrapFlag(BOOLEAN Set);

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
HvSetMovToCr3Vmexit(VIRTUAL_MACHINE_STATE * VCpu, BOOLEAN Set);

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
 * @brief Set Page Modification Logging Enable bit
 *
 * @param Set Set or unset the PML
 * @return VOID
 */
VOID
HvSetPmlEnableFlag(BOOLEAN Set);

/**
 * @brief Set Mode-based Execution Control (MBEC) Enable bit
 *
 * @param Set Set or unset the MBEC
 * @return VOID
 */
VOID
HvSetModeBasedExecutionEnableFlag(BOOLEAN Set);

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
 * @param VCpu
 * @return VOID
 */
VOID
HvHandleMovDebugRegister(VIRTUAL_MACHINE_STATE * VCpu);

/**
 * @brief Set the Mov to Debug Registers Exiting
 *
 * @param Set
 * @return VOID
 */
VOID
HvSetMovDebugRegsExiting(VIRTUAL_MACHINE_STATE * VCpu, BOOLEAN Set);

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
HvSetExceptionBitmap(VIRTUAL_MACHINE_STATE * VCpu, UINT32 IdtIndex);

/**
 * @brief Unset exception bitmap in VMCS
 * @details Should be called in vmx-root
 *
 * @param IdtIndex
 * @return VOID
 */
VOID
HvUnsetExceptionBitmap(VIRTUAL_MACHINE_STATE * VCpu, UINT32 IdtIndex);

/**
 * @brief Set the External Interrupt Exiting
 *
 * @param Set
 * @return VOID
 */
VOID
HvSetExternalInterruptExiting(VIRTUAL_MACHINE_STATE * VCpu, BOOLEAN Set);

/**
 * @brief Checks to enable and reinject previous interrupts
 *
 * @param VCpu The virtual processor's state
 * @param Set Set or unset the External Interrupt Exiting
 * @return VOID
 */
VOID
HvEnableAndCheckForPreviousExternalInterrupts(VIRTUAL_MACHINE_STATE * VCpu);

/**
 * @brief Set the RDTSC/P Exiting
 *
 * @param Set
 * @return VOID
 */
VOID
HvSetRdtscExiting(VIRTUAL_MACHINE_STATE * VCpu, BOOLEAN Set);

/**
 * @brief Read CS selector
 *
 * @return UINT16
 */
UINT16
HvGetCsSelector();

/**
 * @brief Read guest's RFLAGS
 *
 * @return UINT64
 */
UINT64
HvGetRflags();

/**
 * @brief Set guest's RFLAGS
 * @param Rflags
 *
 * @return VOID
 */
VOID
HvSetRflags(UINT64 Rflags);

/**
 * @brief Read guest's RIP
 *
 * @return UINT64
 */
UINT64
HvGetRip();

/**
 * @brief Set guest's RIP
 * @param Rip
 *
 * @return VOID
 */
VOID
HvSetRip(UINT64 Rip);

/**
 * @brief Read guest's interruptibility state
 *
 * @return UINT64
 */
UINT64
HvGetInterruptibilityState();

/**
 * @brief Clear STI and MOV SS bits
 *
 * @return UINT64
 */
UINT64
HvClearSteppingBits(UINT64 Interruptibility);

/**
 * @brief Set guest's interruptibility state
 * @param InterruptibilityState
 *
 * @return VOID
 */
VOID
HvSetInterruptibilityState(UINT64 InterruptibilityState);

/**
 * @brief Inject pending external interrupts
 *
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
HvInjectPendingExternalInterrupts(VIRTUAL_MACHINE_STATE * VCpu);

/**
 * @brief Check and enable external interrupts
 *
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
HvCheckAndEnableExternalInterrupts(VIRTUAL_MACHINE_STATE * VCpu);

/**
 * @brief Disable external-interrupts and interrupt window
 *
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
HvDisableExternalInterruptsAndInterruptWindow(VIRTUAL_MACHINE_STATE * VCpu);

/**
 * @brief Initializes the hypervisor
 * @param VmmCallbacks
 *
 * @return BOOLEAN
 */
BOOLEAN
HvInitVmm(VMM_CALLBACKS * VmmCallbacks);

/**
 * @brief Enables MTF and adjust external interrupt state
 *
 * @return VOID
 */
VOID
HvEnableMtfAndChangeExternalInterruptState(VIRTUAL_MACHINE_STATE * VCpu);

/**
 * @brief Adjust external interrupt state
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
HvPreventExternalInterrupts(VIRTUAL_MACHINE_STATE * VCpu);

/**
 * @brief Get the guest state of pending debug exceptions
 *
 * @return UINT64
 */
UINT64
HvGetPendingDebugExceptions();

/**
 * @brief Set the guest state of pending debug exceptions
 * @param Value The new state
 *
 * @return VOID
 */
VOID
HvSetPendingDebugExceptions(UINT64 Value);

/**
 * @brief Get the guest state of IA32_DEBUGCTL
 *
 * @return UINT64
 */
UINT64
HvGetDebugctl();

/**
 * @brief Set the guest state of IA32_DEBUGCTL
 * @param Value The new state
 *
 * @return VOID
 */
VOID
HvSetDebugctl(UINT64 Value);

/**
 * @brief Handle the case when the trap flag is set, and
 * we need to inject the single-step exception right
 * after vm-entry
 *
 * @return VOID
 */
VOID
HvHandleTrapFlag();