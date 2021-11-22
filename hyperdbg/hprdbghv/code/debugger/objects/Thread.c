/**
 * @file Thread.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Implementation of kernel debugger functions for threads
 * @details
 * 
 * @version 0.1
 * @date 2021-11-23
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "..\hprdbghv\pch.h"

/**
 * @brief change the current thread
 * @param TidRequest
 * 
 * @return BOOLEAN 
 */
BOOLEAN
ThreadInterpretThread(PDEBUGGEE_DETAILS_AND_SWITCH_THREAD_PACKET TidRequest)
{
    switch (TidRequest->ActionType)
    {
    case DEBUGGEE_DETAILS_AND_SWITCH_THREAD_GET_THREAD_DETAILS:

        //
        // Debugger wants to know current tid, nt!_ETHREAD and process name, etc.
        //
        TidRequest->ProcessId = PsGetCurrentProcessId();
        TidRequest->ThreadId  = PsGetCurrentThreadId();
        TidRequest->Process   = PsGetCurrentProcess();
        TidRequest->Thread    = PsGetCurrentThread();
        MemoryMapperReadMemorySafe(GetProcessNameFromEprocess(PsGetCurrentProcess()), &TidRequest->ProcessName, 16);

        //
        // Operation was successful
        //
        TidRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;

        break;

    case DEBUGGEE_DETAILS_AND_SWITCH_THREAD_PERFORM_SWITCH:

        //
        // Perform the thread switch
        //
        // if (!ThreadSwitch(TidRequest->ThreadId, TidRequest->Thread))
        //{
        //    TidRequest->Result = DEBUGGER_ERROR_DETAILS_OR_SWITCH_THREAD_INVALID_PARAMETER;
        //    break;
        //}

        //
        // Operation was successful
        //
        TidRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;

        break;

    case DEBUGGEE_DETAILS_AND_SWITCH_THREAD_GET_THREAD_LIST:

        //
        // Show the thread list
        //
        // if (!ThreadShowList(&TidRequest->ThreadListSymDetails))
        // {
        //   TidRequest->Result = DEBUGGER_ERROR_DETAILS_OR_SWITCH_THREAD_INVALID_PARAMETER;
        //  break;
        // }

        //
        // Operation was successful
        //
        TidRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;

        break;

    default:

        //
        // Invalid type of action
        //
        TidRequest->Result = DEBUGGER_ERROR_DETAILS_OR_SWITCH_THREAD_INVALID_PARAMETER;

        break;
    }

    //
    // Check if the above operation contains error
    //
    if (TidRequest->Result == DEBUGGER_OPERATION_WAS_SUCCESSFULL)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
