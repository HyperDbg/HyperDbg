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
#include "../script-eval/header/ScriptEngineInternalHeader.h"

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
    case PSEUDO_REGISTER_TIME:
        return ScriptEnginePseudoRegGetTime();
    case PSEUDO_REGISTER_DATE:
        return ScriptEnginePseudoRegGetDate();
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
 * @param ScriptGeneralRegisters
 * @param Symbol
 * @param ReturnReference
 * @return UINT64
 */
UINT64
GetValue(PGUEST_REGS                      GuestRegs,
         PACTION_BUFFER                   ActionBuffer,
         PSCRIPT_ENGINE_GENERAL_REGISTERS ScriptGeneralRegisters,
         PSYMBOL                          Symbol,
         BOOLEAN                          ReturnReference)
{
    switch (Symbol->Type)
    {
    case SYMBOL_GLOBAL_ID_TYPE:

        if (ReturnReference)
            return ((UINT64)(&ScriptGeneralRegisters->GlobalVariablesList[Symbol->Value]));
        else
            return ScriptGeneralRegisters->GlobalVariablesList[Symbol->Value];

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

    case SYMBOL_STACK_INDEX_TYPE:
        if (ReturnReference)
            return (UINT64)&ScriptGeneralRegisters->StackIndx;
        else
            return ScriptGeneralRegisters->StackIndx;

    case SYMBOL_STACK_BASE_INDEX_TYPE:
        if (ReturnReference)
            return (UINT64)&ScriptGeneralRegisters->StackBaseIndx;
        else
            return ScriptGeneralRegisters->StackBaseIndx;

    case SYMBOL_RETURN_VALUE_TYPE:
        if (ReturnReference)
            return (UINT64)&ScriptGeneralRegisters->ReturnValue;
        else
            return ScriptGeneralRegisters->ReturnValue;

    case SYMBOL_TEMP_TYPE:

        if (ReturnReference)
            return (UINT64)&ScriptGeneralRegisters->StackBuffer[ScriptGeneralRegisters->StackBaseIndx + Symbol->Value];
        else
            return ScriptGeneralRegisters->StackBuffer[ScriptGeneralRegisters->StackBaseIndx + Symbol->Value];

    case SYMBOL_FUNCTION_PARAMETER_ID_TYPE:

        if (ReturnReference)
            return (UINT64)&ScriptGeneralRegisters->StackBuffer[ScriptGeneralRegisters->StackBaseIndx - 3 - Symbol->Value];
        else
            return ScriptGeneralRegisters->StackBuffer[ScriptGeneralRegisters->StackBaseIndx - 3 - Symbol->Value];
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
 * @param ScriptGeneralRegisters
 * @param Symbol
 * @param Value
 * @return VOID
 */
VOID
SetValue(PGUEST_REGS                       GuestRegs,
         SCRIPT_ENGINE_GENERAL_REGISTERS * ScriptGeneralRegisters,
         PSYMBOL                           Symbol,
         UINT64                            Value)
{
    switch (Symbol->Type)
    {
    case SYMBOL_GLOBAL_ID_TYPE:
        ScriptGeneralRegisters->GlobalVariablesList[Symbol->Value] = Value;
        return;
    case SYMBOL_REGISTER_TYPE:
        SetRegValueUsingSymbol(GuestRegs, Symbol, Value);
        return;

    case SYMBOL_STACK_INDEX_TYPE:
        ScriptGeneralRegisters->StackIndx = Value;
        return;

    case SYMBOL_STACK_BASE_INDEX_TYPE:
        ScriptGeneralRegisters->StackBaseIndx = Value;
        return;

    case SYMBOL_RETURN_VALUE_TYPE:
        ScriptGeneralRegisters->ReturnValue = Value;
        return;

    case SYMBOL_TEMP_TYPE:
        ScriptGeneralRegisters->StackBuffer[ScriptGeneralRegisters->StackBaseIndx + Symbol->Value] = Value;
        return;

    case SYMBOL_FUNCTION_PARAMETER_ID_TYPE:
        ScriptGeneralRegisters->StackBuffer[ScriptGeneralRegisters->StackBaseIndx - 3 - Symbol->Value] = Value;
        return;
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
    case FUNC_POI_PA:
        memcpy(BufferForName, "poi_pa", 6);
        break;
    case FUNC_DB_PA:
        memcpy(BufferForName, "db_pa", 5);
        break;
    case FUNC_DD_PA:
        memcpy(BufferForName, "dd_pa", 5);
        break;
    case FUNC_DW_PA:
        memcpy(BufferForName, "dw_pa", 5);
        break;
    case FUNC_DQ_PA:
        memcpy(BufferForName, "dq_pa", 5);
        break;
    case FUNC_HI_PA:
        memcpy(BufferForName, "hi_pa", 5);
        break;
    case FUNC_LOW_PA:
        memcpy(BufferForName, "low_pa", 6);
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
 * @param ScriptGeneralRegisters of core specific (and global) variable holders
 * @param CodeBuffer The script buffer to be executed
 * @param Indx Script Buffer index
 * @param ErrorOperator Error in operator
 * @return BOOL
 */
BOOL
ScriptEngineExecute(PGUEST_REGS                      GuestRegs,
                    ACTION_BUFFER *                  ActionDetail,
                    PSCRIPT_ENGINE_GENERAL_REGISTERS ScriptGeneralRegisters,
                    SYMBOL_BUFFER *                  CodeBuffer,
                    UINT64 *                         Indx,
                    SYMBOL *                         ErrorOperator)
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
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionEd(SrcVal1, (DWORD)SrcVal0, &HasError);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_EB:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionEb(SrcVal1, (BYTE)SrcVal0, &HasError);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_EQ:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionEq(SrcVal1, SrcVal0, &HasError);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_ED_PA:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionEdPa(SrcVal1, (DWORD)SrcVal0, &HasError);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_EB_PA:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionEbPa(SrcVal1, (BYTE)SrcVal0, &HasError);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_EQ_PA:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionEqPa(SrcVal1, SrcVal0, &HasError);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_INTERLOCKED_EXCHANGE:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionInterlockedExchange((volatile long long *)SrcVal1, SrcVal0, &HasError);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_INTERLOCKED_EXCHANGE_ADD:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionInterlockedExchangeAdd((volatile long long *)SrcVal1, SrcVal0, &HasError);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_INTERLOCKED_COMPARE_EXCHANGE:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Src2 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal2 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src2, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionInterlockedCompareExchange((volatile long long *)SrcVal2, SrcVal1, SrcVal0, &HasError);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_EVENT_INJECT_ERROR_CODE:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Src2 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal2 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src2, FALSE);

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
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Src2 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal2 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src2, FALSE);

        ScriptEngineFunctionMemcpy(SrcVal2, SrcVal1, (UINT32)SrcVal0, &HasError);

        break;

    case FUNC_MEMCPY_PA:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Src2 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal2 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src2, FALSE);

        ScriptEngineFunctionMemcpyPa(SrcVal2, SrcVal1, (UINT32)SrcVal0, &HasError);

        break;

    case FUNC_SPINLOCK_LOCK_CUSTOM_WAIT:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        ScriptEngineFunctionSpinlockLockCustomWait((volatile long *)SrcVal1, (UINT32)SrcVal0, &HasError);

        break;

    case FUNC_EVENT_INJECT:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

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
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

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
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = SrcVal1 | SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_INC:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        DesVal = SrcVal0 + 1;

        Des = Src0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_DEC:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        DesVal = SrcVal0 - 1;

        Des = Src0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_XOR:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 ^ SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_AND:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 & SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_ASR:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 >> SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_ASL:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = SrcVal1 << SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_ADD:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 + SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_SUB:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 - SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_MUL:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 * SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_DIV:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (SrcVal0 == 0)
        {
            HasError = TRUE;
            break;
        }

        DesVal = SrcVal1 / SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_MOD:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        if (SrcVal0 == 0)
        {
            HasError = TRUE;
            break;
        }

        DesVal = SrcVal1 % SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_GT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = (INT64)SrcVal1 > (INT64)SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_LT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = (INT64)SrcVal1 < (INT64)SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_EGT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = (INT64)SrcVal1 >= (INT64)SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_ELT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = (INT64)SrcVal1 <= (INT64)SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_EQUAL:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 == SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_NEQ:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 != SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_POI:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordPoi((PUINT64)GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE),
                                        &HasError);
        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_DB:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordDb((PUINT64)GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE),
                                       &HasError);
        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_DD:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordDd((PUINT64)GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE),
                                       &HasError);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_DW:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordDw((PUINT64)GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE),
                                       &HasError);
        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_DQ:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordDq((PUINT64)GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE),
                                       &HasError);
        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_POI_PA:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordPoiPa((PUINT64)GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE),
                                          &HasError);
        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_DB_PA:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordDbPa((PUINT64)GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE),
                                         &HasError);
        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_DD_PA:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordDdPa((PUINT64)GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE),
                                         &HasError);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_DW_PA:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordDwPa((PUINT64)GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE),
                                         &HasError);
        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_DQ_PA:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordDqPa((PUINT64)GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE),
                                         &HasError);
        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_NOT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ~SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_REFERENCE:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        //
        // It's reference, we need an address
        //
        SrcVal0 = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, TRUE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_PHYSICAL_TO_VIRTUAL:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionPhysicalToVirtual(GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE));

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_VIRTUAL_TO_PHYSICAL:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionVirtualToPhysical(GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE));

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_CHECK_ADDRESS:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        if (ScriptEngineFunctionCheckAddress(SrcVal0, sizeof(BYTE)))
            DesVal = 1; // TRUE
        else
            DesVal = 0; // FALSE

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_STRLEN:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        if (Src0->Type == SYMBOL_STRING_TYPE)
        {
            *Indx =
                *Indx + ((SIZE_SYMBOL_WITHOUT_LEN + Src0->Len) /
                         sizeof(SYMBOL));
            SrcVal0 = (UINT64)&Src0->Value;
        }
        else
        {
            SrcVal0 =
                GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);
        }

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionStrlen((const char *)SrcVal0);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_DISASSEMBLE_LEN:
    case FUNC_DISASSEMBLE_LEN64:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionDisassembleLen((PVOID)SrcVal0, FALSE);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_DISASSEMBLE_LEN32:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionDisassembleLen((PVOID)SrcVal0, TRUE);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_WCSLEN:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        if (Src0->Type == SYMBOL_WSTRING_TYPE)
        {
            *Indx =
                *Indx + ((SIZE_SYMBOL_WITHOUT_LEN + Src0->Len) /
                         sizeof(SYMBOL));
            SrcVal0 = (UINT64)&Src0->Value;
        }
        else
        {
            SrcVal0 =
                GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);
        }

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionWcslen((const wchar_t *)SrcVal0);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_MICROSLEEP:
        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        ScriptEngineFunctionMicroSleep(SrcVal0);
        break;

    case FUNC_RDTSC:

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionRdtsc();

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_RDTSCP:

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionRdtscp();

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);
        break;

    case FUNC_INTERLOCKED_INCREMENT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionInterlockedIncrement((volatile long long *)SrcVal0, &HasError);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_INTERLOCKED_DECREMENT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionInterlockedDecrement((volatile long long *)SrcVal0, &HasError);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_NEG:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = -(INT64)SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_HI:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordHi((PUINT64)GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE),
                                       &HasError);
        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_LOW:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordLow((PUINT64)GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE),
                                        &HasError);
        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_MOV:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_PRINT:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

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
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

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
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

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
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

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
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        ScriptEngineFunctionEventEnable(SrcVal0);

        break;

    case FUNC_EVENT_DISABLE:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        ScriptEngineFunctionEventDisable(SrcVal0);

        break;

    case FUNC_EVENT_CLEAR:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        ScriptEngineFunctionEventClear(SrcVal0);

        break;

    case FUNC_FORMATS:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

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
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        if (SrcVal1 == 0)
            *Indx = SrcVal0;

        break;

    case FUNC_JNZ:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;
        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        if (SrcVal1 != 0)
            *Indx = SrcVal0;

        break;

    case FUNC_JMP:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        *Indx = SrcVal0;

        break;

    case FUNC_PUSH:
        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        ScriptGeneralRegisters->StackBuffer[ScriptGeneralRegisters->StackIndx] = SrcVal0;
        ScriptGeneralRegisters->StackIndx++;

        break;

    case FUNC_POP:
        ScriptGeneralRegisters->StackIndx--;

        SrcVal0 = ScriptGeneralRegisters->StackBuffer[ScriptGeneralRegisters->StackIndx];

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SetValue(GuestRegs, ScriptGeneralRegisters, Des, SrcVal0);

        break;

    case FUNC_CALL:
        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        *Indx = *Indx + 1;

        ScriptGeneralRegisters->StackBuffer[ScriptGeneralRegisters->StackIndx] = *Indx;

        ScriptGeneralRegisters->StackIndx++;

        *Indx = SrcVal0;
        break;

    case FUNC_RET:

        ScriptGeneralRegisters->StackIndx--;

        *Indx = ScriptGeneralRegisters->StackBuffer[ScriptGeneralRegisters->StackIndx];
        break;
    case FUNC_STRCMP:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (Src0->Type == SYMBOL_STRING_TYPE)
        {
            *Indx =
                *Indx + ((SIZE_SYMBOL_WITHOUT_LEN + Src0->Len) /
                         sizeof(SYMBOL));
            SrcVal0 = (UINT64)&Src0->Value;
        }
        else
        {
            SrcVal0 =
                GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);
        }

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (Src1->Type == SYMBOL_STRING_TYPE)
        {
            *Indx =
                *Indx + ((SIZE_SYMBOL_WITHOUT_LEN + Src1->Len) /
                         sizeof(SYMBOL));
            SrcVal1 = (UINT64)&Src1->Value;
        }
        else
        {
            SrcVal1 =
                GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);
        }

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionStrcmp((const char *)SrcVal1, (const char *)SrcVal0);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_WCSCMP:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (Src0->Type == SYMBOL_WSTRING_TYPE)
        {
            *Indx =
                *Indx + ((SIZE_SYMBOL_WITHOUT_LEN + Src0->Len) /
                         sizeof(SYMBOL));
            SrcVal0 = (UINT64)&Src0->Value;
        }
        else
        {
            SrcVal0 =
                GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);
        }

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (Src1->Type == SYMBOL_WSTRING_TYPE)
        {
            *Indx =
                *Indx + ((SIZE_SYMBOL_WITHOUT_LEN + Src1->Len) /
                         sizeof(SYMBOL));
            SrcVal1 = (UINT64)&Src1->Value;
        }
        else
        {
            SrcVal1 =
                GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);
        }

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionWcscmp((const wchar_t *)SrcVal1, (const wchar_t *)SrcVal0);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_MEMCMP:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (Src1->Type == SYMBOL_STRING_TYPE)
        {
            *Indx =
                *Indx + ((SIZE_SYMBOL_WITHOUT_LEN + Src1->Len) /
                         sizeof(SYMBOL));
            SrcVal1 = (UINT64)&Src1->Value;
        }
        else
        {
            SrcVal1 =
                GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);
        }

        Src2 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (Src2->Type == SYMBOL_STRING_TYPE)
        {
            *Indx =
                *Indx + ((SIZE_SYMBOL_WITHOUT_LEN + Src2->Len) /
                         sizeof(SYMBOL));
            SrcVal2 = (UINT64)&Src2->Value;
        }
        else
        {
            SrcVal2 =
                GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src2, FALSE);
        }

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionMemcmp((const char *)SrcVal2, (const char *)SrcVal1, SrcVal0);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_STRNCMP:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (Src1->Type == SYMBOL_STRING_TYPE)
        {
            *Indx =
                *Indx + ((SIZE_SYMBOL_WITHOUT_LEN + Src1->Len) /
                         sizeof(SYMBOL));
            SrcVal1 = (UINT64)&Src1->Value;
        }
        else
        {
            SrcVal1 =
                GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);
        }

        Src2 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (Src2->Type == SYMBOL_STRING_TYPE)
        {
            *Indx =
                *Indx + ((SIZE_SYMBOL_WITHOUT_LEN + Src2->Len) /
                         sizeof(SYMBOL));
            SrcVal2 = (UINT64)&Src2->Value;
        }
        else
        {
            SrcVal2 =
                GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src2, FALSE);
        }

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionStrncmp((const char *)SrcVal2, (const char *)SrcVal1, SrcVal0);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_WCSNCMP:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (Src1->Type == SYMBOL_WSTRING_TYPE)
        {
            *Indx =
                *Indx + ((SIZE_SYMBOL_WITHOUT_LEN + Src1->Len) /
                         sizeof(SYMBOL));
            SrcVal1 = (UINT64)&Src1->Value;
        }
        else
        {
            SrcVal1 =
                GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);
        }

        Src2 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (Src2->Type == SYMBOL_WSTRING_TYPE)
        {
            *Indx =
                *Indx + ((SIZE_SYMBOL_WITHOUT_LEN + Src2->Len) /
                         sizeof(SYMBOL));
            SrcVal2 = (UINT64)&Src2->Value;
        }
        else
        {
            SrcVal2 =
                GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src2, FALSE);
        }

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionWcsncmp((const wchar_t *)SrcVal2, (const wchar_t *)SrcVal1, SrcVal0);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_PRINTF:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        //
        // Call the target function
        //

        *Indx =
            *Indx + ((SIZE_SYMBOL_WITHOUT_LEN + Src0->Len) /
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
            ScriptGeneralRegisters,
            ActionDetail->Tag,
            ActionDetail->ImmediatelySendTheResults,
            (char *)&Src0->Value,
            Src1->Value,
            Src2,
            (BOOLEAN *)&HasError);

        break;
    }

    //
    // Return the result of whether error detected or not
    //
    return HasError;
}
