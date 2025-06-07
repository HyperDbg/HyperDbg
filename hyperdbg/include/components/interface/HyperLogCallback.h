/**
 * @file HyperLogCallback.h
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
