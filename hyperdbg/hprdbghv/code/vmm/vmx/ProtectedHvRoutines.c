/**
 * @file ProtectedHvRoutines.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief File for protected hypervisor resources
 * @details Protected Hypervisor Routines are those resource that 
 * are used in different parts of the debugger or hypervisor,
 * these resources need extra checks to avoid integrity problems
 * 
 * @version 0.1
 * @date 2021-10-04
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "..\hprdbghv\pch.h"

/**
 * @brief Add extra mask to this resource 
 * @details As exception bitmap is a protected resource, this 
 * routine makes sure that modifying exception bitmap won't 
 * break the debugger's integrity
 * 
 * @param CurrentMask The mask that debugger wants to write
 * @param PassOver Adds some pass over to the checks
 * thus we won't check for exceptions
 * 
 * @return UINT32 The actual value that should be written 
 */
UINT32
ProtectedHvExceptionBitmapIntegrityCheck(UINT32 CurrentMask, PROTECTED_HV_RESOURCES_PASSING_OVERS PassOver)
{
    UINT32 CurrentCoreId = 0;

    CurrentCoreId = KeGetCurrentProcessorNumber();
    //
    // Check if the integrity check is because of clearing
    // events or not, if it's for clearing events, the debugger
    // will automatically set
    //
    if (!(PassOver & PASSING_OVER_EXCEPTION_EVENTS))
    {
        //
        // we have to check for !exception events and apply the mask
        //
        CurrentMask |= DebuggerExceptionEventBitmapMask(CurrentCoreId);
    }

    //
    // Check if it's because of disabling !syscall or !sysret commands
    // or not, if it's because of clearing #UD in these events then we
    // can ignore the checking for this command, otherwise, we have to
    // check it
    //
    if (!(PassOver & PASSING_OVER_UD_EXCEPTIONS_FOR_SYSCALL_SYSRET_HOOK))
    {
        //
        // Check if the debugger has events relating to syscall or sysret,
        // if no, we can safely ignore #UDs, otherwise, #UDs should be
        // activated
        //
        if (DebuggerEventListCountByCore(&g_Events->SyscallHooksEferSyscallEventsHead, CurrentCoreId) != 0 ||
            DebuggerEventListCountByCore(&g_Events->SyscallHooksEferSysretEventsHead, CurrentCoreId) != 0)
        {
            //
            // #UDs should be activated
            //
            CurrentMask |= 1 << EXCEPTION_VECTOR_UNDEFINED_OPCODE;
        }
    }

    //
    // Check for kernel debugger (kHyperDbg) presence
    //
    if (g_KernelDebuggerState)
    {
        CurrentMask |= 1 << EXCEPTION_VECTOR_BREAKPOINT;
        CurrentMask |= 1 << EXCEPTION_VECTOR_DEBUG_BREAKPOINT;
    }

    //
    // Check for possible EPT Hooks (Hidden Breakpoints)
    //
    if (EptHookGetCountOfEpthooks(FALSE) != 0)
    {
        CurrentMask |= 1 << EXCEPTION_VECTOR_BREAKPOINT;
    }

    return CurrentMask;
}

/**
 * @brief Set exception bitmap in VMCS 
 * @details Should be called in vmx-root
 * 
 * @param IdtIndex Interrupt Descriptor Table index of exception 
 * @return VOID 
 */
VOID
ProtectedHvSetExceptionBitmap(UINT32 IdtIndex)
{
    UINT32 ExceptionBitmap = 0;

    //
    // Read the current bitmap
    //
    ExceptionBitmap = HvReadExceptionBitmap();

    if (IdtIndex == DEBUGGER_EVENT_EXCEPTIONS_ALL_FIRST_32_ENTRIES)
    {
        ExceptionBitmap = 0xffffffff;
    }
    else
    {
        ExceptionBitmap |= 1 << IdtIndex;
    }

    //
    // Set the new value
    //
    HvWriteExceptionBitmap(ProtectedHvExceptionBitmapIntegrityCheck(ExceptionBitmap, PASSING_OVER_NONE));
}

/**
 * @brief Unset exception bitmap in VMCS 
 * @details Should be called in vmx-root
 * 
 * @param IdtIndex Interrupt Descriptor Table index of exception 
 * @return VOID 
 */
VOID
ProtectedHvUnsetExceptionBitmap(UINT32 IdtIndex)
{
    UINT32 ExceptionBitmap = 0;

    //
    // Read the current bitmap
    //
    ExceptionBitmap = HvReadExceptionBitmap();

    if (IdtIndex == DEBUGGER_EVENT_EXCEPTIONS_ALL_FIRST_32_ENTRIES)
    {
        ExceptionBitmap = 0x0;
    }
    else
    {
        ExceptionBitmap &= ~(1 << IdtIndex);
    }

    //
    // Set the new value
    //
    HvWriteExceptionBitmap(ProtectedHvExceptionBitmapIntegrityCheck(ExceptionBitmap, PASSING_OVER_NONE));
}

/**
 * @brief Reset exception bitmap in VMCS because of clearing 
 * !exception commands
 * @details Should be called in vmx-root
 * 
 * @return VOID 
 */
VOID
ProtectedHvResetExceptionBitmapToClearEvents()
{
    UINT32 ExceptionBitmap = 0;

    //
    // Set the new value
    //
    HvWriteExceptionBitmap(ProtectedHvExceptionBitmapIntegrityCheck(ExceptionBitmap, PASSING_OVER_EXCEPTION_EVENTS));
}

/**
 * @brief Reset exception bitmap in VMCS because of clearing 
 * !exception commands
 * @details Should be called in vmx-root
 * 
 * @return VOID 
 */
VOID
ProtectedHvRemoveUndefinedInstructionForDisablingSyscallSysretCommands()
{
    UINT32 ExceptionBitmap = 0;

    //
    // Read the current bitmap
    //
    ExceptionBitmap = HvReadExceptionBitmap();

    //
    // Unset exception bitmap for #UD
    //
    ExceptionBitmap &= ~(1 << EXCEPTION_VECTOR_UNDEFINED_OPCODE);

    //
    // Set the new value
    //
    HvWriteExceptionBitmap(ProtectedHvExceptionBitmapIntegrityCheck(ExceptionBitmap, PASSING_OVER_UD_EXCEPTIONS_FOR_SYSCALL_SYSRET_HOOK));
}
