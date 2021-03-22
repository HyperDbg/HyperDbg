/**
 * @file namedpipe.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Server and Client communication over NamedPipes
 * @details
 * @version 0.1
 * @date 2020-07-15
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern OVERLAPPED g_OverlappedIoStructureForReadDebugger;
extern OVERLAPPED g_OverlappedIoStructureForWriteDebugger;

////////////////////////////////////////////////////////////////////////////
//                                                                        //
//                            Server Side                                 //
//                                                                        //
////////////////////////////////////////////////////////////////////////////

/**
 * @brief Create a named pipe server
 *
 * @param PipeName
 * @param OutputBufferSize
 * @param InputBufferSize
 * @return HANDLE
 */
HANDLE
NamedPipeServerCreatePipe(LPCSTR PipeName, UINT32 OutputBufferSize, UINT32 InputBufferSize)
{
    HANDLE hPipe;

    hPipe = CreateNamedPipeA(PipeName,                   // pipe name
                             PIPE_ACCESS_DUPLEX,         // read/write access
                             PIPE_TYPE_MESSAGE |         // message type pipe
                                 PIPE_READMODE_MESSAGE | // message-read mode
                                 PIPE_WAIT,              // blocking mode
                             PIPE_UNLIMITED_INSTANCES,   // max. instances
                             OutputBufferSize,           // output buffer size
                             InputBufferSize,            // input buffer size
                             NMPWAIT_USE_DEFAULT_WAIT,   // client time-out
                             NULL);                      // default security attribute

    if (INVALID_HANDLE_VALUE == hPipe)
    {
        ShowMessages("Error occurred while "
                     "creating the pipe: %d\n",
                     GetLastError());
        return NULL;
    }
    return hPipe;
}

/**
 * @brief wait for client conncetion
 *
 * @param PipeHandle
 * @return BOOLEAN
 */
BOOLEAN
NamedPipeServerWaitForClientConntection(HANDLE PipeHandle)
{
    //
    // Wait for the client to connect
    //
    BOOL bClientConnected = ConnectNamedPipe(PipeHandle, NULL);

    if (FALSE == bClientConnected)
    {
        ShowMessages("Error occurred while connecting"
                     " to the client: %d\n",
                     GetLastError());
        CloseHandle(PipeHandle);
        return FALSE;
    }

    //
    // Client connected
    //
    return TRUE;
}

/**
 * @brief read client message from the named pipe
 *
 * @param PipeHandle
 * @param BufferToSave
 * @param MaximumReadBufferLength
 * @return UINT32
 */
UINT32
NamedPipeServerReadClientMessage(HANDLE PipeHandle, char * BufferToSave, int MaximumReadBufferLength)
{
    DWORD cbBytes;

    //
    // We are connected to the client.
    // To communicate with the client
    // we will use ReadFile()/WriteFile()
    // on the pipe handle - hPipe
    //

    //
    // Read client message
    //
    BOOL bResult = ReadFile(PipeHandle,              // handle to pipe
                            BufferToSave,            // buffer to receive data
                            MaximumReadBufferLength, // size of buffer
                            &cbBytes,                // number of bytes read
                            NULL);                   // not overlapped I/O

    if ((!bResult) || (0 == cbBytes))
    {
        ShowMessages("\nError occurred while reading "
                     "from the client: %d",
                     GetLastError());
        CloseHandle(PipeHandle);
        return 0;
    }

    //
    // Number of bytes that the client sends to us
    //
    return cbBytes;
}

BOOLEAN
NamedPipeServerSendMessageToClient(HANDLE PipeHandle,
                                   char * BufferToSend,
                                   int    BufferSize)
{
    DWORD cbBytes;

    //
    // Reply to client
    //
    BOOLEAN bResult =
        WriteFile(PipeHandle,   // handle to pipe
                  BufferToSend, // buffer to write from
                  BufferSize,   // number of bytes to write, include the NULL
                  &cbBytes,     // number of bytes written
                  NULL);        // not overlapped I/O

    if ((!bResult) || (BufferSize != cbBytes))
    {
        ShowMessages("\nError occurred while writing"
                     " to the client: %d",
                     GetLastError());
        CloseHandle(PipeHandle);
        return FALSE;
    }
    return TRUE;
}

