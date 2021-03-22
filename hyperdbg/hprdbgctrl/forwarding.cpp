/**
 * @file forwarding.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Event source forwarding
 * @details
 * @version 0.1
 * @date 2020-11-16
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern UINT64     g_OutputSourceTag;
extern LIST_ENTRY g_OutputSources;

/**
 * @brief Get the output source tag and increase the
 * global variable for tag
 *
 * @return UINT64
 */
UINT64
ForwardingGetNewOutputSourceTag()
{
    return g_OutputSourceTag++;
}

/**
 * @brief Opens the output source
 * @param SourceDescriptor Descriptor of the source
 *
 * @return DEBUGGER_OUTPUT_SOURCE_STATUS return status of the opening function
 */
DEBUGGER_OUTPUT_SOURCE_STATUS
ForwardingOpenOutputSource(PDEBUGGER_EVENT_FORWARDING SourceDescriptor)
{
    //
    // Check if already closed
    //
    if (SourceDescriptor->State == EVENT_FORWARDING_CLOSED)
    {
        return DEBUGGER_OUTPUT_SOURCE_STATUS_ALREADY_CLOSED;
    }

    //
    // check if already opened
    //
    if (SourceDescriptor->State == EVENT_FORWARDING_STATE_OPENED)
    {
        return DEBUGGER_OUTPUT_SOURCE_STATUS_ALREADY_OPENED;
    }

    //
    // Set the status to opened
    //
    SourceDescriptor->State = EVENT_FORWARDING_STATE_OPENED;

    //
    // Now, it's time to open the source based on its type
    //
    if (SourceDescriptor->Type == EVENT_FORWARDING_FILE)
    {
        //
        // Nothing special to do here, file is opened with CreateFile
        // and nothing should be called to open the handle
        //
        return DEBUGGER_OUTPUT_SOURCE_STATUS_SUCCESSFULLY_OPENED;
    }
    else if (SourceDescriptor->Type == EVENT_FORWARDING_NAMEDPIPE)
    {
        //
        // Nothing special to do here, namedpipe is opened with CreateFile
        // and nothing should be called to open the handle
        //
        return DEBUGGER_OUTPUT_SOURCE_STATUS_SUCCESSFULLY_OPENED;
    }
    else if (SourceDescriptor->Type == EVENT_FORWARDING_TCP)
    {
        //
        // Nothing special to do here, tcp socket is opened with
        // CommunicationClientConnectToServer and nothing should be
        // called to open the socket
        //
        return DEBUGGER_OUTPUT_SOURCE_STATUS_SUCCESSFULLY_OPENED;
    }

    return DEBUGGER_OUTPUT_SOURCE_STATUS_UNKNOWN_ERROR;
}

/**
 * @brief Closes the output source
 * @param SourceDescriptor Descriptor of the source
 *
 * @return DEBUGGER_OUTPUT_SOURCE_STATUS return status of the closing function
 */
DEBUGGER_OUTPUT_SOURCE_STATUS
ForwardingCloseOutputSource(PDEBUGGER_EVENT_FORWARDING SourceDescriptor)
{
    //
    // Check if already closed
    //
    if (SourceDescriptor->State == EVENT_FORWARDING_CLOSED)
    {
        return DEBUGGER_OUTPUT_SOURCE_STATUS_ALREADY_CLOSED;
    }

    //
    // Check if not opened
    //
    if (SourceDescriptor->State == EVENT_FORWARDING_STATE_NOT_OPENED ||
        SourceDescriptor->State != EVENT_FORWARDING_STATE_OPENED)
    {
        //
        // Not opennd ? or state other than opened ?
        //
        return DEBUGGER_OUTPUT_SOURCE_STATUS_UNKNOWN_ERROR;
    }

    //
    // Set the state
    //
    SourceDescriptor->State = EVENT_FORWARDING_CLOSED;

    //
    // Now, it's time to close the source based on its type
    //
    if (SourceDescriptor->Type == EVENT_FORWARDING_FILE)
    {
        //
        // Close the hanlde
        //
        CloseHandle(SourceDescriptor->Handle);

        //
        // Return the status
        //
        return DEBUGGER_OUTPUT_SOURCE_STATUS_SUCCESSFULLY_CLOSED;
    }
    else if (SourceDescriptor->Type == EVENT_FORWARDING_TCP)
    {
        //
        // Shutdown connection
        //
        CommunicationClientShutdownConnection(SourceDescriptor->Socket);

        //
        // Cleanup
        //
        CommunicationClientCleanup(SourceDescriptor->Socket);

        //
        // Return the status
        //
        return DEBUGGER_OUTPUT_SOURCE_STATUS_SUCCESSFULLY_CLOSED;
    }
    else if (SourceDescriptor->Type == EVENT_FORWARDING_NAMEDPIPE)
    {
        //
        // Close the file
        //
        NamedPipeClientClosePipe(SourceDescriptor->Handle);

        //
        // Return the status
        //
        return DEBUGGER_OUTPUT_SOURCE_STATUS_SUCCESSFULLY_CLOSED;
    }

    return DEBUGGER_OUTPUT_SOURCE_STATUS_UNKNOWN_ERROR;
}

