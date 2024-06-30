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

    VmxVmwrite64(VMCS_CTRL_VMENTRY_INTERRUPTION_INFORMATION_FIELD, Inject.Flags);

    if (DeliverErrorCode)
    {
        VmxVmwrite64(VMCS_CTRL_VMENTRY_EXCEPTION_ERROR_CODE, ErrorCode);
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

    VmxVmread32P(VMCS_VMEXIT_INSTRUCTION_LENGTH, &ExitInstrLength);
    VmxVmwrite64(VMCS_CTRL_VMENTRY_INSTRUCTION_LENGTH, ExitInstrLength);
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

    VmxVmread32P(VMCS_VMEXIT_INSTRUCTION_LENGTH, &ExitInstrLength);
    VmxVmwrite64(VMCS_CTRL_VMENTRY_INSTRUCTION_LENGTH, ExitInstrLength);
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
 * @brief Inject NMI to the guest (Event Injection)
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
EventInjectNmi(VIRTUAL_MACHINE_STATE * VCpu)
{
    EventInjectInterruption(INTERRUPT_TYPE_NMI, EXCEPTION_VECTOR_NMI, FALSE, 0);

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
EventInjectPageFaultWithoutErrorCode(UINT64 PageFaultAddress)
{
    PAGE_FAULT_EXCEPTION ErrorCode = {0};

    //
    // Write the page-fault address
    //
    __writecr2(PageFaultAddress);

    //
    // Make the error code
    //
    ErrorCode.Execute        = 0;
    ErrorCode.Present        = 0;
    ErrorCode.UserModeAccess = 0;
    ErrorCode.Write          = 0;

    //
    // Error code is from PAGE_FAULT_ERROR_CODE structure
    //
    EventInjectInterruption(INTERRUPT_TYPE_HARDWARE_EXCEPTION, EXCEPTION_VECTOR_PAGE_FAULT, TRUE, ErrorCode.AsUInt);
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
    UINT32 ErrorCode = 0;

    //
    // Re-inject it
    //
    VmxVmwrite64(VMCS_CTRL_VMENTRY_INTERRUPTION_INFORMATION_FIELD, InterruptExit.AsUInt);

    //
    // re-write error code (if any)
    //
    if (InterruptExit.ErrorCodeValid)
    {
        //
        // Read the error code
        //
        VmxVmread32P(VMCS_VMEXIT_INTERRUPTION_ERROR_CODE, &ErrorCode);

        //
        // Write the error code
        //
        VmxVmwrite64(VMCS_CTRL_VMENTRY_EXCEPTION_ERROR_CODE, ErrorCode);
    }
}

/**
 * @brief inject #PFs to the guest
 *
 * @param VCpu The virtual processor's state
 * @param InterruptExit interrupt info from vm-exit
 * @param PageFaultAddress Page-fault address to be placed to cr2 register
 * @param PageFaultCode Page-fault error code
 *
 * @return VOID
 */
VOID
EventInjectPageFaults(_Inout_ VIRTUAL_MACHINE_STATE *   VCpu,
                      _In_ VMEXIT_INTERRUPT_INFORMATION InterruptExit,
                      _In_ UINT64                       PageFaultAddress,
                      _In_ PAGE_FAULT_EXCEPTION         PageFaultCode)
{
    //
    // *** #PF is treated differently, we have to deal with cr2 too ***
    //

    //
    // Cr2 is used as the page-fault address
    //
    __writecr2(PageFaultAddress);

    HvSuppressRipIncrement(VCpu);

    //
    // Re-inject the interrupt/exception
    //
    VmxVmwrite64(VMCS_CTRL_VMENTRY_INTERRUPTION_INFORMATION_FIELD, InterruptExit.AsUInt);

    //
    // re-write error code (if any)
    //
    if (InterruptExit.ErrorCodeValid)
    {
        //
        // Write the error code
        //
        VmxVmwrite64(VMCS_CTRL_VMENTRY_EXCEPTION_ERROR_CODE, PageFaultCode.AsUInt);
    }
}

/**
 * @brief Inject a range of page-faults
 *
 * @param VCpu The virtual processor's state
 * @param AddressFrom Page-fault address (from)
 * @param AddressTo Page-fault address (to)
 * @param Address Page-fault address
 * @param PageFaultCode Page-fault error code
 *
 * @return VOID
 */
VOID
EventInjectPageFaultRangeAddress(VIRTUAL_MACHINE_STATE * VCpu,
                                 UINT64                  AddressFrom,
                                 UINT64                  AddressTo,
                                 UINT32                  PageFaultCode)
{
    UNREFERENCED_PARAMETER(VCpu);

    //
    // Indicate that the VMM is waiting for interrupt-window to
    // be opened to inject page-fault
    //
    g_WaitingForInterruptWindowToInjectPageFault = TRUE;

    //
    // Set the (from) address for page-fault injection
    //
    g_PageFaultInjectionAddressFrom = AddressFrom;

    //
    // Set the (to) address for page-fault injection
    //
    g_PageFaultInjectionAddressTo = AddressTo;

    //
    // Set the error code for page-fault injection
    //
    g_PageFaultInjectionErrorCode = PageFaultCode;

    //
    // Set interrupt-window exiting to TRUE
    //
    HvSetInterruptWindowExiting(TRUE);
}

/**
 * @brief Inject page-fault with an address as cr2
 *
 * @param VCpu The virtual processor's state
 * @param Address Page-fault address
 * @param PageFaultCode Page-fault error code
 *
 * @return VOID
 */
VOID
EventInjectPageFaultWithCr2(VIRTUAL_MACHINE_STATE * VCpu, UINT64 Address, UINT32 PageFaultCode)
{
    VMEXIT_INTERRUPT_INFORMATION InterruptInfo      = {0};
    PAGE_FAULT_EXCEPTION         PageFaultErrorCode = {0};

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

    //
    // Configure the page-fault error code
    //
    PageFaultErrorCode.AsUInt = PageFaultCode;

    //
    // Inject #PF
    //
    EventInjectPageFaults(VCpu,
                          InterruptInfo,
                          Address,
                          PageFaultErrorCode);
}
