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
NamedPipeServerWaitForClientConnection(HANDLE PipeHandle);

UINT32
NamedPipeServerReadClientMessage(HANDLE PipeHandle, CHAR * BufferToSave, INT32 MaximumReadBufferLength);

BOOLEAN
NamedPipeServerSendMessageToClient(HANDLE PipeHandle,
                                   CHAR * BufferToSend,
                                   INT32  BufferSize);

VOID
NamedPipeServerCloseHandle(HANDLE PipeHandle);

////////////////////////////////////////////////////////////////////////////
//                            Client Side                                 //
////////////////////////////////////////////////////////////////////////////

HANDLE
NamedPipeClientCreatePipe(LPCSTR PipeName);

BOOLEAN
NamedPipeClientSendMessage(HANDLE PipeHandle, CHAR * BufferToSend, INT32 BufferSize);

UINT32
NamedPipeClientReadMessage(HANDLE PipeHandle, CHAR * BufferToRead, INT32 MaximumSizeOfBuffer);

VOID
NamedPipeClientClosePipe(HANDLE PipeHandle);
