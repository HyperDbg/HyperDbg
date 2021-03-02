/**
 * @file kd.h
 * @author Sina Karvandi (sina@rayanfam.com)
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
//		    Display Windows Details             //
//////////////////////////////////////////////////

struct HKeyHolder {

private:
  HKEY m_Key;

public:
  HKeyHolder() : m_Key(nullptr) {}

  HKeyHolder(const HKeyHolder &) = delete;
  HKeyHolder &operator=(const HKeyHolder &) = delete;

  ~HKeyHolder() {
    if (m_Key != nullptr)
      RegCloseKey(m_Key);
  }

  operator HKEY() const { return m_Key; }

  HKEY *operator&() { return &m_Key; }
};

//////////////////////////////////////////////////
//			       	 Enums                      //
//////////////////////////////////////////////////

/**
 * @brief stepping types
 *
 */
typedef enum _DEBUGGER_REMOTE_STEPPING_REQUEST {

  DEBUGGER_REMOTE_STEPPING_REQUEST_STEP_OUT,
  DEBUGGER_REMOTE_STEPPING_REQUEST_STEP_IN,
  DEBUGGER_REMOTE_STEPPING_REQUEST_STEP_OUT_WITH_REGS,
  DEBUGGER_REMOTE_STEPPING_REQUEST_STEP_IN_WITH_REGS,

} DEBUGGER_REMOTE_STEPPING_REQUEST;

//////////////////////////////////////////////////
//			    	 Functions                  //
//////////////////////////////////////////////////

VOID KdBreakControlCheckAndPauseDebugger();

BOOLEAN KdCommandPacketToDebuggee(
    DEBUGGER_REMOTE_PACKET_TYPE PacketType,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION RequestedAction);

BOOLEAN KdCommandPacketAndBufferToDebuggee(
    DEBUGGER_REMOTE_PACKET_TYPE PacketType,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION RequestedAction, CHAR *Buffer,
    UINT32 BufferLength);

VOID KdTheRemoteSystemIsRunning();

BOOLEAN KdPrepareSerialConnectionToRemoteSystem(HANDLE SerialHandle,
                                                BOOLEAN IsNamedPipe);

BOOLEAN KdPrepareAndConnectDebugPort(const char *PortName, DWORD Baudrate,
                                     UINT32 Port, BOOLEAN IsPreparing,
                                     BOOLEAN IsNamedPipe);

BOOLEAN KdSendPacketToDebuggee(const CHAR *Buffer, UINT32 Length,
                               BOOLEAN SendEndOfBuffer);

BOOLEAN KdReceivePacketFromDebuggee(CHAR *BufferToSave, UINT32 *LengthReceived);

VOID KdBreakControlCheckAndContinueDebugger();

BOOLEAN KdCheckForTheEndOfTheBuffer(PUINT32 CurrentLoopIndex, BYTE *Buffer);

BOOLEAN KdSendSwitchCorePacketToDebuggee(UINT32 NewCore);

BOOLEAN KdSendEventQueryAndModifyPacketToDebuggee(
    UINT64 Tag, DEBUGGER_MODIFY_EVENTS_TYPE TypeOfAction, BOOLEAN *IsEnabled);

BOOLEAN KdSendFlushPacketToDebuggee();

BOOLEAN KdSendReadRegisterPacketToDebuggee(PDEBUGGEE_REGISTER_READ_DESCRIPTION);

PDEBUGGER_EVENT_AND_ACTION_REG_BUFFER
KdSendRegisterEventPacketToDebuggee(PDEBUGGER_GENERAL_EVENT_DETAIL Event,
                                    UINT32 EventBufferLength);

PDEBUGGER_EVENT_AND_ACTION_REG_BUFFER
KdSendAddActionToEventPacketToDebuggee(PDEBUGGER_GENERAL_ACTION GeneralAction,
                                       UINT32 GeneralActionLength);

BOOLEAN KdSendSwitchProcessPacketToDebuggee(BOOLEAN GetRemotePid,
                                            UINT32 NewPid);

BOOLEAN KdSendScriptPacketToDebuggee(UINT64 BufferAddress, UINT32 BufferLength,
                                     UINT32 Pointer, BOOLEAN IsFormat);

BOOLEAN KdSendUserInputPacketToDebuggee(const char *Sendbuf, int Len);

BOOLEAN
KdSendStepPacketToDebuggee(DEBUGGER_REMOTE_STEPPING_REQUEST StepRequestType);

BYTE KdComputeDataChecksum(PVOID Buffer, UINT32 Length);

VOID KdHandleUserInputInDebuggee(CHAR *Input);

BOOLEAN KdRegisterEventInDebuggee(PDEBUGGER_GENERAL_EVENT_DETAIL EventRegBuffer,
                                  UINT32 Length);

BOOLEAN
KdAddActionToEventInDebuggee(PDEBUGGER_GENERAL_ACTION ActionAddingBuffer,
                             UINT32 Length);

BOOLEAN KdSendModifyEventInDebuggee(PDEBUGGER_MODIFY_EVENTS ModifyEvent);

VOID KdSendUsermodePrints(CHAR *Input, UINT32 Length);

BOOLEAN KdSendGeneralBuffersFromDebuggeeToDebugger(
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION RequestedAction, PVOID Buffer,
    UINT32 BufferLength, BOOLEAN PauseDebuggeeWhenSent);

BOOLEAN KdCloseConnection();

VOID KdUninitializeConnection();
