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
        if (ActionBuffer->CurrentAction != NULL)
        {
            return ScriptEnginePseudoRegGetBuffer(
                (UINT64 *)ActionBuffer->CurrentAction);
        }
        else
        {
            return NULL;
        }
    case PSEUDO_REGISTER_CONTEXT:
        return ActionBuffer->Context;
    case PSEUDO_REGISTER_EVENT_TAG:
        return ScriptEnginePseudoRegGetEventTag(ActionBuffer);
    case PSEUDO_REGISTER_EVENT_ID:
        return ScriptEnginePseudoRegGetEventId(ActionBuffer);
    case INVALID:
#ifdef SCRIPT_ENGINE_USER_MODE
        ShowMessages("error in reading regesiter");
#endif // SCRIPT_ENGINE_USER_MODE
        return INVALID;
        // TODO: Add all the register
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
         BOOLEAN                       ReturnReference)
{
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
            return NULL; // Not reasonable, you should not dereference a register!
        else
            return GetRegValue(GuestRegs, (REGS_ENUM)Symbol->Value);

    case SYMBOL_PSEUDO_REG_TYPE:

        if (ReturnReference)
            return NULL; // Not reasonable, you should not dereference a pseudo-register!
        else
            return GetPseudoRegValue(Symbol, ActionBuffer);

    case SYMBOL_TEMP_TYPE:

        if (ReturnReference)
            return ((UINT64)&VariablesList->TempList[Symbol->Value]);
        else
            return VariablesList->TempList[Symbol->Value];
    }
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
SetValue(PGUEST_REGS GuestRegs, SCRIPT_ENGINE_VARIABLES_LIST * VariablesList, PSYMBOL Symbol, UINT64 Value)
{
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
                    int *                          Indx,
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
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionEd(SrcVal1, SrcVal0, &HasError);

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_EB:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionEb(SrcVal1, SrcVal0, &HasError);

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_EQ:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionEq(SrcVal1, SrcVal0, &HasError);

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_INTERLOCKED_EXCHANGE:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionInterlockedExchange((volatile long long *)SrcVal1, SrcVal0, &HasError);

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_INTERLOCKED_EXCHANGE_ADD:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionInterlockedExchangeAdd((volatile long long *)SrcVal1, SrcVal0, &HasError);

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_INTERLOCKED_COMPARE_EXCHANGE:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Src2 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal2 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src2, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionInterlockedCompareExchange((volatile long long *)SrcVal2, SrcVal1, SrcVal0, &HasError);

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_SPINLOCK_LOCK_CUSTOM_WAIT:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        ScriptEngineFunctionSpinlockLockCustomWait((volatile long *)SrcVal1, SrcVal0, &HasError);

        return HasError;

    case FUNC_PAUSE:

        ScriptEngineFunctionPause(ActionDetail->Tag,
                                  ActionDetail->ImmediatelySendTheResults,
                                  GuestRegs,
                                  ActionDetail->Context);
        return HasError;

    case FUNC_FLUSH:

        ScriptEngineFunctionFlush();

        return HasError;

    case FUNC_EVENT_IGNORE:

        ScriptEngineFunctionEventIgnore();

        return HasError;

    case FUNC_OR:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = SrcVal1 | SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_INC:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        DesVal = SrcVal0 + 1;

        Des = Src0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_DEC:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        DesVal = SrcVal0 - 1;

        Des = Src0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_XOR:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 ^ SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_AND:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 & SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_ASR:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 >> SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_ASL:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = SrcVal1 << SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_ADD:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 + SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_SUB:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 - SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_MUL:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 * SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_DIV:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (SrcVal0 == 0)
        {
            HasError = TRUE;
            return HasError;
        }

        DesVal = SrcVal1 / SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_MOD:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        if (SrcVal0 == 0)
        {
            HasError = TRUE;
            return HasError;
        }

        DesVal = SrcVal1 % SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_GT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 > SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_LT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 < SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_EGT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 >= SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_ELT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = SrcVal1 <= SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_EQUAL:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 == SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_NEQ:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 != SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_POI:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordPoi((PUINT64)GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE),
                                        &HasError);
        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_DB:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordDb((PUINT64)GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE),
                                       &HasError);
        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_DD:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordDd((PUINT64)GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE),
                                       &HasError);

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_DW:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordDw((PUINT64)GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE),
                                       &HasError);
        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_DQ:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordDq((PUINT64)GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE),
                                       &HasError);
        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_NOT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ~SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_REFERENCE:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        //
        // It's reference, we need an address
        //
        SrcVal0 = GetValue(GuestRegs, ActionDetail, VariablesList, Src0, TRUE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_PHYSICAL_TO_VIRTUAL:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionPhysicalToVirtual(GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE));

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_VIRTUAL_TO_PHYSICAL:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionVirtualToPhysical(GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE));

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_CHECK_ADDRESS:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        if (ScriptEngineFunctionCheckAddress(SrcVal0, sizeof(BYTE)))
            DesVal = 1; // TRUE
        else
            DesVal = 0; // FALSE

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_STRLEN:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionStrlen((const char *)SrcVal0);

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_WCSLEN:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionWcslen((const wchar_t *)SrcVal0);

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_INTERLOCKED_INCREMENT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionInterlockedIncrement((volatile long long *)SrcVal0, &HasError);

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_INTERLOCKED_DECREMENT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionInterlockedDecrement((volatile long long *)SrcVal0, &HasError);

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_NEG:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = -(INT64)SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_HI:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordHi((PUINT64)GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE),
                                       &HasError);
        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_LOW:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordLow((PUINT64)GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE),
                                        &HasError);
        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_MOV:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_PRINT:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        //
        // Call the target function
        //
        ScriptEngineFunctionPrint(ActionDetail->Tag,
                                  ActionDetail->ImmediatelySendTheResults,
                                  SrcVal0);
        return HasError;

    case FUNC_TEST_STATEMENT:
        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        //
        // Call the target function
        //
        ScriptEngineFunctionTestStatement(ActionDetail->Tag,
                                          ActionDetail->ImmediatelySendTheResults,
                                          SrcVal0);
        return HasError;

    case FUNC_SPINLOCK_LOCK:
        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        //
        // Call the target function
        //
        ScriptEngineFunctionSpinlockLock((volatile LONG *)SrcVal0, &HasError);

        return HasError;

    case FUNC_SPINLOCK_UNLOCK:
        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        //
        // Call the target function
        //
        ScriptEngineFunctionSpinlockUnlock((volatile LONG *)SrcVal0, &HasError);

        return HasError;

    case FUNC_EVENT_DISABLE:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        ScriptEngineFunctionDisableEvent(
            ActionDetail->Tag,
            ActionDetail->ImmediatelySendTheResults,
            SrcVal0);

        return HasError;

    case FUNC_EVENT_ENABLE:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        ScriptEngineFunctionEnableEvent(
            ActionDetail->Tag,
            ActionDetail->ImmediatelySendTheResults,
            SrcVal0);

        return HasError;

    case FUNC_FORMATS:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        //
        // Call the target function
        //
        ScriptEngineFunctionFormats(
            ActionDetail->Tag,
            ActionDetail->ImmediatelySendTheResults,
            SrcVal0);

        return HasError;

    case FUNC_JZ:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        if (SrcVal1 == 0)
            *Indx = SrcVal0;

        return HasError;

    case FUNC_JNZ:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;
        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        if (SrcVal1 != 0)
            *Indx = SrcVal0;

        return HasError;

    case FUNC_JMP:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        *Indx = SrcVal0;

        return HasError;

    case FUNC_PRINTF:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        //
        // Call the target function
        //

        *Indx =
            *Indx + ((sizeof(unsigned long long) + strlen((char *)&Src0->Value)) /
                     sizeof(SYMBOL));

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        PSYMBOL Src2 = NULL;

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
            (BOOLEAN *)&HasError);

        return HasError;
    }
}
