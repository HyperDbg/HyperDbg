/**
 * @file kd.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief routines for remote kernel debugging
 * @details
 * @version 0.1
 * @date 2020-12-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//		            Definitions                 //
//////////////////////////////////////////////////

#define DbgWaitForKernelResponse(KernelSyncObjectId)                       \
    do                                                                     \
    {                                                                      \
        DEBUGGER_SYNCRONIZATION_EVENTS_STATE * SyncronizationObject =      \
            &g_KernelSyncronizationObjectsHandleTable[KernelSyncObjectId]; \
                                                                           \
        SyncronizationObject->IsOnWaitingState = TRUE;                     \
        WaitForSingleObject(SyncronizationObject->EventHandle, INFINITE);  \
    } while (FALSE);

#define DbgReceivedKernelResponse(KernelSyncObjectId)                      \
    do                                                                     \
    {                                                                      \
        DEBUGGER_SYNCRONIZATION_EVENTS_STATE * SyncronizationObject =      \
            &g_KernelSyncronizationObjectsHandleTable[KernelSyncObjectId]; \
                                                                           \
        SyncronizationObject->IsOnWaitingState = FALSE;                    \
        SetEvent(SyncronizationObject->EventHandle);                       \
    } while (FALSE);

//////////////////////////////////////////////////
//		    Display Windows Details             //
//////////////////////////////////////////////////

struct HKeyHolder
{
private:
    HKEY m_Key;

public:
    HKeyHolder() :
        m_Key(nullptr) { }

    HKeyHolder(const HKeyHolder &) = delete;
    HKeyHolder & operator=(const HKeyHolder &) = delete;

    ~HKeyHolder()
    {
        if (m_Key != nullptr)
            RegCloseKey(m_Key);
    }

    operator HKEY() const { return m_Key; }

    HKEY * operator&() { return &m_Key; }
};

//////////////////////////////////////////////////
//			    	 Functions                  //
//////////////////////////////////////////////////

BOOLEAN
KdCommandPacketToDebuggee(
    DEBUGGER_REMOTE_PACKET_TYPE             PacketType,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION RequestedAction);

BOOLEAN
KdCommandPacketAndBufferToDebuggee(
    DEBUGGER_REMOTE_PACKET_TYPE             PacketType,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION RequestedAction,
    CHAR *                                  Buffer,
    UINT32                                  BufferLength);

BOOLEAN
KdPrepareSerialConnectionToRemoteSystem(HANDLE  SerialHandle,
                                        BOOLEAN IsNamedPipe);

BOOLEAN
KdPrepareAndConnectDebugPort(const char * PortName, DWORD Baudrate, UINT32 Port, BOOLEAN IsPreparing, BOOLEAN IsNamedPipe);

BOOLEAN
KdSendPacketToDebuggee(const CHAR * Buffer, UINT32 Length, BOOLEAN SendEndOfBuffer);

BOOLEAN
KdReceivePacketFromDebuggee(CHAR * BufferToSave, UINT32 * LengthReceived);

BOOLEAN
KdCheckForTheEndOfTheBuffer(PUINT32 CurrentLoopIndex, BYTE * Buffer);

BOOLEAN
KdSendSwitchCorePacketToDebuggee(UINT32 NewCore);

BOOLEAN
KdSendEventQueryAndModifyPacketToDebuggee(
    UINT64                      Tag,
    DEBUGGER_MODIFY_EVENTS_TYPE TypeOfAction,
    BOOLEAN *                   IsEnabled);

BOOLEAN
KdSendFlushPacketToDebuggee();

BOOLEAN
KdSendCallStackPacketToDebuggee(UINT64                            BaseAddress,
                                UINT32                            Size,
                                DEBUGGER_CALLSTACK_DISPLAY_METHOD DisplayMethod,
                                BOOLEAN                           Is32Bit);

BOOLEAN
KdSendTestQueryPacketToDebuggee(UINT32 RequestIndex);

BOOLEAN
KdSendSymbolReloadPacketToDebuggee(UINT32 ProcessId);

BOOLEAN KdSendReadRegisterPacketToDebuggee(PDEBUGGEE_REGISTER_READ_DESCRIPTION);

BOOLEAN
KdSendReadMemoryPacketToDebuggee(PDEBUGGER_READ_MEMORY ReadMem);

BOOLEAN
KdSendEditMemoryPacketToDebuggee(PDEBUGGER_EDIT_MEMORY EditMem, UINT32 Size);

PDEBUGGER_EVENT_AND_ACTION_REG_BUFFER
KdSendRegisterEventPacketToDebuggee(PDEBUGGER_GENERAL_EVENT_DETAIL Event,
                                    UINT32                         EventBufferLength);

PDEBUGGER_EVENT_AND_ACTION_REG_BUFFER
KdSendAddActionToEventPacketToDebuggee(PDEBUGGER_GENERAL_ACTION GeneralAction,
                                       UINT32                   GeneralActionLength);

BOOLEAN
KdSendSwitchProcessPacketToDebuggee(DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_TYPE ActionType,
                                    UINT32                                   NewPid,
                                    UINT64                                   NewProcess,
                                    BOOLEAN                                  SetChangeByClockInterrupt,
                                    PDEBUGGEE_PROCESS_LIST_NEEDED_DETAILS    SymDetailsForProcessList);

BOOLEAN
KdSendSwitchThreadPacketToDebuggee(DEBUGGEE_DETAILS_AND_SWITCH_THREAD_TYPE ActionType,
                                   UINT32                                  NewTid,
                                   UINT64                                  NewThread,
                                   BOOLEAN                                 CheckByClockInterrupt,
                                   PDEBUGGEE_THREAD_LIST_NEEDED_DETAILS    SymDetailsForThreadList);

BOOLEAN
KdSendBpPacketToDebuggee(PDEBUGGEE_BP_PACKET BpPacket);

BOOLEAN
KdSendVa2paAndPa2vaPacketToDebuggee(PDEBUGGER_VA2PA_AND_PA2VA_COMMANDS Va2paAndPa2vaPacket);

BOOLEAN
KdSendPtePacketToDebuggee(PDEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS PtePacket);

BOOLEAN
KdSendListOrModifyPacketToDebuggee(
    PDEBUGGEE_BP_LIST_OR_MODIFY_PACKET ListOrModifyPacket);

BOOLEAN
KdSendScriptPacketToDebuggee(UINT64 BufferAddress, UINT32 BufferLength, UINT32 Pointer, BOOLEAN IsFormat);

BOOLEAN
KdSendUserInputPacketToDebuggee(const char * Sendbuf, int Len, BOOLEAN IgnoreBreakingAgain);

BOOLEAN
KdSendSearchRequestPacketToDebuggee(UINT64 * SearchRequestBuffer, UINT32 SearchRequestBufferSize);

BOOLEAN
KdSendStepPacketToDebuggee(DEBUGGER_REMOTE_STEPPING_REQUEST StepRequestType);

BYTE
KdComputeDataChecksum(PVOID Buffer, UINT32 Length);

BOOLEAN
KdRegisterEventInDebuggee(PDEBUGGER_GENERAL_EVENT_DETAIL EventRegBuffer,
                          UINT32                         Length);

BOOLEAN
KdAddActionToEventInDebuggee(PDEBUGGER_GENERAL_ACTION ActionAddingBuffer,
                             UINT32                   Length);

BOOLEAN
KdSendModifyEventInDebuggee(PDEBUGGER_MODIFY_EVENTS ModifyEvent);

BOOLEAN
KdSendGeneralBuffersFromDebuggeeToDebugger(
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION RequestedAction,
    PVOID                                   Buffer,
    UINT32                                  BufferLength,
    BOOLEAN                                 PauseDebuggeeWhenSent);

BOOLEAN
KdCloseConnection();

BOOLEAN
KdReloadSymbolsInDebuggee(BOOLEAN PauseDebuggee, UINT32 UserProcessId);

VOID
KdUninitializeConnection();

VOID
KdSendUsermodePrints(CHAR * Input, UINT32 Length);

VOID
KdSendSymbolDetailPacket(PMODULE_SYMBOL_DETAIL SymbolDetailPacket,
                         UINT32                CurrentSymbolInfoIndex,
                         UINT32                TotalSymbols);

VOID
KdHandleUserInputInDebuggee(DEBUGGEE_USER_INPUT_PACKET * Descriptor);

VOID
KdTheRemoteSystemIsRunning();

VOID
KdBreakControlCheckAndPauseDebugger();

VOID
KdBreakControlCheckAndContinueDebugger();

VOID
KdSetStatusAndWaitForPause();
