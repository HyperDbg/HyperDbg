/**
 * @file Attaching.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Attaching and detaching for debugging user-mode processes
 * @details 
 *
 * @version 0.1
 * @date 2021-12-28
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "..\hprdbghv\pch.h"

/**
 * @brief Attach to the target process
 * @details this function should be called in vmx-root
 * 
 * @param AttachRequest 
 * @return BOOLEAN 
 */
VOID
AttachingTargetProcess(PDEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS AttachRequest)
{
    DbgBreakPoint();

    //
    // Fill the BaseAddress and Entrypoint and detect whether it's a 32-bit
    // process or a 64-bit process
    //
    if (UserAccessGetBaseOfModuleFromProcessId(AttachRequest->ProcessId,
                                               &AttachRequest->Is32Bit,
                                               &AttachRequest->BaseAddressOfMainModule,
                                               &AttachRequest->EntrypoinOfMainModule))
    {
        DbgBreakPoint();

        AttachRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
    }
    else
    {
        DbgBreakPoint();

        AttachRequest->Result = DEBUGGER_ERROR_UNABLE_TO_ATTACH_TO_TARGET_USER_MODE_PROCESS;
    }
}
