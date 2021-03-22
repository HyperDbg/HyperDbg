/**
 * @file namedpipe.h
 * @author Sina Karvandi (sina@rayanfam.com)
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
NamedPipeServerReadClientMessage(HANDLE PipeHandle, char * BufferToSave, int MaximumReadBufferLength);

BOOLEAN
NamedPipeServerSendMessageToClient(HANDLE PipeHandle,
                                   char * BufferToSend,
                                   int    BufferSize);

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
NamedPipeClientSendMessage(HANDLE PipeHandle, char * BufferToSend, int BufferSize);

UINT32
NamedPipeClientReadMessage(HANDLE PipeHandle, char * BufferToRead, int MaximumSizeOfBuffer);

VOID
NamedPipeClientClosePipe(HANDLE PipeHandle);
