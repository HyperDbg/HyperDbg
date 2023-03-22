/**
 * @file Events.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Functions relating to Exception Bitmap and Event (Interrupt and Exception) Injection
 * @details
 * @version 0.1
 * @date 2020-04-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Injects interruption to a guest
 *
 * @param InterruptionType Type of interrupt
 * @param Vector Vector Number of Interrupt (IDT Index)
 * @param DeliverErrorCode Deliver Error Code or Not
 * @param ErrorCode Error Code (If DeliverErrorCode is true)
 * @return VOID
 */
VOID
EventInjectInterruption(INTERRUPT_TYPE InterruptionType, EXCEPTION_VECTORS Vector, BOOLEAN DeliverErrorCode, UINT32 ErrorCode)
{
    INTERRUPT_INFO Inject       = {0};
    Inject.Fields.Valid         = TRUE;
    Inject.Fields.InterruptType = InterruptionType;
    Inject.Fields.Vector        = Vector;
    Inject.Fields.DeliverCode   = DeliverErrorCode;
    __vmx_vmwrite(VMCS_CTRL_VMENTRY_INTERRUPTION_INFORMATION_FIELD, Inject.Flags);

    if (DeliverErrorCode)
    {
        __vmx_vmwrite(VMCS_CTRL_VMENTRY_EXCEPTION_ERROR_CODE, ErrorCode);
    }
}

/**
 * @brief Inject #BP to the guest (Event Injection)
 *
 * @return VOID
 */
VOID
EventInjectBreakpoint()
{
    UINT32 ExitInstrLength;

    EventInjectInterruption(INTERRUPT_TYPE_SOFTWARE_EXCEPTION, EXCEPTION_VECTOR_BREAKPOINT, FALSE, 0);

    __vmx_vmread(VMCS_VMEXIT_INSTRUCTION_LENGTH, &ExitInstrLength);
    __vmx_vmwrite(VMCS_CTRL_VMENTRY_INSTRUCTION_LENGTH, ExitInstrLength);
}

/**
 * @brief Inject #GP to the guest (Event Injection)
 *
 * @return VOID
 */
VOID
EventInjectGeneralProtection()
{
    UINT32 ExitInstrLength;

    EventInjectInterruption(INTERRUPT_TYPE_HARDWARE_EXCEPTION, EXCEPTION_VECTOR_GENERAL_PROTECTION_FAULT, TRUE, 0);

    __vmx_vmread(VMCS_VMEXIT_INSTRUCTION_LENGTH, &ExitInstrLength);
    __vmx_vmwrite(VMCS_CTRL_VMENTRY_INSTRUCTION_LENGTH, ExitInstrLength);
}

/**
 * @brief Inject #UD to the guest (Invalid Opcode - Undefined Opcode)
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
EventInjectUndefinedOpcode(VIRTUAL_MACHINE_STATE * VCpu)
{
    EventInjectInterruption(INTERRUPT_TYPE_HARDWARE_EXCEPTION, EXCEPTION_VECTOR_UNDEFINED_OPCODE, FALSE, 0);

    //
    // Suppress RIP increment
    //
    HvSuppressRipIncrement(VCpu);
}

/**
 * @brief Inject Debug Breakpoint Exception
 *
 * @return VOID
 */
VOID
EventInjectDebugBreakpoint()
{
    EventInjectInterruption(INTERRUPT_TYPE_HARDWARE_EXCEPTION, EXCEPTION_VECTOR_DEBUG_BREAKPOINT, FALSE, 0);
}

/**
 * @brief Inject #PF to the guest (Page-Fault for EFER Injector)
 *
 * @param PageFaultAddress Address of page fault
 * @return VOID
 */
VOID
EventInjectPageFault(UINT64 PageFaultAddress)
{
    PAGE_FAULT_ERROR_CODE ErrorCode = {0};

    //
    // Write the page-fault address
    //
    __writecr2(PageFaultAddress);

    //
    // Make the error code
    //
    ErrorCode.Fields.Fetch    = 0;
    ErrorCode.Fields.Present  = 0;
    ErrorCode.Fields.Reserved = 0;
    ErrorCode.Fields.User     = 0;
    ErrorCode.Fields.Write    = 0;

    //
    // Error code is from PAGE_FAULT_ERROR_CODE structure
    //
    EventInjectInterruption(INTERRUPT_TYPE_HARDWARE_EXCEPTION, EXCEPTION_VECTOR_PAGE_FAULT, TRUE, ErrorCode.Flags);
}

/**
 * @brief re-inject interrupt or exception to the guest
 *
 * @param InterruptExit interrupt info from vm-exit
 *
 * @return VOID
 */
VOID
EventInjectInterruptOrException(_In_ VMEXIT_INTERRUPT_INFORMATION InterruptExit)
{
    ULONG ErrorCode = 0;

    //
    // Re-inject it
    //
    __vmx_vmwrite(VMCS_CTRL_VMENTRY_INTERRUPTION_INFORMATION_FIELD, InterruptExit.AsUInt);

    //
    // re-write error code (if any)
    //
    if (InterruptExit.ErrorCodeValid)
    {
        //
        // Read the error code
        //
        __vmx_vmread(VMCS_VMEXIT_INTERRUPTION_ERROR_CODE, &ErrorCode);

        //
        // Write the error code
        //
        __vmx_vmwrite(VMCS_CTRL_VMENTRY_EXCEPTION_ERROR_CODE, ErrorCode);
    }
}

/**
 * @brief re-inject interrupt or exception to the guest
 *
 * @param VCpu The virtual processor's state
 * @param Address Page fault address
 *
 * @return VOID
 */
VOID
EventInjectPageFaultWithCr2(VIRTUAL_MACHINE_STATE * VCpu, UINT64 Address)
{
    //
    // Inject #PF
    //
    VMEXIT_INTERRUPT_INFORMATION InterruptInfo = {0};

    //
    // Configure the #PF injection
    //

    //
    // InterruptExit                 [Type: _VMEXIT_INTERRUPT_INFO]
    //
    // [+0x000 ( 7: 0)] Vector           : 0xe [Type: unsigned int]
    // [+0x000 (10: 8)] InterruptionType : 0x3 [Type: unsigned int]
    // [+0x000 (11:11)] ErrorCodeValid   : 0x1 [Type: unsigned int]
    // [+0x000 (12:12)] NmiUnblocking    : 0x0 [Type: unsigned int]
    // [+0x000 (30:13)] Reserved         : 0x0 [Type: unsigned int]
    // [+0x000 (31:31)] Valid            : 0x1 [Type: unsigned int]
    // [+0x000] Flags                    : 0x80000b0e [Type: unsigned int]
    //
    InterruptInfo.Vector           = EXCEPTION_VECTOR_PAGE_FAULT;
    InterruptInfo.InterruptionType = INTERRUPT_TYPE_HARDWARE_EXCEPTION;
    InterruptInfo.ErrorCodeValid   = TRUE;
    InterruptInfo.NmiUnblocking    = FALSE;
    InterruptInfo.Valid            = TRUE;

    IdtEmulationHandlePageFaults(VCpu,
                                 InterruptInfo,
                                 Address,
                                 0x14);
}
