/**
 * @file script-engine.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Interpret script engine affairs
 * @details
 * @version 0.1
 * @date 2021-09-23
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern UINT64                   g_ResultOfEvaluatedExpression;
extern UINT32                   g_ErrorStateOfResultOfEvaluatedExpression;
extern BOOLEAN                  g_IsSerialConnectedToRemoteDebuggee;
extern ACTIVE_DEBUGGING_PROCESS g_ActiveProcessDebuggingState;

/**
 * @brief Get the value from the evaluation of single expression
 * from local debuggee and remote debuggee
 *
 * @param Expr
 * @param HasError
 * @return UINT64
 */
UINT64
ScriptEngineEvalSingleExpression(string Expr, PBOOLEAN HasError)
{
    UINT64 Result = NULL;

    //
    // Prepend and append 'formats(' and ')'
    //
    Expr.insert(0, "formats(");
    Expr.append(");");

    if (g_IsSerialConnectedToRemoteDebuggee || g_ActiveProcessDebuggingState.IsActive)
    {
        //
        // Send data to the target user debugger or kernel debugger
        //
        if (!ScriptEngineExecuteSingleExpression((CHAR *)Expr.c_str(), TRUE, FALSE))
        {
            *HasError = TRUE;
            return NULL;
        }

        //
        // Check whether there was an error in evaluation or not
        //
        if (g_ErrorStateOfResultOfEvaluatedExpression == DEBUGGER_OPERATION_WAS_SUCCESSFUL)
        {
            //
            // Everything was fine, return the result of the evaluated
            // expression and null the global holders
            //
            Result                                    = g_ResultOfEvaluatedExpression;
            g_ErrorStateOfResultOfEvaluatedExpression = NULL;
            g_ResultOfEvaluatedExpression             = NULL;
            *HasError                                 = FALSE;
        }
        else
        {
            //
            // There was an error evaluating the expression from the kernel (debuggee)
            //
            g_ErrorStateOfResultOfEvaluatedExpression = NULL;
            g_ResultOfEvaluatedExpression             = NULL;

            *HasError = TRUE;
            Result    = NULL;
        }
    }
    else
    {
        //
        // It's in vmi-mode,
        // execute it locally with regs set to ZERO
        //
        Result = ScriptEngineEvalUInt64StyleExpressionWrapper(Expr, HasError);
    }

    return Result;
}

/**
 * @brief Execute single expression for the kernel debugger and the user-mode debugger
 *
 * @param Expr
 * @param ShowErrorMessageIfAny
 * @param IsFormat If it's a format expression
 *
 * @return BOOLEAN Returns TRUE if it was successful
 */
BOOLEAN
ScriptEngineExecuteSingleExpression(CHAR * Expr, BOOLEAN ShowErrorMessageIfAny, BOOLEAN IsFormat)
{
    PVOID   CodeBuffer;
    UINT64  BufferAddress;
    UINT32  BufferLength;
    UINT32  Pointer;
    BOOLEAN Result = FALSE;

    //
    // Check whether a kernel debugger is connected or a user-mode debugger is active
    //
    if (!g_IsSerialConnectedToRemoteDebuggee && !g_ActiveProcessDebuggingState.IsActive)
    {
        if (ShowErrorMessageIfAny)
        {
            ShowMessages("err, you're not connected to any debuggee (neither user debugger nor kernel debugger)\n");
        }
        return FALSE;
    }

    //
    // Check if the user-mode debuggee is paused
    //
    if (g_ActiveProcessDebuggingState.IsActive && !g_ActiveProcessDebuggingState.IsPaused)
    {
        if (ShowErrorMessageIfAny)
        {
            ShowMessages("err, the target process is NOT paused, you should run 'pause' to pause it\n");
        }
        return FALSE;
    }

    //
    // Run script engine handler
    //
    CodeBuffer = ScriptEngineParseWrapper(Expr, ShowErrorMessageIfAny);

    if (CodeBuffer == NULL)
    {
        //
        // return to show that this item contains an error
        //
        return FALSE;
    }

    //
    // Print symbols (test)
    //
    // PrintSymbolBufferWrapper(CodeBuffer);

    //
    // Set the buffer and length
    //
    BufferAddress = ScriptEngineWrapperGetHead(CodeBuffer);
    BufferLength  = ScriptEngineWrapperGetSize(CodeBuffer);
    Pointer       = ScriptEngineWrapperGetPointer(CodeBuffer);

    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // Send it to the remote debuggee (kernel debugger)
        //
        Result = KdSendScriptPacketToDebuggee(BufferAddress,
                                              BufferLength,
                                              Pointer,
                                              IsFormat);
    }
    else if (g_ActiveProcessDebuggingState.IsActive)
    {
        //
        // Send it to the user debugger
        //
        Result = UdSendScriptBufferToProcess(
            g_ActiveProcessDebuggingState.ProcessDebuggingToken,
            g_ActiveProcessDebuggingState.ThreadId,
            BufferAddress,
            BufferLength,
            Pointer,
            IsFormat);
    }
    else
    {
        //
        // Not connected to any debuggee
        //
        ShowMessages("err, you're not connected to any debuggee (neither user debugger nor kernel debugger)\n");
        Result = FALSE;
    }

    //
    // Remove the buffer of script engine interpreted code
    //
    ScriptEngineWrapperRemoveSymbolBuffer(CodeBuffer);

    //
    // Return result
    //
    return Result;
}
