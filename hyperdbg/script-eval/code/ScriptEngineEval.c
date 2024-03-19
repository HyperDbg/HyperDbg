/**
 * @file ScriptEngineEval.c
 * @author M.H. Gholamrezaei (mh@hyperdbg.org)
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Shared Headers for Script engine
 * @details
 * @version 0.1
 * @date 2020-10-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"
#include "..\script-eval\header\ScriptEngineInternalHeader.h"

/**
 * @brief Get the Pseudo reg value
 *
 * @param Symbol
 * @param ActionBuffer
 * @return UINT64
 */
UINT64
GetPseudoRegValue(PSYMBOL Symbol, PACTION_BUFFER ActionBuffer)
{
    switch (Symbol->Value)
    {
    case PSEUDO_REGISTER_TID:
        return ScriptEnginePseudoRegGetTid();
    case PSEUDO_REGISTER_PID:
        return ScriptEnginePseudoRegGetPid();
    case PSEUDO_REGISTER_PNAME:
        return (UINT64)ScriptEnginePseudoRegGetPname();
    case PSEUDO_REGISTER_CORE:
        return ScriptEnginePseudoRegGetCore();
    case PSEUDO_REGISTER_PROC:
        return ScriptEnginePseudoRegGetProc();
    case PSEUDO_REGISTER_THREAD:
        return ScriptEnginePseudoRegGetThread();
    case PSEUDO_REGISTER_PEB:
        return ScriptEnginePseudoRegGetPeb();
    case PSEUDO_REGISTER_TEB:
        return ScriptEnginePseudoRegGetTeb();
    case PSEUDO_REGISTER_IP:
        return ScriptEnginePseudoRegGetIp();
    case PSEUDO_REGISTER_BUFFER:
        if (ActionBuffer->CurrentAction != (UINT64)NULL)
        {
            return ScriptEnginePseudoRegGetBuffer(
                (UINT64 *)ActionBuffer->CurrentAction);
        }
        else
        {
            return (UINT64)NULL;
        }
    case PSEUDO_REGISTER_CONTEXT:
        return ActionBuffer->Context;
    case PSEUDO_REGISTER_EVENT_TAG:
        return ScriptEnginePseudoRegGetEventTag(ActionBuffer);
    case PSEUDO_REGISTER_EVENT_ID:
        return ScriptEnginePseudoRegGetEventId(ActionBuffer);
    case PSEUDO_REGISTER_EVENT_STAGE:
        return ScriptEnginePseudoRegGetEventStage(ActionBuffer);
    case INVALID:
#ifdef SCRIPT_ENGINE_USER_MODE
        ShowMessages("error in reading regesiter");
#endif // SCRIPT_ENGINE_USER_MODE
        return INVALID;
    default:
#ifdef SCRIPT_ENGINE_USER_MODE
        ShowMessages("unknown pseudo-register");
#endif // SCRIPT_ENGINE_USER_MODE
        return INVALID;
    }
}

/**
 * @brief Get the Value (reg, peseudo-reg, etc.)
 *
 * @param GuestRegs
 * @param ActionBuffer
 * @param VariablesList
 * @param Symbol
 * @param ReturnReference
 * @return UINT64
 */
