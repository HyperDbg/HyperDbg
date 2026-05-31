/**
 * @file namedpipe.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Server and Client communication over NamedPipes
 * @details
 * @version 0.1
 * @date 2020-07-15
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Connect and transfer buffers via named pipe
 *
 * @return UINT32
 */
UINT32
NamedPipeConnectingAndTransferringBuffers()
{
    // HANDLE  PipeHandle;
    // BOOLEAN SentMessageResult;
    // UINT32  ReadBytes;
    // char *  Buffer;
    //
    // Buffer = (char *)malloc(TEST_CASE_MAXIMUM_BUFFERS_TO_COMMUNICATE);
    //
    // if (!Buffer)
    // {
    //     printf("err, could not allocate communication buffer\n");
    //     _getch();
    //     return 1;
    // }
    //
    // RtlZeroMemory(Buffer, TEST_CASE_MAXIMUM_BUFFERS_TO_COMMUNICATE);
    // strcpy_s(Buffer, TEST_CASE_MAXIMUM_BUFFERS_TO_COMMUNICATE, "Hey there, Are you HyperDbg?");
    //
    // //
    // // Perform our shaking with HyperDbg
    // //
    //
    // //
    // // It's not called directly, it's probably from HyperDbg
    // //
    // PipeHandle = NamedPipeClientCreatePipe("\\\\.\\Pipe\\HyperDbgTests");
    //
    // if (!PipeHandle)
    // {
    //     //
    //     // Unable to create handle
    //     //
    //     free(Buffer);
    //
    //     printf("err, unable to create handle\n");
    //     _getch();
    //     return 1;
    // }
    //
    // SentMessageResult =
    //     NamedPipeClientSendMessage(PipeHandle, Buffer, (int)strlen(Buffer) + 1);
    //
    // if (!SentMessageResult)
    // {
    //     //
    //     // Sending error
    //     //
    //     free(Buffer);
    //
    //     printf("err, unable to send message\n");
    //     _getch();
    //     return 1;
    // }
    //
    // ReadBytes = NamedPipeClientReadMessage(PipeHandle, Buffer, TEST_CASE_MAXIMUM_BUFFERS_TO_COMMUNICATE);
    //
    // if (!ReadBytes)
    // {
    //     //
    //     // Nothing to read
    //     //
    //     free(Buffer);
    //
    //     printf("err, unable to read message\n");
    //     _getch();
    //     return 1;
    // }
    //
    // if (strcmp(Buffer,
    //            "Hello, Dear Test Process... Yes, I'm HyperDbg Debugger :)") ==
    //     0)
    // {
    //     //
    //     // *** Connected to the HyperDbg debugger ***
    //     //
    //
    //     //
    //     // Now we should request the test case number from the HyperDbg Debugger
    //     //
    //     RtlZeroMemory(Buffer, TEST_CASE_MAXIMUM_BUFFERS_TO_COMMUNICATE);
    //
    //     strcpy_s(
    //         Buffer,
    //         TEST_CASE_MAXIMUM_BUFFERS_TO_COMMUNICATE,
    //         "Wow! I miss you... Would you plz send test cases?");
    //
    //     SentMessageResult =
    //         NamedPipeClientSendMessage(PipeHandle, Buffer, (int)strlen(Buffer) + 1);
    //
    //     if (!SentMessageResult)
    //     {
    //         //
    //         // Sending error
    //         //
    //         free(Buffer);
    //
    //         printf("err, sending error\n");
    //         _getch();
    //         return 1;
    //     }
    //
    //     //
    //     // Read the test case number
    //     //
    //     RtlZeroMemory(Buffer, TEST_CASE_MAXIMUM_BUFFERS_TO_COMMUNICATE);
    //     ReadBytes = NamedPipeClientReadMessage(PipeHandle, Buffer, TEST_CASE_MAXIMUM_BUFFERS_TO_COMMUNICATE);
    //
    //     if (!ReadBytes)
    //     {
    //         //
    //         // Nothing to read
    //         //
    //         free(Buffer);
    //
    //         printf("err, nothing to read\n");
    //         _getch();
    //         return 1;
    //     }
    //
    //     //
    //     // Dispatch the test case number
    //     //
    //
    //     ///  TestCreateLookupTable(PipeHandle, (PVOID)Buffer, ReadBytes);
    //     printf("!!!! Read to run the test cases !!!!");
    //     _getch();
    //
    //     //
    //     // Close the pipe connection
    //     //
    //     NamedPipeClientClosePipe(PipeHandle);
    //
    //     //
    //     // Make sure to exit the test program
    //     //
    //     exit(0);
    // }
    //
    // free(Buffer);
    return 0;
}

////////////////////////////////////////////////////////////////////////////
//                            Server Side                                 //
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
        printf("err, occurred while creating the pipe (%x)\n",
               GetLastError());
        return NULL;
    }
    return hPipe;
}

/**
 * @brief wait for client connection
 *
 * @param PipeHandle
 * @return BOOLEAN
 */
