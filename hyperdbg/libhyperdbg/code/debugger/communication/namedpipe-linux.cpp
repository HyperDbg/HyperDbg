/**
 * @file namedpipe-linux.cpp
 * @author Max Raulea (max.raulea@hyperdbg.org)
 * @brief Linux stub implementations of the named-pipe transport (namedpipe.cpp)
 * @details The Windows implementation (namedpipe.cpp) is a thin wrapper over the
 *          Win32 named-pipe IPC API: the server side uses
 *          CreateNamedPipe/ConnectNamedPipe/ReadFile/WriteFile, and the client
 *          side uses CreateFileA against the "\\.\pipe\..." name plus overlapped
 *          ReadFile/WriteFile with the g_OverlappedIoStructureFor*Debugger
 *          events. None of that maps 1:1 onto Linux, so the whole translation
 *          unit is swapped out on Linux (CMake `if(UNIX)` REMOVE_ITEM
 *          namedpipe.cpp + APPEND namedpipe-linux.cpp), mirroring the
 *          symbol.cpp -> symbol-linux.cpp / pe-parser.cpp -> pe-parser-linux.cpp
 *          / install.cpp -> install-linux.cpp pattern. namedpipe.cpp itself is
 *          left 100% untouched for the Windows build. Only the 10 public
 *          functions declared in namedpipe.h are provided here; the two internal
 *          *Example() demo functions are not part of the interface and simply do
 *          not exist in the Linux TU.
 *
 *          The Create* entry points return INVALID_HANDLE_VALUE, so every caller
 *          bails before reaching the send/read/close paths — those stay silent to
 *          avoid spamming a message on each loop iteration; only the Create*
 *          functions emit the "not supported" note.
 *
 *          TODO(Linux) to make these real: back the transport with either a
 *          filesystem FIFO (mkfifo(3), matching the "named pipe" naming most
 *          closely) or, more usefully for bidirectional message framing, a Unix
 *          domain socket (AF_UNIX, socket/bind/listen/accept on the server side,
 *          socket/connect on the client side) whose path is derived from the
 *          "\\.\pipe\NAME" string. The overlapped/event machinery collapses to
 *          plain blocking read()/write() (or poll()) since a dedicated thread
 *          already owns each direction.
 *
 * @version 0.1
 * @date 2026-07-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#ifdef __linux__

////////////////////////////////////////////////////////////////////////////
//                            Server Side                                 //
////////////////////////////////////////////////////////////////////////////

/**
 * @brief Create a named-pipe server endpoint.
 * @return HANDLE INVALID_HANDLE_VALUE — named pipes are not supported on Linux
 *         yet (see file header for the FIFO / AF_UNIX plan).
 */
HANDLE
NamedPipeServerCreatePipe(LPCSTR PipeName, UINT32 OutputBufferSize, UINT32 InputBufferSize)
{
    UNREFERENCED_PARAMETER(PipeName);
    UNREFERENCED_PARAMETER(OutputBufferSize);
    UNREFERENCED_PARAMETER(InputBufferSize);

    ShowMessages("err, named-pipe communication is not supported on Linux yet\n");
    return INVALID_HANDLE_VALUE;
}

/**
 * @brief Wait for a client to connect to the server pipe.
 * @return BOOLEAN FALSE — not supported on Linux yet.
 */
BOOLEAN
NamedPipeServerWaitForClientConntection(HANDLE PipeHandle)
{
    UNREFERENCED_PARAMETER(PipeHandle);
    return FALSE;
}

/**
 * @brief Read a message sent by the connected client.
 * @return UINT32 0 — not supported on Linux yet.
 */
UINT32
NamedPipeServerReadClientMessage(HANDLE PipeHandle, CHAR * BufferToSave, INT MaximumReadBufferLength)
{
    UNREFERENCED_PARAMETER(PipeHandle);
    UNREFERENCED_PARAMETER(BufferToSave);
    UNREFERENCED_PARAMETER(MaximumReadBufferLength);
    return 0;
}

/**
 * @brief Send a message to the connected client.
 * @return BOOLEAN FALSE — not supported on Linux yet.
 */
BOOLEAN
NamedPipeServerSendMessageToClient(HANDLE PipeHandle,
                                   CHAR * BufferToSend,
                                   INT    BufferSize)
{
    UNREFERENCED_PARAMETER(PipeHandle);
    UNREFERENCED_PARAMETER(BufferToSend);
    UNREFERENCED_PARAMETER(BufferSize);
    return FALSE;
}

/**
 * @brief Close the server pipe handle.
 * @return VOID no-op — not supported on Linux yet.
 */
VOID
NamedPipeServerCloseHandle(HANDLE PipeHandle)
{
    UNREFERENCED_PARAMETER(PipeHandle);
}

////////////////////////////////////////////////////////////////////////////
//                            Client Side                                 //
////////////////////////////////////////////////////////////////////////////

/**
 * @brief Connect to a named-pipe server endpoint.
 * @return HANDLE INVALID_HANDLE_VALUE — not supported on Linux yet.
 */
HANDLE
NamedPipeClientCreatePipe(LPCSTR PipeName)
{
    UNREFERENCED_PARAMETER(PipeName);

    ShowMessages("err, named-pipe communication is not supported on Linux yet\n");
    return INVALID_HANDLE_VALUE;
}

/**
 * @brief Connect to a named-pipe server endpoint using overlapped I/O.
 * @return HANDLE INVALID_HANDLE_VALUE — not supported on Linux yet.
 */
HANDLE
NamedPipeClientCreatePipeOverlappedIo(LPCSTR PipeName)
{
    UNREFERENCED_PARAMETER(PipeName);

    ShowMessages("err, named-pipe communication is not supported on Linux yet\n");
    return INVALID_HANDLE_VALUE;
}

/**
 * @brief Send a message to the server over the client pipe.
 * @return BOOLEAN FALSE — not supported on Linux yet.
 */
BOOLEAN
NamedPipeClientSendMessage(HANDLE PipeHandle, CHAR * BufferToSend, INT BufferSize)
{
    UNREFERENCED_PARAMETER(PipeHandle);
    UNREFERENCED_PARAMETER(BufferToSend);
    UNREFERENCED_PARAMETER(BufferSize);
    return FALSE;
}

/**
 * @brief Read a message from the server over the client pipe.
 * @return UINT32 0 — not supported on Linux yet.
 */
UINT32
NamedPipeClientReadMessage(HANDLE PipeHandle, CHAR * BufferToRead, INT MaximumSizeOfBuffer)
{
    UNREFERENCED_PARAMETER(PipeHandle);
    UNREFERENCED_PARAMETER(BufferToRead);
    UNREFERENCED_PARAMETER(MaximumSizeOfBuffer);
    return 0;
}

/**
 * @brief Close the client pipe handle.
 * @return VOID no-op — not supported on Linux yet.
 */
VOID
NamedPipeClientClosePipe(HANDLE PipeHandle)
{
    UNREFERENCED_PARAMETER(PipeHandle);
}

#endif // __linux__
