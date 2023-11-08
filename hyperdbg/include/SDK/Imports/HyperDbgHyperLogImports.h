/**
 * @file HyperDbgHyperLogImports.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers relating exported functions from hyperlog project
 * @version 0.1
 * @date 2023-01-15
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#ifdef HYPERDBG_HYPER_LOG
#    define IMPORT_EXPORT_HYPERLOG __declspec(dllexport)
#else
#    define IMPORT_EXPORT_HYPERLOG __declspec(dllimport)
#endif

//////////////////////////////////////////////////
//				   Functions					//
//////////////////////////////////////////////////

IMPORT_EXPORT_HYPERLOG BOOLEAN
LogInitialize(MESSAGE_TRACING_CALLBACKS * MsgTracingCallbacks);

IMPORT_EXPORT_HYPERLOG VOID
LogUnInitialize();

IMPORT_EXPORT_HYPERLOG UINT32
LogMarkAllAsRead(BOOLEAN IsVmxRoot);

IMPORT_EXPORT_HYPERLOG BOOLEAN
LogCallbackPrepareAndSendMessageToQueue(UINT32       OperationCode,
                                        BOOLEAN      IsImmediateMessage,
                                        BOOLEAN      ShowCurrentSystemTime,
                                        BOOLEAN      Priority,
                                        const char * Fmt,
                                        ...);

IMPORT_EXPORT_HYPERLOG BOOLEAN
LogCallbackPrepareAndSendMessageToQueueWrapper(UINT32       OperationCode,
                                               BOOLEAN      IsImmediateMessage,
                                               BOOLEAN      ShowCurrentSystemTime,
                                               BOOLEAN      Priority,
                                               const char * Fmt,
                                               va_list      ArgList);

IMPORT_EXPORT_HYPERLOG BOOLEAN
LogCallbackSendBuffer(_In_ UINT32                          OperationCode,
                      _In_reads_bytes_(BufferLength) PVOID Buffer,
                      _In_ UINT32                          BufferLength,
                      _In_ BOOLEAN                         Priority);

IMPORT_EXPORT_HYPERLOG BOOLEAN
LogCallbackCheckIfBufferIsFull(BOOLEAN Priority);

IMPORT_EXPORT_HYPERLOG BOOLEAN
LogCallbackSendMessageToQueue(UINT32 OperationCode, BOOLEAN IsImmediateMessage, CHAR * LogMessage, UINT32 BufferLen, BOOLEAN Priority);

IMPORT_EXPORT_HYPERLOG NTSTATUS
LogRegisterEventBasedNotification(PDEVICE_OBJECT DeviceObject, PIRP Irp);

IMPORT_EXPORT_HYPERLOG NTSTATUS
LogRegisterIrpBasedNotification(PDEVICE_OBJECT DeviceObject, PIRP Irp);
