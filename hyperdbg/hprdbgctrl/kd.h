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

VOID KdTheRemoteSystemIsRunning();

BOOLEAN KdPrepareSerialConnectionToRemoteSystem(HANDLE SerialHandle,
                                                BOOLEAN IsNamedPipe);

BOOLEAN KdPrepareAndConnectDebugPort(const char *PortName, DWORD Baudrate,
                                     UINT32 Port, BOOLEAN IsPreparing,
                                     BOOLEAN IsNamedPipe);

BOOLEAN KdSendPacketToDebuggee(const CHAR *Buffer, UINT32 Length);

BOOLEAN KdReceivePacketFromDebuggee(CHAR *BufferToSave, UINT32 *LengthReceived);

VOID KdBreakControlCheckAndContinueDebugger();

BOOLEAN KdCheckForTheEndOfTheBuffer(PUINT32 CurrentLoopIndex, BYTE *Buffer);

BOOLEAN
KdSendStepPacketToDebuggee(DEBUGGER_REMOTE_STEPPING_REQUEST StepRequestType);
