/**
 * @file ScriptEngineInternalHeader.h
 * @author M.H. Gholamrezaei (mh@hyperdbg.org)
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Internal Headers of script engine
 * @details
 * @version 0.2
 * @date 2022-06-29
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//			        Registers                   //
//////////////////////////////////////////////////

VOID
SetRegValue(PGUEST_REGS GuestRegs, PSYMBOL Symbol, UINT64 Value);

UINT64
GetRegValue(PGUEST_REGS GuestRegs, REGS_ENUM RegId);

//////////////////////////////////////////////////
//			    Pseudo-registers               //
//////////////////////////////////////////////////

UINT64
ScriptEnginePseudoRegGetTid();

UINT64
ScriptEnginePseudoRegGetCore();

UINT64
ScriptEnginePseudoRegGetPid();

CHAR *
ScriptEnginePseudoRegGetPname();

UINT64
ScriptEnginePseudoRegGetProc();

UINT64
ScriptEnginePseudoRegGetThread();

UINT64
ScriptEnginePseudoRegGetPeb();

UINT64
ScriptEnginePseudoRegGetTeb();

UINT64
ScriptEnginePseudoRegGetIp();

UINT64
ScriptEnginePseudoRegGetBuffer(UINT64 * CorrespondingAction);

UINT64
ScriptEnginePseudoRegGetEventTag(PACTION_BUFFER ActionBuffer);

UINT64
ScriptEnginePseudoRegGetEventId(PACTION_BUFFER ActionBuffer);

//////////////////////////////////////////////////
//			         Keywords                   //
//////////////////////////////////////////////////

UINT64
ScriptEngineKeywordPoi(PUINT64 Address, BOOL * HasError);

WORD
ScriptEngineKeywordHi(PUINT64 Address, BOOL * HasError);

WORD
ScriptEngineKeywordLow(PUINT64 Address, BOOL * HasError);

BYTE
ScriptEngineKeywordDb(PUINT64 Address, BOOL * HasError);

WORD
ScriptEngineKeywordDd(PUINT64 Address, BOOL * HasError);

DWORD
ScriptEngineKeywordDw(PUINT64 Address, BOOL * HasError);

QWORD
ScriptEngineKeywordDq(PUINT64 Address, BOOL * HasError);

//////////////////////////////////////////////////
//			        Functions                   //
//////////////////////////////////////////////////

BOOLEAN
ScriptEngineFunctionEq(UINT64 Address, QWORD Value, BOOL * HasError);

BOOLEAN
ScriptEngineFunctionEd(UINT64 Address, DWORD Value, BOOL * HasError);

BOOLEAN
ScriptEngineFunctionEb(UINT64 Address, BYTE Value, BOOL * HasError);

BOOLEAN
ScriptEngineFunctionCheckAddress(UINT64 Address, UINT32 Length);

UINT64
ScriptEngineFunctionVirtualToPhysical(UINT64 Address);

UINT64
ScriptEngineFunctionPhysicalToVirtual(UINT64 Address);

VOID
ScriptEngineFunctionPrint(UINT64 Tag, BOOLEAN ImmediateMessagePassing, UINT64 Value);

VOID
ScriptEngineFunctionTestStatement(UINT64 Tag, BOOLEAN ImmediateMessagePassing, UINT64 Value);

VOID
ScriptEngineFunctionSpinlockLock(volatile LONG * Lock, BOOL * HasError);

VOID
ScriptEngineFunctionSpinlockUnlock(volatile LONG * Lock, BOOL * HasError);

VOID
ScriptEngineFunctionSpinlockLockCustomWait(volatile long * Lock, unsigned MaxWait, BOOL * HasError);

UINT64
ScriptEngineFunctionStrlen(const char * Address);

UINT64
ScriptEngineFunctionWcslen(const wchar_t * Address);

long long
ScriptEngineFunctionInterlockedExchange(long long volatile * Target,
                                        long long            Value,
                                        BOOL *               HasError);

long long
ScriptEngineFunctionInterlockedExchangeAdd(long long volatile * Addend,
                                           long long            Value,
                                           BOOL *               HasError);

long long
ScriptEngineFunctionInterlockedIncrement(long long volatile * Addend,
                                         BOOL *               HasError);

long long
ScriptEngineFunctionInterlockedDecrement(long long volatile * Addend,
                                         BOOL *               HasError);

long long
ScriptEngineFunctionInterlockedCompareExchange(
    long long volatile * Destination,
    long long            ExChange,
    long long            Comperand,
    BOOL *               HasError);

VOID
ScriptEngineFunctionEnableEvent(UINT64  Tag,
                                BOOLEAN ImmediateMessagePassing,
                                UINT64  Value);

VOID
ScriptEngineFunctionDisableEvent(UINT64  Tag,
                                 BOOLEAN ImmediateMessagePassing,
                                 UINT64  Value);

VOID
ScriptEngineFunctionPause(UINT64      Tag,
                          BOOLEAN     ImmediateMessagePassing,
                          PGUEST_REGS GuestRegs,
                          UINT64      Context);

VOID
ScriptEngineFunctionFlush();

VOID
ScriptEngineFunctionEventIgnore();

VOID
ScriptEngineFunctionFormats(UINT64 Tag, BOOLEAN ImmediateMessagePassing, UINT64 Value);

VOID
ScriptEngineFunctionPrintf(PGUEST_REGS                    GuestRegs,
                           ACTION_BUFFER *                ActionDetail,
                           SCRIPT_ENGINE_VARIABLES_LIST * VariablesList,
                           UINT64                         Tag,
                           BOOLEAN                        ImmediateMessagePassing,
                           char *                         Format,
                           UINT64                         ArgCount,
                           PSYMBOL                        FirstArg,
                           BOOLEAN *                      HasError);
