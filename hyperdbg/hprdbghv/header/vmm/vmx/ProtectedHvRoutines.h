/**
 * @file ProtectedHvRoutines.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Header files for protected hypervisor resources
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
#pragma once

//////////////////////////////////////////////////
//                     Enums                    //
//////////////////////////////////////////////////

/**
 * @brief Things to consider when applying resour
 * 
 */
typedef enum _PROTECTED_HV_RESOURCES_PASSING_OVERS
{
    PASSING_OVER_NONE                                  = 0,
    PASSING_OVER_UD_EXCEPTIONS_FOR_SYSCALL_SYSRET_HOOK = 1,
    PASSING_OVER_EXCEPTION_EVENTS,

} PROTECTED_HV_RESOURCES_PASSING_OVERS;
//////////////////////////////////////////////////
//                   Functions                  //
//////////////////////////////////////////////////

VOID
ProtectedHvSetExceptionBitmap(UINT32 IdtIndex);

VOID
ProtectedHvUnsetExceptionBitmap(UINT32 IdtIndex);

VOID
ProtectedHvResetExceptionBitmapToClearEvents();

VOID
ProtectedHvRemoveUndefinedInstructionForDisablingSyscallSysretCommands();
