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

BOOLEAN
SetRegValueUsingSymbol(PGUEST_REGS GuestRegs, PSYMBOL Symbol, UINT64 Value);

//////////////////////////////////////////////////
//			    Pseudo-registers                //
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

UINT64
ScriptEnginePseudoRegGetEventStage(PACTION_BUFFER ActionBuffer);

UINT64
ScriptEnginePseudoRegGetTime();

UINT64
ScriptEnginePseudoRegGetDate();

//////////////////////////////////////////////////
//			         Keywords                   //
//////////////////////////////////////////////////

//
// For Virtual Memory
//

UINT64
ScriptEngineKeywordPoi(PUINT64 Address, BOOL * HasError);

WORD
ScriptEngineKeywordHi(PUINT64 Address, BOOL * HasError);

WORD
ScriptEngineKeywordLow(PUINT64 Address, BOOL * HasError);

BYTE
ScriptEngineKeywordDb(PUINT64 Address, BOOL * HasError);

DWORD
ScriptEngineKeywordDd(PUINT64 Address, BOOL * HasError);

WORD
ScriptEngineKeywordDw(PUINT64 Address, BOOL * HasError);

QWORD
ScriptEngineKeywordDq(PUINT64 Address, BOOL * HasError);

//
// For Physical Memory
//

UINT64
ScriptEngineKeywordPoiPa(PUINT64 Address, BOOL * HasError);

WORD
ScriptEngineKeywordHiPa(PUINT64 Address, BOOL * HasError);

WORD
ScriptEngineKeywordLowPa(PUINT64 Address, BOOL * HasError);

BYTE
ScriptEngineKeywordDbPa(PUINT64 Address, BOOL * HasError);

DWORD
ScriptEngineKeywordDdPa(PUINT64 Address, BOOL * HasError);

WORD
ScriptEngineKeywordDwPa(PUINT64 Address, BOOL * HasError);

QWORD
ScriptEngineKeywordDqPa(PUINT64 Address, BOOL * HasError);

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
ScriptEngineFunctionEqPa(UINT64 Address, QWORD Value, BOOL * HasError);

BOOLEAN
ScriptEngineFunctionEdPa(UINT64 Address, DWORD Value, BOOL * HasError);

BOOLEAN
ScriptEngineFunctionEbPa(UINT64 Address, BYTE Value, BOOL * HasError);

BOOLEAN
ScriptEngineFunctionCheckAddress(UINT64 Address, UINT32 Length);

VOID
ScriptEngineFunctionMemcpy(UINT64 Destination, UINT64 Source, UINT32 Num, BOOL * HasError);

VOID
ScriptEngineFunctionMemcpyPa(UINT64 Destination, UINT64 Source, UINT32 Num, BOOL * HasError);

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
ScriptEngineFunctionVirtualToPhysical(UINT64 Address);

UINT64
ScriptEngineFunctionPhysicalToVirtual(UINT64 Address);

UINT64
ScriptEngineFunctionStrlen(const char * Address);

UINT64
ScriptEngineFunctionDisassembleLen(PVOID Address, BOOLEAN Is32Bit);

UINT64
ScriptEngineFunctionWcslen(const wchar_t * Address);

VOID
ScriptEngineFunctionMicroSleep(UINT64 us);

UINT64
ScriptEngineFunctionRdtsc();

UINT64
ScriptEngineFunctionRdtscp();

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
ScriptEngineFunctionEventEnable(UINT64 EventId);

VOID
ScriptEngineFunctionEventDisable(UINT64 EventId);

VOID
ScriptEngineFunctionEventClear(UINT64 EventId);

VOID
ScriptEngineFunctionPause(
    ACTION_BUFFER * ActionDetail,
    PGUEST_REGS     GuestRegs);

VOID
ScriptEngineFunctionFlush();

VOID
ScriptEngineFunctionShortCircuitingEvent(UINT64 State, ACTION_BUFFER * ActionDetail);

VOID
ScriptEngineFunctionFormats(UINT64 Tag, BOOLEAN ImmediateMessagePassing, UINT64 Value);

VOID
ScriptEngineFunctionPrintf(PGUEST_REGS                       GuestRegs,
                           ACTION_BUFFER *                   ActionDetail,
                           SCRIPT_ENGINE_GENERAL_REGISTERS * ScriptGeneralRegisters,
                           UINT64                            Tag,
                           BOOLEAN                           ImmediateMessagePassing,
                           char *                            Format,
                           UINT64                            ArgCount,
                           PSYMBOL                           FirstArg,
                           BOOLEAN *                         HasError);

VOID
ScriptEngineFunctionEventInject(UINT32 InterruptionType, UINT32 Vector, BOOL * HasError);

VOID
ScriptEngineFunctionEventInjectErrorCode(UINT32 InterruptionType, UINT32 Vector, UINT32 ErrorCode, BOOL * HasError);

VOID
ScriptEngineFunctionEventTraceInstrumentationStep();

VOID
ScriptEngineFunctionEventTraceStepIn();

UINT64
ScriptEngineFunctionStrcmp(const char * Address1, const char * Address2);

UINT64
ScriptEngineFunctionStrncmp(const char * Address1, const char * Address2, size_t Num);

UINT64
ScriptEngineFunctionWcscmp(const wchar_t * Address1, const wchar_t * Address2);

UINT64
ScriptEngineFunctionWcsncmp(const wchar_t * Address1, const wchar_t * Address2, size_t Num);

UINT64
ScriptEngineFunctionMemcmp(const char * Address1, const char * Address2, size_t Count);