/**
 * @brief Close handle of server's named pipe
 *
 * @param PipeHandle
 * @return VOID
 */
VOID
NamedPipeServerCloseHandle(HANDLE PipeHandle)
{
    CloseHandle(PipeHandle);
}

//**************************************************************************

////////////////////////////////////////////////////////////////////////////
//                                                                        //
//                            Client Side                                 //
//                                                                        //
////////////////////////////////////////////////////////////////////////////

/**
 * @brief Create a client named pipe
 * @details Pipe name format - \\servername\pipe\pipename
 * This pipe is for server on the same computer,
 * however, pipes can be used to connect to a remote server
 *
 * @param PipeName
 * @return HANDLE
 */
HANDLE
NamedPipeClientCreatePipe(LPCSTR PipeName)
{
    HANDLE hPipe;

    //
    // Connect to the server pipe using CreateFile()
    //
    hPipe = CreateFileA(PipeName,      // pipe name
                        GENERIC_READ | // read and write access
                            GENERIC_WRITE,
                        0,             // no sharing
                        NULL,          // default security attributes
                        OPEN_EXISTING, // opens existing pipe
                        0,             // default attributes
                        NULL);         // no template file

    if (INVALID_HANDLE_VALUE == hPipe)
    {
        ShowMessages("Error occurred while connecting"
                     " to the server: %d\n",
                     GetLastError());
        //
        // One might want to check whether the server pipe is busy
        // This sample will error out if the server pipe is busy
        // Read on ERROR_PIPE_BUSY and WaitNamedPipe() for that
        //

        //
        // Error
        //
        return NULL;
    }
    else
    {
        return hPipe;
    }
}

/**
 * @brief Create a client named pipe through overlapped I/O
 * @details Pipe name format - \\servername\pipe\pipename
 * This pipe is for server on the same computer,
 * however, pipes can be used to connect to a remote server
 *
 * @param PipeName
 * @return HANDLE
 */
HANDLE
NamedPipeClientCreatePipeOverlappedIo(LPCSTR PipeName)
{
    HANDLE hPipe;

    //
    // Connect to the server pipe using CreateFile()
    //
    hPipe = CreateFileA(PipeName,      // pipe name
                        GENERIC_READ | // read and write access
                            GENERIC_WRITE,
                        0,                    // no sharing
                        NULL,                 // default security attributes
                        OPEN_EXISTING,        // opens existing pipe
                        FILE_FLAG_OVERLAPPED, // Overlapped I/O
                        NULL);                // no template file

    if (INVALID_HANDLE_VALUE == hPipe)
    {
        ShowMessages("Error occurred while connecting"
                     " to the server: %d\n",
                     GetLastError());
        //
        // One might want to check whether the server pipe is busy
        // This sample will error out if the server pipe is busy
        // Read on ERROR_PIPE_BUSY and WaitNamedPipe() for that
        //

        //
        // Error
        //
        return NULL;
    }
    else
    {
        //
        // Create event for overlapped I/O (Read)
        //
        g_OverlappedIoStructureForReadDebugger.hEvent =
            CreateEvent(NULL, TRUE, FALSE, NULL);

        //
        // Create event for overlapped I/O (Write)
        //
        g_OverlappedIoStructureForWriteDebugger.hEvent =
            CreateEvent(NULL, TRUE, FALSE, NULL);

        return hPipe;
    }
}

/**
 * @brief send client message over named pipe
 *
 * @param PipeHandle
 * @param BufferToSend
 * @param BufferSize
 * @return BOOLEAN
 */