BOOLEAN
NamedPipeServerWaitForClientConnection(HANDLE PipeHandle)
{
    //
    // Wait for the client to connect
    //
    BOOLEAN ClientConnected = ConnectNamedPipe(PipeHandle, NULL);

    if (FALSE == ClientConnected)
    {
        printf("err, occurred while connecting to the client (%x)\n",
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
NamedPipeServerReadClientMessage(HANDLE PipeHandle, CHAR * BufferToSave, INT32 MaximumReadBufferLength)
{
    DWORD BytesTransferred;

    //
    // We are connected to the client.
    // To communicate with the client
    // we will use ReadFile()/WriteFile()
    // on the pipe handle - hPipe
    //

    //
    // Read client message
    //
    BOOLEAN Result = ReadFile(PipeHandle,              // handle to pipe
                              BufferToSave,            // buffer to receive data
                              MaximumReadBufferLength, // size of buffer
                              &BytesTransferred,       // number of bytes read
                              NULL);                   // not overlapped I/O

    if ((!Result) || (0 == BytesTransferred))
    {
        printf("err, occurred while reading from the client (%x)\n",
               GetLastError());
        CloseHandle(PipeHandle);
        return 0;
    }

    //
    // Number of bytes that the client sends to us
    //
    return BytesTransferred;
}

/**
 * @brief Send a message to the client over named pipe
 *
 * @param PipeHandle Handle of the named pipe
 * @param BufferToSend Buffer containing the message to send
 * @param BufferSize Size of the buffer to send
 * @return BOOLEAN TRUE if successful, FALSE otherwise
 */
BOOLEAN
NamedPipeServerSendMessageToClient(HANDLE PipeHandle,
                                   CHAR * BufferToSend,
                                   INT32  BufferSize)
{
    DWORD BytesTransferred;

    //
    // Reply to client
    //
    BOOLEAN Result =
        WriteFile(PipeHandle,        // handle to pipe
                  BufferToSend,      // buffer to write from
                  BufferSize,        // number of bytes to write, include the NULL
                  &BytesTransferred, // number of bytes written
                  NULL);             // not overlapped I/O

    if ((!Result) || (BufferSize != (INT32)BytesTransferred))
    {
        printf("err, occurred while writing to the client (%x)\n",
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
        printf("err, occurred while connecting to the server (%x)\n",
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
 * @brief Send client message over named pipe
 *
 * @param PipeHandle Handle of the named pipe
 * @param BufferToSend Buffer containing the message to send
 * @param BufferSize Size of the buffer to send
 * @return BOOLEAN TRUE if successful, FALSE otherwise
 */
BOOLEAN
NamedPipeClientSendMessage(HANDLE PipeHandle, CHAR * BufferToSend, INT32 BufferSize)
{
    //
    // We are done connecting to the server pipe,
    // we can start communicating with
    // the server using ReadFile()/WriteFile()
    // on handle - hPipe
    //

    DWORD BytesTransferred;

    //
    // Send the message to server
    //
    BOOLEAN Result =
        WriteFile(PipeHandle,        // handle to pipe
                  BufferToSend,      // buffer to write from
                  BufferSize,        // number of bytes to write, include the NULL
                  &BytesTransferred, // number of bytes written
                  NULL);             // not overlapped I/O

    if ((!Result) || (BufferSize != (INT32)BytesTransferred))
    {
        printf("err, occurred while writing to the server (%x)\n",
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

/**
 * @brief Read a message from the server over named pipe
 *
 * @param PipeHandle Handle of the named pipe
 * @param BufferToRead Buffer to store the received message
 * @param MaximumSizeOfBuffer Maximum size of the receive buffer
 * @return UINT32 number of bytes read, or 0 on failure
 */
UINT32
NamedPipeClientReadMessage(HANDLE PipeHandle, CHAR * BufferToRead, INT32 MaximumSizeOfBuffer)
{
    DWORD BytesTransferred;

    //
    // Read server response
    //
    BOOLEAN Result = ReadFile(PipeHandle,          // handle to pipe
                              BufferToRead,        // buffer to receive data
                              MaximumSizeOfBuffer, // size of buffer
                              &BytesTransferred,   // number of bytes read
                              NULL);               // not overlapped I/O

    if ((!Result) || (0 == BytesTransferred))
    {
        printf("err, occurred while reading from the server (%x)\n",
               GetLastError());
        CloseHandle(PipeHandle);
        return 0; // Error
    }

    //
    // Success
    //
    return BytesTransferred;
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
 * @brief An example of how to use named pipe as a server
 *
 * @return INT32
 */
INT32
NamedPipeServerExample()
{
    HANDLE      PipeHandle;
    BOOLEAN     SentMessageResult;
    UINT32      ReadBytes;
    const INT32 BufferSize               = 1024;
    CHAR        BufferToRead[BufferSize] = {0};
    CHAR        BufferToSend[BufferSize] = "test message to send from server !!!";

    printf("create name pipe\n");
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

    printf("success!\n");
    printf("wait for the client connection\n");

    if (!NamedPipeServerWaitForClientConnection(PipeHandle))
    {
        //
        // Error in connection
        //
        return 1;
    }

    printf("client connected\n");
    printf("read client message\n");

    ReadBytes =
        NamedPipeServerReadClientMessage(PipeHandle, BufferToRead, BufferSize);

    if (!ReadBytes)
    {
        //
        // Nothing to read
        //
        return 1;
    }

    printf("Message from client : %s\n", BufferToRead);

    SentMessageResult = NamedPipeServerSendMessageToClient(
        PipeHandle,
        BufferToSend,
        (INT32)strlen(BufferToSend) + 1);

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
 * @brief An example of how to use named pipe as a client
 *
 * @return INT32
 */
INT32
NamedPipeClientExample()
{
    HANDLE      PipeHandle;
    BOOLEAN     SentMessageResult;
    UINT32      ReadBytes;
    const INT32 BufferSize         = 1024;
    CHAR        Buffer[BufferSize] = "test message to send from client !!!";
    PipeHandle                     = NamedPipeClientCreatePipe("\\\\.\\Pipe\\HyperDbgTests");

    if (!PipeHandle)
    {
        //
        // Unable to create handle
        //
        return 1;
    }

    SentMessageResult =
        NamedPipeClientSendMessage(PipeHandle, Buffer, (INT32)strlen(Buffer) + 1);

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

    printf("Server sent the following message: %s\n", Buffer);

    NamedPipeClientClosePipe(PipeHandle);

    return 0;
}
