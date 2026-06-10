/**
 * @file namedpipe.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Named pipe communication headers
 * @details
 * @version 0.1
 * @date 2020-04-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

////////////////////////////////////////////////////////////////////////////
//                            Server Side                                 //
////////////////////////////////////////////////////////////////////////////

HANDLE
NamedPipeServerCreatePipe(LPCSTR PipeName, UINT32 OutputBufferSize, UINT32 InputBufferSize);

BOOLEAN
NamedPipeServerWaitForClientConntection(HANDLE PipeHandle);

UINT32
NamedPipeServerReadClientMessage(HANDLE PipeHandle, CHAR * BufferToSave, INT MaximumReadBufferLength);

BOOLEAN
NamedPipeServerSendMessageToClient(HANDLE PipeHandle,
                                   CHAR * BufferToSend,
                                   INT    BufferSize);

VOID
NamedPipeServerCloseHandle(HANDLE PipeHandle);

////////////////////////////////////////////////////////////////////////////
//                            Client Side                                 //
////////////////////////////////////////////////////////////////////////////

HANDLE
NamedPipeClientCreatePipe(LPCSTR PipeName);

HANDLE
NamedPipeClientCreatePipeOverlappedIo(LPCSTR PipeName);

BOOLEAN
NamedPipeClientSendMessage(HANDLE PipeHandle, CHAR * BufferToSend, INT BufferSize);

UINT32
NamedPipeClientReadMessage(HANDLE PipeHandle, CHAR * BufferToRead, INT MaximumSizeOfBuffer);

VOID
NamedPipeClientClosePipe(HANDLE PipeHandle);