UINT64
GetValue(PGUEST_REGS                   GuestRegs,
         PACTION_BUFFER                ActionBuffer,
         PSCRIPT_ENGINE_VARIABLES_LIST VariablesList,
         PSYMBOL                       Symbol,
         BOOLEAN                       ReturnReference,
         SYMBOL_BUFFER *               StackBuffer,
         int *                         StackIndx,
         int *                         StackBaseIndx,
         int *                         StackTempBaseIndx)
{
#ifdef SCRIPT_ENGINE_KERNEL_MODE
    UNREFERENCED_PARAMETER(StackIndx);
#endif // SCRIPT_ENGINE_KERNEL_MODE

    switch (Symbol->Type)
    {
    case SYMBOL_GLOBAL_ID_TYPE:

        if (ReturnReference)
            return ((UINT64)(&VariablesList->GlobalVariablesList[Symbol->Value]));
        else
            return VariablesList->GlobalVariablesList[Symbol->Value];

    case SYMBOL_LOCAL_ID_TYPE:

        if (ReturnReference)
            return ((UINT64)(&VariablesList->LocalVariablesList[Symbol->Value]));
        else
            return VariablesList->LocalVariablesList[Symbol->Value];

    case SYMBOL_NUM_TYPE:

        if (ReturnReference)
            return ((UINT64)&Symbol->Value);
        else
            return Symbol->Value;

    case SYMBOL_REGISTER_TYPE:

        if (ReturnReference)
            return (UINT64)NULL; // Not reasonable, you should not dereference a register!
        else
            return GetRegValue(GuestRegs, (REGS_ENUM)Symbol->Value);

    case SYMBOL_PSEUDO_REG_TYPE:

        if (ReturnReference)
            return (UINT64)NULL; // Not reasonable, you should not dereference a pseudo-register!
        else
            return GetPseudoRegValue(Symbol, ActionBuffer);

    case SYMBOL_TEMP_TYPE:
        if (ReturnReference)
            return ((UINT64)&VariablesList->TempList[Symbol->Value]);
        else
            return VariablesList->TempList[Symbol->Value];

    case SYMBOL_STACK_TEMP_TYPE:
    {
        PSYMBOL StackSymbol = NULL;

        StackSymbol = (PSYMBOL)((unsigned long long)StackBuffer->Head +
                                (unsigned long long)((*StackTempBaseIndx + Symbol->Value) * sizeof(SYMBOL)));
        return StackSymbol->Value;
    }
    case SYMBOL_FUNCTION_PARAMETER_ID_TYPE:
    {
        PSYMBOL StackSymbol = NULL;

        StackSymbol = (PSYMBOL)((unsigned long long)StackBuffer->Head +
                                (unsigned long long)((*StackBaseIndx + Symbol->Value) * sizeof(SYMBOL)));
        return StackSymbol->Value;
    }
    }

    //
    // Shouldn't reach here
    //
    return NULL64_ZERO;
}

/**
 * @brief Set the value
 *
 * @param GuestRegs
 * @param VariablesList
 * @param Symbol
 * @param Value
 * @return VOID
 */
VOID
SetValue(PGUEST_REGS                    GuestRegs,
         SCRIPT_ENGINE_VARIABLES_LIST * VariablesList,
         PSYMBOL                        Symbol,
         UINT64                         Value,
         SYMBOL_BUFFER *                StackBuffer,
         int *                          StackIndx,
         int *                          StackBaseIndx,
         int *                          StackTempBaseIndx)
{
#ifdef SCRIPT_ENGINE_KERNEL_MODE
    UNREFERENCED_PARAMETER(StackIndx);
#endif // SCRIPT_ENGINE_KERNEL_MODE

    switch (Symbol->Type)
    {
    case SYMBOL_GLOBAL_ID_TYPE:
        VariablesList->GlobalVariablesList[Symbol->Value] = Value;
        return;
    case SYMBOL_LOCAL_ID_TYPE:
        VariablesList->LocalVariablesList[Symbol->Value] = Value;
        return;
    case SYMBOL_TEMP_TYPE:
        VariablesList->TempList[Symbol->Value] = Value;
        return;
    case SYMBOL_REGISTER_TYPE:
        SetRegValue(GuestRegs, Symbol, Value);
        return;
    case SYMBOL_STACK_TEMP_TYPE:
    {
        PSYMBOL StackSymbol = NULL;

        StackSymbol        = (PSYMBOL)((unsigned long long)StackBuffer->Head +
                                (unsigned long long)((*StackTempBaseIndx + Symbol->Value) * sizeof(SYMBOL)));
        StackSymbol->Value = Value;
        return;
    }
    case SYMBOL_FUNCTION_PARAMETER_ID_TYPE:
    {
        PSYMBOL StackSymbol = (PSYMBOL)((unsigned long long)StackBuffer->Head +
                                        (unsigned long long)((*StackBaseIndx + Symbol->Value) * sizeof(SYMBOL)));
        StackSymbol->Value  = Value;
        return;
    }
    }
}