BOOLEAN
NamedPipeClientSendMessage(HANDLE PipeHandle, char * BufferToSend, int BufferSize)
{
    //
    // We are done connecting to the server pipe,
    // we can start communicating with
    // the server using ReadFile()/WriteFile()
    // on handle - hPipe
    //

    DWORD cbBytes;

    //
    // Send the message to server
    //
    BOOL bResult =
        WriteFile(PipeHandle,   // handle to pipe
                  BufferToSend, // buffer to write from
                  BufferSize,   // number of bytes to write, include the NULL
                  &cbBytes,     // number of bytes written
                  NULL);        // not overlapped I/O

    if ((!bResult) || (BufferSize != cbBytes))
    {
        ShowMessages("Error occurred while writing"
                     " to the server: %d\n",
                     GetLastError());
        CloseHandle(PipeHandle);

        //
        // Error
        //
        CloseHandle(PipeHandle);
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

//
// Read the count of read buffer
//
UINT32
NamedPipeClientReadMessage(HANDLE PipeHandle, char * BufferToRead, int MaximumSizeOfBuffer)
{
    DWORD cbBytes;

    //
    // Read server response
    //
    BOOL bResult = ReadFile(PipeHandle,          // handle to pipe
                            BufferToRead,        // buffer to receive data
                            MaximumSizeOfBuffer, // size of buffer
                            &cbBytes,            // number of bytes read
                            NULL);               // not overlapped I/O

    if ((!bResult) || (0 == cbBytes))
    {
        ShowMessages("\nError occurred while reading"
                     " from the server: %d",
                     GetLastError());
        CloseHandle(PipeHandle);
        return NULL; // Error
    }

    //
    // Success
    //
    return cbBytes;
}

/**
 * @brief close named pipe handle of client
 *
 * @param PipeHandle
 * @return VOID
 */
VOID
NamedPipeClientClosePipe(HANDLE PipeHandle)
{
    CloseHandle(PipeHandle);
}

////////////////////////////////////////////////////////////////////////////
//                                                                        //
//                            Example Server                              //
//                                                                        //
////////////////////////////////////////////////////////////////////////////

/**
 * @brief and example of how to use named pipe as a server
 *
 * @return int
 */
int
NamedPipeServerExample()
{
    HANDLE    PipeHandle;
    BOOLEAN   SentMessageResult;
    UINT32    ReadBytes;
    const int BufferSize               = 1024;
    char      BufferToRead[BufferSize] = {0};
    char      BufferToSend[BufferSize] = "test message to send from server !!!";

    PipeHandle = NamedPipeServerCreatePipe("\\\\.\\Pipe\\HyperDbgTests",
                                           BufferSize,
                                           BufferSize);
    if (!PipeHandle)
    {
        //
        // Error in creating handle
        //
        return 1;
    }

    if (!NamedPipeServerWaitForClientConntection(PipeHandle))
    {
        //
        // Error in connection
        //
        return 1;
    }

    ReadBytes =
        NamedPipeServerReadClientMessage(PipeHandle, BufferToRead, BufferSize);

    if (!ReadBytes)
    {
        //
        // Nothing to read
        //
        return 1;
    }

    ShowMessages("Message from client : %s\n", BufferToRead);

    SentMessageResult = NamedPipeServerSendMessageToClient(
        PipeHandle,
        BufferToSend,
        strlen(BufferToSend) + 1);

    if (!SentMessageResult)
    {
        //
        // error in sending
        //
        return 1;
    }

    NamedPipeServerCloseHandle(PipeHandle);

    return 0;
}

////////////////////////////////////////////////////////////////////////////
//                                                                        //
//                            Example Client                              //
//                                                                        //
////////////////////////////////////////////////////////////////////////////

/**
 * @brief and example of how to use named pipe as a client
 *
 * @return int
 */
int
NamedPipeClientExample()
{
    HANDLE    PipeHandle;
    BOOLEAN   SentMessageResult;
    UINT32    ReadBytes;
    const int BufferSize         = 1024;
    char      Buffer[BufferSize] = "test message to send from client !!!";

    PipeHandle = NamedPipeClientCreatePipe("\\\\.\\Pipe\\HyperDbgTests");

    if (!PipeHandle)
    {
        //
        // Unable to create handle
        //
        return 1;
    }

    SentMessageResult =
        NamedPipeClientSendMessage(PipeHandle, Buffer, strlen(Buffer) + 1);

    if (!SentMessageResult)
    {
        //
        // Sending error
        //
        return 1;
    }

    ReadBytes = NamedPipeClientReadMessage(PipeHandle, Buffer, BufferSize);

    if (!ReadBytes)
    {
        //
        // Nothing to read
        //
        return 1;
    }

    ShowMessages("Server sent the following message: %s\n", Buffer);

    NamedPipeClientClosePipe(PipeHandle);

    return 0;
}