/**
 * @brief Create a new source (create handle from the source)
 * @param SourceType Type of the source
 * @param Description Description of the source
 * @param Socket Socket object in the case of TCP connection
 *
 * @details If the target connection is a tcp connection then there
 * is no handle and instead there is a socket, this way we pass a
 * valid value for handle (TRUE) which is not a valid handle but it
 * indicates that the operation was successful and the caller can use
 * the pointer that it passed as the socket.
 * On anything other than tcp sockets, the socket pointer in not
 * modified; thus, it's not value
 *
 * @return HANDLE returns handle of the source
 */
HANDLE
ForwardingCreateOutputSource(DEBUGGER_EVENT_FORWARDING_TYPE SourceType,
                             string                         Description,
                             SOCKET *                       Socket)
{
    string IpPortDelimiter;
    string Ip;
    string Port;

    if (SourceType == EVENT_FORWARDING_FILE)
    {
        //
        // Create a new file
        //
        HANDLE FileHandle = CreateFileA(Description.c_str(), GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        //
        // The handle might be INVALID_HANDLE_VALUE which will be
        // checked by the caller
        //
        return FileHandle;
    }
    else if (SourceType == EVENT_FORWARDING_NAMEDPIPE)
    {
        HANDLE PipeHandle = NamedPipeClientCreatePipe(Description.c_str());

        if (!PipeHandle)
        {
            //
            // Unable to create handle
            //
            return INVALID_HANDLE_VALUE;
        }

        return PipeHandle;
    }
    else if (SourceType == EVENT_FORWARDING_TCP)
    {
        //
        // Check if the port is included in the description string or not
        //
        if (Description.find(":") != std::string::npos)
        {
            //
            // Split the ip and port by : delimiter
            //
            IpPortDelimiter = ":";
            Ip              = Description.substr(0, Description.find(IpPortDelimiter));
            Port            = Description.substr(Description.find(IpPortDelimiter) + 1,
                                      Description.find(IpPortDelimiter) +
                                          Description.size());

            //
            // Connect to server, in this function 0 means there was no error
            // and 1 means there was an error
            //
            if (CommunicationClientConnectToServer(Ip.c_str(), Port.c_str(), Socket) == 0)
            {
                //
                // Send a fake handle just to avoid sending INVALID_HANDLE_VALUE
                // because this functionality doesn't work with handlers; however,
                // send 1 or TRUE is a valid handle
                //
                return (HANDLE)TRUE;
            }
            else
            {
                //
                // There was an error in connecting to the server
                // so return an invalid handle
                //
                return INVALID_HANDLE_VALUE;
            }
        }
        else
        {
            //
            // Invalid address format
            //
            return INVALID_HANDLE_VALUE;
        }
    }

    //
    // By default, handle is invalid
    //
    return INVALID_HANDLE_VALUE;
}

/**
 * @brief Send the event result to the corresponding sources
 * @param EventDetail Description saved about the event in the
 * user-mode
 * @param MessageLength Length of the message
 * @details This function will not check whether the event has an
 * output source or not, the caller if this function should make
 * sure that the following event has valid output sources or not
 *
 * @return BOOLEAN whether sending results was successful or not
 */
BOOLEAN
ForwardingPerformEventForwarding(PDEBUGGER_GENERAL_EVENT_DETAIL EventDetail,
                                 CHAR *                         Message,
                                 UINT32                         MessageLength)
{
    BOOLEAN     Result   = FALSE;
    PLIST_ENTRY TempList = 0;

    for (size_t i = 0; i < DebuggerOutputSourceMaximumRemoteSourceForSingleEvent;
         i++)
    {
        //
        // Check whether we reached to the end of the events
        //
        if (EventDetail->OutputSourceTags[i] == NULL)
        {
            return Result;
        }

        //
        // If we reach here then the output tag is not null
        // means that we should find the event tag from list
        // of tags
        //
        TempList = &g_OutputSources;

        while (&g_OutputSources != TempList->Flink)
        {
            TempList = TempList->Flink;

            PDEBUGGER_EVENT_FORWARDING CurrentOutputSourceDetails = CONTAINING_RECORD(
                TempList,
                DEBUGGER_EVENT_FORWARDING,
                OutputSourcesList);

            if (EventDetail->OutputSourceTags[i] ==
                CurrentOutputSourceDetails->OutputUniqueTag)
            {
                //
                // Indicate that we found a tag that matches this item
                // Now, we should check whether the output is opened or
                // not closed
                //
                if (CurrentOutputSourceDetails->State ==
                    EVENT_FORWARDING_STATE_OPENED)
                {
                    switch (CurrentOutputSourceDetails->Type)
                    {
                    case EVENT_FORWARDING_NAMEDPIPE:
                        Result = ForwardingSendToNamedPipe(
                            CurrentOutputSourceDetails->Handle,
                            Message,
                            MessageLength);
                        break;
                    case EVENT_FORWARDING_FILE:
                        Result = ForwardingWriteToFile(CurrentOutputSourceDetails->Handle,
                                                       Message,
                                                       MessageLength);
                        break;
                    case EVENT_FORWARDING_TCP:
                        Result = ForwardingSendToTcpSocket(
                            CurrentOutputSourceDetails->Socket,
                            Message,
                            MessageLength);
                        break;
                    default:
                        break;
                    }
                }

                //
                // No need to search through the list anymore
                //
                break;
            }
        }
    }

    return FALSE;
}

/**
 * @brief Write the output results to the file
 * @param FileHandle Handle of the target file
 * @param Message The message that should be written to file
 * @param MessageLength Length of the message
 * @details This function will not check whether the event has an
 * output source or not, the caller if this function should make
 * sure that the following event has valid output sources or not
 *
 * @return BOOLEAN whether the writing to the file was successful or not
 */
BOOLEAN
ForwardingWriteToFile(HANDLE FileHandle, CHAR * Message, UINT32 MessageLength)
{
    DWORD BytesWritten = 0;
    BOOL  ErrorFlag    = FALSE;

    ErrorFlag = WriteFile(FileHandle,    // open file handle
                          Message,       // start of data to write
                          MessageLength, // number of bytes to write
                          &BytesWritten, // number of bytes that were written
                          NULL);         // no overlapped structure

    if (ErrorFlag == FALSE)
    {
        //
        // Err, terminal failure: Unable to write to file
        //

        return FALSE;
    }
    else
    {
        if (BytesWritten != MessageLength)
        {
            //
            // This is an error because a synchronous write that results in
            // success (WriteFile returns TRUE) should write all data as
            // requested. This would not necessarily be the case for
            // asynchronous writes.
            //

            return FALSE;
        }
        else
        {
            //
            // Successfully wrote
            //
            return TRUE;
        }
    }

    //
    // by default we assume there was an error
    //
    return FALSE;
}

/**
 * @brief Send the output results to the namedpipe
 * @param NamedPipeHandle Handle of the target namedpipe
 * @param Message The message that should be sent to namedpipe
 * @param MessageLength Length of the message
 * @details This function will not check whether the event has an
 * output source or not, the caller if this function should make
 * sure that the following event has valid output sources or not
 *
 * @return BOOLEAN whether the sending to the namedpipe was successful or not
 */
BOOLEAN
ForwardingSendToNamedPipe(HANDLE NamedPipeHandle, CHAR * Message, UINT32 MessageLength)
{
    BOOLEAN SentMessageResult;

    SentMessageResult =
        NamedPipeClientSendMessage(NamedPipeHandle, Message, MessageLength);

    if (!SentMessageResult)
    {
        //
        // Sending error
        //
        return FALSE;
    }

    //
    // Successfully sent
    //
    return TRUE;
}

/**
 * @brief Send the output results to the tcp socket
 * @param TcpSocket Socket object of the target tcp socket
 * @param Message The message that should be sent to the tcp socket
 * @param MessageLength Length of the message
 * @details This function will not check whether the event has an
 * output source or not, the caller if this function should make
 * sure that the following event has valid output sources or not
 *
 * @return BOOLEAN whether the sending to the tcp socket was successful or not
 */
BOOLEAN
ForwardingSendToTcpSocket(SOCKET TcpSocket, CHAR * Message, UINT32 MessageLength)
{
    if (CommunicationClientSendMessage(TcpSocket, Message, MessageLength) != 0)
    {
        //
        // Failed to send
        //
        return FALSE;
    }

    //
    // Successfully sent
    //
    return TRUE;
}