/**
 * @brief Get the operator name
 *
 * @param OperatorSymbol
 * @param BufferForName
 * @return VOID
 */
VOID
ScriptEngineGetOperatorName(PSYMBOL OperatorSymbol, CHAR * BufferForName)
{
    switch (OperatorSymbol->Value)
    {
    case FUNC_POI:
        memcpy(BufferForName, "poi", 3);
        break;
    case FUNC_DB:
        memcpy(BufferForName, "db", 2);
        break;
    case FUNC_DD:
        memcpy(BufferForName, "dd", 2);
        break;
    case FUNC_DW:
        memcpy(BufferForName, "dw", 2);
        break;
    case FUNC_DQ:
        memcpy(BufferForName, "dq", 2);
        break;
    case FUNC_HI:
        memcpy(BufferForName, "hi", 2);
        break;
    case FUNC_LOW:
        memcpy(BufferForName, "low", 3);
        break;
    default:
        memcpy(BufferForName, "error", 5);
        break;
    }
}

/**
 * @brief Execute the script buffer
 *
 * @param GuestRegs General purpose registers
 * @param ActionDetail Detail of the specific action
 * @param VariablesList List of core specific (and global) variable holders
 * @param CodeBuffer The script buffer to be executed
 * @param Indx Script Buffer index
 * @param ErrorOperator Error in operator
 * @return BOOL
 */
