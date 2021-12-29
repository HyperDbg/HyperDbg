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
 * @brief Initialize the attaching mechanism
 * 
 * @return BOOLEAN
 */
BOOLEAN
AttachingInitialize()
{
    UNICODE_STRING FunctionName;

    //
    // Find address of PsGetProcessPeb
    //
    if (g_PsGetProcessPeb == NULL)
    {
        RtlInitUnicodeString(&FunctionName, L"PsGetProcessPeb");
        g_PsGetProcessPeb = (PsGetProcessPeb)MmGetSystemRoutineAddress(&FunctionName);

        if (g_PsGetProcessPeb == NULL)
        {
            LogError("Err, cannot resolve PsGetProcessPeb");

            //
            // Won't fail the entire debugger for not finding this
            //
            // return FALSE;
        }
    }

    //
    // Find address of PsGetProcessWow64Process
    //
    if (g_PsGetProcessWow64Process == NULL)
    {
        RtlInitUnicodeString(&FunctionName, L"PsGetProcessWow64Process");
        g_PsGetProcessWow64Process = (PsGetProcessWow64Process)MmGetSystemRoutineAddress(&FunctionName);

        if (g_PsGetProcessWow64Process == NULL)
        {
            LogError("Err, cannot resolve PsGetProcessPeb");

            //
            // Won't fail the entire debugger for not finding this
            //
            // return FALSE;
        }
    }

    //
    // Find address of ZwQueryInformationProcess
    //
    if (g_ZwQueryInformationProcess == NULL)
    {
        UNICODE_STRING RoutineName;

        RtlInitUnicodeString(&RoutineName, L"ZwQueryInformationProcess");

        g_ZwQueryInformationProcess =
            (ZwQueryInformationProcess)MmGetSystemRoutineAddress(&RoutineName);

        if (g_ZwQueryInformationProcess == NULL)
        {
            LogError("Err, cannot resolve ZwQueryInformationProcess");

            //
            // Won't fail the entire debugger for not finding this
            //
            // return FALSE;
        }
    }

    return TRUE;
}

/**
 * @brief Attach to the target process
 * @details this function should be called in vmx-root
 * 
 * @param AttachRequest 
 * @return VOID 
 */
VOID
AttachingTargetProcess(PDEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS AttachRequest)
{
    //
    // Fill the BaseAddress and Entrypoint and detect whether it's a 32-bit
    // process or a 64-bit process
    //
    if (UserAccessPrintLoadedModules(AttachRequest->ProcessId))
    {
        AttachRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
    }
    else
    {
        AttachRequest->Result = DEBUGGER_ERROR_UNABLE_TO_ATTACH_TO_TARGET_USER_MODE_PROCESS;
    }
}
