/**
 * @file Callback.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Header for VMM callback interface routines
 * @details
 *
 * @version 0.2
 * @date 2023-01-29
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//
// Log Callbacks
//

BOOLEAN
LogCallbackPrepareAndSendMessageToQueue(UINT32       OperationCode,
                                        BOOLEAN      IsImmediateMessage,
                                        BOOLEAN      ShowCurrentSystemTime,
                                        BOOLEAN      Priority,
                                        const char * Fmt,
                                        ...);

BOOLEAN
LogCallbackSendMessageToQueue(UINT32  OperationCode,
                              BOOLEAN IsImmediateMessage,
                              CHAR *  LogMessage,
                              UINT32  BufferLen,
                              BOOLEAN Priority);

BOOLEAN
LogCallbackCheckIfBufferIsFull(BOOLEAN Priority);

BOOLEAN
LogCallbackSendBuffer(_In_ UINT32                          OperationCode,
                      _In_reads_bytes_(BufferLength) PVOID Buffer,
                      _In_ UINT32                          BufferLength,
                      _In_ BOOLEAN                         Priority);

//
// VMM Callbacks
//

VMM_CALLBACK_TRIGGERING_EVENT_STATUS_TYPE
VmmCallbackTriggerEvents(VMM_EVENT_TYPE_ENUM                   EventType,
                         VMM_CALLBACK_EVENT_CALLING_STAGE_TYPE CallingStage,
                         PVOID                                 Context,
                         BOOLEAN *                             PostEventRequired,
                         GUEST_REGS *                          Regs);

BOOLEAN
VmmCallbackVmcallHandler(UINT32 CoreId,
                         UINT64 VmcallNumber,
                         UINT64 OptionalParam1,
                         UINT64 OptionalParam2,
                         UINT64 OptionalParam3);

BOOLEAN
VmmCallbackQueryTerminateProtectedResource(UINT32                               CoreId,
                                           PROTECTED_HV_RESOURCES_TYPE          ResourceType,
                                           PVOID                                Context,
                                           PROTECTED_HV_RESOURCES_PASSING_OVERS PassOver);

BOOLEAN
VmmCallbackRestoreEptState(UINT32 CoreId);

BOOLEAN
VmmCallbackUnhandledEptViolation(UINT32 CoreId,
                                 UINT64 ViolationQualification,
                                 UINT64 GuestPhysicalAddr);

VOID
VmmCallbackSetLastError(UINT32 LastError);

VOID
VmmCallbackRegisteredMtfHandler(UINT32 CoreId);

VOID
VmmCallbackNmiBroadcastRequestHandler(UINT32 CoreId, BOOLEAN IsOnVmxNmiHandler);

//
// Debugging Callbacks
//

BOOLEAN
DebuggingCallbackHandleBreakpointException(UINT32 CoreId);

BOOLEAN
DebuggingCallbackHandleDebugBreakpointException(UINT32 CoreId);

BOOLEAN
DebuggingCallbackConditionalPageFaultException(UINT32 CoreId,
                                               UINT64 Address,
                                               UINT32 PageFaultErrorCode);

//
// Interception Callbacks
//

VOID
InterceptionCallbackTriggerCr3ProcessChange(UINT32 CoreId);

VOID
InterceptionCallbackCr3VmexitsForThreadInterception(UINT32 CoreId, CR3_TYPE NewCr3);