BOOL
ScriptEngineExecute(PGUEST_REGS                    GuestRegs,
                    ACTION_BUFFER *                ActionDetail,
                    SCRIPT_ENGINE_VARIABLES_LIST * VariablesList,
                    SYMBOL_BUFFER *                CodeBuffer,
                    UINT64 *                       Indx,
                    SYMBOL_BUFFER *                StackBuffer,
                    int *                          StackIndx,
                    int *                          StackBaseIndx,
                    int *                          StackTempBaseIndx,
                    SYMBOL *                       ErrorOperator)
{
    PSYMBOL Operator;
    PSYMBOL Src0;
    PSYMBOL Src1;
    PSYMBOL Src2;

    PSYMBOL Des;
    UINT64  SrcVal0;
    UINT64  SrcVal1;
    UINT64  SrcVal2;

    UINT64 DesVal;
    BOOL   HasError = FALSE;

    static int                StackIndxTemp = NULL_ZERO;
    static unsigned long long ReturnValue   = NULL64_ZERO;

    Operator = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

    *ErrorOperator = *Operator;

    *Indx = *Indx + 1;

    if (Operator->Type != SYMBOL_SEMANTIC_RULE_TYPE)
    {
#ifdef SCRIPT_ENGINE_USER_MODE
        ShowMessages("err, expecting operator type\n");
        return HasError;
#endif // SCRIPT_ENGINE_USER_MODE
    };

    switch (Operator->Value)
    {
    case FUNC_ED:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionEd(SrcVal1, (DWORD)SrcVal0, &HasError);

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_EB:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionEb(SrcVal1, (BYTE)SrcVal0, &HasError);

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_EQ:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionEq(SrcVal1, SrcVal0, &HasError);

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_INTERLOCKED_EXCHANGE:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionInterlockedExchange((volatile long long *)SrcVal1, SrcVal0, &HasError);

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_INTERLOCKED_EXCHANGE_ADD:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionInterlockedExchangeAdd((volatile long long *)SrcVal1, SrcVal0, &HasError);

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_INTERLOCKED_COMPARE_EXCHANGE:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src2 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal2 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src2, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionInterlockedCompareExchange((volatile long long *)SrcVal2, SrcVal1, SrcVal0, &HasError);

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_EVENT_INJECT_ERROR_CODE:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src2 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal2 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src2, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        ScriptEngineFunctionEventInjectErrorCode((UINT32)SrcVal2, (UINT32)SrcVal1, (UINT32)SrcVal0, &HasError);

        break;

    case FUNC_MEMCPY:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src2 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal2 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src2, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        ScriptEngineFunctionMemcpy(SrcVal2, SrcVal1, (UINT32)SrcVal0, &HasError);

        break;

    case FUNC_SPINLOCK_LOCK_CUSTOM_WAIT:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        ScriptEngineFunctionSpinlockLockCustomWait((volatile long *)SrcVal1, (UINT32)SrcVal0, &HasError);

        break;

    case FUNC_EVENT_INJECT:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        ScriptEngineFunctionEventInject((UINT32)SrcVal1, (UINT32)SrcVal0, &HasError);

        break;

    case FUNC_PAUSE:

        ScriptEngineFunctionPause(ActionDetail,
                                  GuestRegs);
        break;

    case FUNC_FLUSH:

        ScriptEngineFunctionFlush();

        break;

    case FUNC_EVENT_TRACE_INSTRUMENTATION_STEP:
    case FUNC_EVENT_TRACE_INSTRUMENTATION_STEP_IN:

        ScriptEngineFunctionEventTraceInstrumentationStep();

        break;

    case FUNC_EVENT_TRACE_STEP:
    case FUNC_EVENT_TRACE_STEP_IN:

        ScriptEngineFunctionEventTraceStepIn();

        break;

    case FUNC_EVENT_TRACE_STEP_OUT:

        //
        // To be implemented!
        //

        break;

    case FUNC_EVENT_SC:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        ScriptEngineFunctionShortCircuitingEvent(SrcVal0, ActionDetail);

        break;

    case FUNC_OR:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = SrcVal1 | SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_INC:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        DesVal = SrcVal0 + 1;

        Des = Src0;

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_DEC:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        DesVal = SrcVal0 - 1;

        Des = Src0;

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_XOR:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 ^ SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_AND:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 & SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_ASR:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 >> SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_ASL:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = SrcVal1 << SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_ADD:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 + SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_SUB:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 - SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_MUL:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 * SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_DIV:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (SrcVal0 == 0)
        {
            HasError = TRUE;
            break;
        }

        DesVal = SrcVal1 / SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_MOD:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        if (SrcVal0 == 0)
        {
            HasError = TRUE;
            break;
        }

        DesVal = SrcVal1 % SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_GT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = (INT64)SrcVal1 > (INT64)SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_LT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = (INT64)SrcVal1 < (INT64)SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_EGT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = (INT64)SrcVal1 >= (INT64)SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_ELT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = (INT64)SrcVal1 <= (INT64)SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_EQUAL:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 == SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_NEQ:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 != SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_POI:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordPoi((PUINT64)GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx),
                                        &HasError);
        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_DB:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordDb((PUINT64)GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx),
                                       &HasError);
        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_DD:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordDd((PUINT64)GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx),
                                       &HasError);

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_DW:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordDw((PUINT64)GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx),
                                       &HasError);
        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_DQ:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordDq((PUINT64)GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx),
                                       &HasError);
        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_NOT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ~SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_REFERENCE:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        //
        // It's reference, we need an address
        //
        SrcVal0 = GetValue(GuestRegs, ActionDetail, VariablesList, Src0, TRUE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_PHYSICAL_TO_VIRTUAL:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionPhysicalToVirtual(GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx));

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_VIRTUAL_TO_PHYSICAL:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionVirtualToPhysical(GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx));

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_CHECK_ADDRESS:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        if (ScriptEngineFunctionCheckAddress(SrcVal0, sizeof(BYTE)))
            DesVal = 1; // TRUE
        else
            DesVal = 0; // FALSE

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_STRLEN:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        if (Src0->Type == SYMBOL_STRING_TYPE)
        {
            *Indx =
                *Indx + ((3 * sizeof(unsigned long long) + Src0->Len) /
                         sizeof(SYMBOL));
            SrcVal0 = (UINT64)&Src0->Value;
        }
        else
        {
            SrcVal0 =
                GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);
        }

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionStrlen((const char *)SrcVal0);

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_DISASSEMBLE_LEN:
    case FUNC_DISASSEMBLE_LEN64:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionDisassembleLen((PVOID)SrcVal0, FALSE);

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_DISASSEMBLE_LEN32:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionDisassembleLen((PVOID)SrcVal0, TRUE);

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_WCSLEN:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        if (Src0->Type == SYMBOL_WSTRING_TYPE)
        {
            *Indx =
                *Indx + ((3 * sizeof(unsigned long long) + Src0->Len) /
                         sizeof(SYMBOL));
            SrcVal0 = (UINT64)&Src0->Value;
        }
        else
        {
            SrcVal0 =
                GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);
        }

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionWcslen((const wchar_t *)SrcVal0);

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_INTERLOCKED_INCREMENT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionInterlockedIncrement((volatile long long *)SrcVal0, &HasError);

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_INTERLOCKED_DECREMENT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionInterlockedDecrement((volatile long long *)SrcVal0, &HasError);

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_NEG:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = -(INT64)SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_HI:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordHi((PUINT64)GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx),
                                       &HasError);
        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_LOW:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordLow((PUINT64)GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx),
                                        &HasError);
        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_MOV:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_PRINT:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        //
        // Call the target function
        //
        ScriptEngineFunctionPrint(ActionDetail->Tag,
                                  ActionDetail->ImmediatelySendTheResults,
                                  SrcVal0);
        break;

    case FUNC_TEST_STATEMENT:
        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        //
        // Call the target function
        //
        ScriptEngineFunctionTestStatement(ActionDetail->Tag,
                                          ActionDetail->ImmediatelySendTheResults,
                                          SrcVal0);
        break;

    case FUNC_SPINLOCK_LOCK:
        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        //
        // Call the target function
        //
        ScriptEngineFunctionSpinlockLock((volatile LONG *)SrcVal0, &HasError);

        break;

    case FUNC_SPINLOCK_UNLOCK:
        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        //
        // Call the target function
        //
        ScriptEngineFunctionSpinlockUnlock((volatile LONG *)SrcVal0, &HasError);

        break;

    case FUNC_EVENT_ENABLE:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        ScriptEngineFunctionEventEnable(SrcVal0);

        break;

    case FUNC_EVENT_DISABLE:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        ScriptEngineFunctionEventDisable(SrcVal0);

        break;

    case FUNC_EVENT_CLEAR:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        ScriptEngineFunctionEventClear(SrcVal0);

        break;

    case FUNC_FORMATS:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        //
        // Call the target function
        //
        ScriptEngineFunctionFormats(
            ActionDetail->Tag,
            ActionDetail->ImmediatelySendTheResults,
            SrcVal0);

        break;

    case FUNC_JZ:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        if (SrcVal1 == 0)
            *Indx = SrcVal0;

        break;

    case FUNC_JNZ:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;
        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        if (SrcVal1 != 0)
            *Indx = SrcVal0;

        break;

    case FUNC_JMP:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        *Indx = SrcVal0;

        break;

    case FUNC_CALL_USER_DEFINED_FUNCTION:
        StackIndxTemp = *StackIndx;
        break;
    case FUNC_CALL_USER_DEFINED_FUNCTION_PARAMETER:

        //
        // push parameter variable into stack
        //
        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src1               = (PSYMBOL)((unsigned long long)StackBuffer->Head +
                         (unsigned long long)(*StackIndx * sizeof(SYMBOL)));
        *StackIndx         = *StackIndx + 1;
        Src1->Len          = 0;
        Src1->Type         = SYMBOL_FUNCTION_PARAMETER_TYPE;
        Src1->VariableType = 0;
        Src1->Value        = SrcVal0;

        break;

    case FUNC_END_OF_CALLING_USER_DEFINED_FUNCTION_WITHOUT_RETURNING_VALUE:
    case FUNC_END_OF_CALLING_USER_DEFINED_FUNCTION_WITH_RETURNING_VALUE:
    {
        //
        //
        //
        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        UINT64 FunctionAddress =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        //
        // push return address symbol into stack
        //
        Src0               = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx              = *Indx + 1;
        Src1               = (PSYMBOL)((unsigned long long)StackBuffer->Head +
                         (unsigned long long)(*StackIndx * sizeof(SYMBOL)));
        *StackIndx         = *StackIndx + 1;
        Src1->Len          = Src0->Len;
        Src1->Type         = Src0->Type;
        Src1->Value        = Src0->Value;
        Src1->VariableType = Src0->VariableType;

        *StackTempBaseIndx = *StackIndx;
        *StackBaseIndx     = 0;

        *StackBaseIndx = StackIndxTemp;

        // jump to function
        *Indx = FunctionAddress + 1;
    }

    break;

    case FUNC_START_OF_USER_DEFINED_FUNCTION:
        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0    = GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);
        *StackIndx = (UINT32)(*StackIndx + SrcVal0);

        break;

    case FUNC_MOV_RETURN_VALUE:
        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SetValue(GuestRegs, VariablesList, Des, ReturnValue, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);
        break;

    case FUNC_END_OF_USER_DEFINED_FUNCTION:
    case FUNC_RETURN_OF_USER_DEFINED_FUNCTION_WITHOUT_VALUE:
        // check
    case FUNC_RETURN_OF_USER_DEFINED_FUNCTION_WITH_VALUE:
    {
        if (Operator->Value == FUNC_RETURN_OF_USER_DEFINED_FUNCTION_WITH_VALUE)
        {
            Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                             (unsigned long long)(*Indx * sizeof(SYMBOL)));
            *Indx = *Indx + 1;
            ReturnValue =
                GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);
        }

        //
        // pop return address
        //
        UINT64 ReturnAddress = 0;

        Src0 = (PSYMBOL)((unsigned long long)StackBuffer->Head +
                         (unsigned long long)((*StackTempBaseIndx - 1) * sizeof(SYMBOL)));

        if (Src0->Type == SYMBOL_RETURN_ADDRESS_TYPE)
        {
            ReturnAddress = Src0->Value;
        }
        else
        {
            // error
        }

        *StackIndx = *StackBaseIndx;

        // update  StackBaseIndx and StackTempBaseIndx

        if (*StackIndx == 0)
        {
            *StackTempBaseIndx = 0;
            *StackBaseIndx     = 0;
        }
        else
        {
            int     i           = *StackIndx;
            PSYMBOL StackSymbol = NULL;

            *StackTempBaseIndx = 0;
            *StackBaseIndx     = 0;
            for (; i > 0; i--)
            {
                StackSymbol = (PSYMBOL)((unsigned long long)StackBuffer->Head +
                                        (unsigned long long)((i - 1) * sizeof(SYMBOL)));
                if (StackSymbol->Type == SYMBOL_RETURN_ADDRESS_TYPE)
                {
                    *StackTempBaseIndx = i;
                    *StackBaseIndx     = *StackTempBaseIndx - 1;
                    break;
                }
            }

            for (i = *StackTempBaseIndx - 2; i >= 0; i--)
            {
                StackSymbol = (PSYMBOL)((unsigned long long)StackBuffer->Head +
                                        (unsigned long long)((i) * sizeof(SYMBOL)));
                if (StackSymbol->Type == SYMBOL_FUNCTION_PARAMETER_TYPE)
                {
                    continue;
                }
                else
                {
                    *StackBaseIndx = i + 1;
                    break;
                }
            }

            if (i == -1)
            {
                *StackBaseIndx = 0;
            }
        }

        *Indx = ReturnAddress;
    }

    break;

    case FUNC_STRCMP:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (Src0->Type == SYMBOL_STRING_TYPE)
        {
            *Indx =
                *Indx + ((3 * sizeof(unsigned long long) + Src0->Len) /
                         sizeof(SYMBOL));
            SrcVal0 = (UINT64)&Src0->Value;
        }
        else
        {
            SrcVal0 =
                GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);
        }

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (Src1->Type == SYMBOL_STRING_TYPE)
        {
            *Indx =
                *Indx + ((3 * sizeof(unsigned long long) + Src1->Len) /
                         sizeof(SYMBOL));
            SrcVal1 = (UINT64)&Src1->Value;
        }
        else
        {
            SrcVal1 =
                GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);
        }

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionStrcmp((const char *)SrcVal1, (const char *)SrcVal0);

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_WCSCMP:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (Src0->Type == SYMBOL_WSTRING_TYPE)
        {
            *Indx =
                *Indx + ((3 * sizeof(unsigned long long) + Src0->Len) /
                         sizeof(SYMBOL));
            SrcVal0 = (UINT64)&Src0->Value;
        }
        else
        {
            SrcVal0 =
                GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);
        }

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (Src1->Type == SYMBOL_WSTRING_TYPE)
        {
            *Indx =
                *Indx + ((3 * sizeof(unsigned long long) + Src1->Len) /
                         sizeof(SYMBOL));
            SrcVal1 = (UINT64)&Src1->Value;
        }
        else
        {
            SrcVal1 =
                GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);
        }

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionWcscmp((const wchar_t *)SrcVal1, (const wchar_t *)SrcVal0);

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_MEMCMP:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (Src1->Type == SYMBOL_STRING_TYPE)
        {
            *Indx =
                *Indx + ((3 * sizeof(unsigned long long) + Src1->Len) /
                         sizeof(SYMBOL));
            SrcVal1 = (UINT64)&Src1->Value;
        }
        else
        {
            SrcVal1 =
                GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);
        }

        Src2 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (Src2->Type == SYMBOL_STRING_TYPE)
        {
            *Indx =
                *Indx + ((3 * sizeof(unsigned long long) + Src2->Len) /
                         sizeof(SYMBOL));
            SrcVal2 = (UINT64)&Src2->Value;
        }
        else
        {
            SrcVal2 =
                GetValue(GuestRegs, ActionDetail, VariablesList, Src2, FALSE, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);
        }

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionMemcmp((const char *)SrcVal2, (const char *)SrcVal1, SrcVal0);

        SetValue(GuestRegs, VariablesList, Des, DesVal, StackBuffer, StackIndx, StackBaseIndx, StackTempBaseIndx);

        break;

    case FUNC_PRINTF:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        //
        // Call the target function
        //

        *Indx =
            *Indx + ((3 * sizeof(unsigned long long) + Src0->Len) /
                     sizeof(SYMBOL));

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        Src2 = NULL;

        if (Src1->Value > 0)
        {
            Src2 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                             (unsigned long long)(*Indx * sizeof(SYMBOL)));

            *Indx = *Indx + Src1->Value;
        }

        ScriptEngineFunctionPrintf(
            GuestRegs,
            ActionDetail,
            VariablesList,
            ActionDetail->Tag,
            ActionDetail->ImmediatelySendTheResults,
            (char *)&Src0->Value,
            Src1->Value,
            Src2,
            (BOOLEAN *)&HasError,
            StackBuffer,
            StackIndx,
            StackBaseIndx,
            StackTempBaseIndx);

        break;
    }

    //
    // Return the result of whether error detected or not
    //
    return HasError;
}
