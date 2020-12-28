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
