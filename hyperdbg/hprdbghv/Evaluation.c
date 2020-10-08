/**
 * @file Evaluation.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Interpret and evaluate the expressions from the scripting engine
 * @details 
 * @version 0.1
 * @date 2020-10-08
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "pch.h"

/**
 * @brief Perform tasks relating to stepping (step-in & step-out) requests
 * 
 * @param DebuggerPrintRequest Request to print
 * @param BufferToSendToUsermode The buffer will be delivered eventually to 
 * @param SizeOfBufferToSendToUsermode Return the size of the buffer that  
 * should be returned to the user-mode
 * the user-mode
 * @return NTSTATUS 
 */
NTSTATUS
EvaluationInterpretPrintRequest(PDEBUGGER_PRINT DebuggerPrintRequest, PVOID BufferToSendToUsermode, PUINT SizeOfBufferToSendToUsermode)
{
    //
    // Warning, DebuggerPrintRequest and BufferToSendToUsermode are at the same
    // location, don't write to BufferToSendToUsermode while you still expect to
    // read from DebuggerPrintRequest
    //
    DbgBreakPoint();

    return STATUS_SUCCESS;
}
