/**
 * @file Events.c
 * @author Sina Karvandi (sina@rayanfam.com)
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
EventInjectInterruption(INTERRUPT_TYPE InterruptionType, EXCEPTION_VECTORS Vector, BOOLEAN DeliverErrorCode, ULONG32 ErrorCode)
{
    INTERRUPT_INFO Inject = {0};
    Inject.Valid          = TRUE;
    Inject.InterruptType  = InterruptionType;
    Inject.Vector         = Vector;
    Inject.DeliverCode    = DeliverErrorCode;
    __vmx_vmwrite(VM_ENTRY_INTR_INFO, Inject.Flags);

    if (DeliverErrorCode)
    {
        __vmx_vmwrite(VM_ENTRY_EXCEPTION_ERROR_CODE, ErrorCode);
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
    EventInjectInterruption(INTERRUPT_TYPE_SOFTWARE_EXCEPTION, EXCEPTION_VECTOR_BREAKPOINT, FALSE, 0);
    UINT32 ExitInstrLength;
    __vmx_vmread(VM_EXIT_INSTRUCTION_LEN, &ExitInstrLength);
    __vmx_vmwrite(VM_ENTRY_INSTRUCTION_LEN, ExitInstrLength);
}

/**
 * @brief Inject #GP to the guest (Event Injection)
 * 
 * @return VOID 
 */
VOID
EventInjectGeneralProtection()
{
    EventInjectInterruption(INTERRUPT_TYPE_HARDWARE_EXCEPTION, EXCEPTION_VECTOR_GENERAL_PROTECTION_FAULT, TRUE, 0);
    UINT32 ExitInstrLength;
    __vmx_vmread(VM_EXIT_INSTRUCTION_LEN, &ExitInstrLength);
    __vmx_vmwrite(VM_ENTRY_INSTRUCTION_LEN, ExitInstrLength);
}

/**
 * @brief Inject #UD to the guest (Invalid Opcode - Undefined Opcode)
 * 
 * @return VOID 
 */
VOID
EventInjectUndefinedOpcode(UINT32 CurrentProcessorIndex)
{
    EventInjectInterruption(INTERRUPT_TYPE_HARDWARE_EXCEPTION, EXCEPTION_VECTOR_UNDEFINED_OPCODE, FALSE, 0);

    //
    // Suppress RIP increment
    //
    g_GuestState[CurrentProcessorIndex].IncrementRip = FALSE;
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
    EventInjectInterruption(INTERRUPT_TYPE_HARDWARE_EXCEPTION, EXCEPTION_VECTOR_PAGE_FAULT, TRUE, ErrorCode.All);
}
