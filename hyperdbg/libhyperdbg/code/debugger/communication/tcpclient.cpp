/**
 * @file tcpclient.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Server functions over TCP
 * @details
 * @version 0.1
 * @date 2020-08-21
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief communication for client, connecting to the server
 *
 * @param Ip
 * @param Port
 * @param ConnectSocketArg
 * @return INT
 */
INT
CommunicationClientConnectToServer(PCSTR Ip, PCSTR Port, SOCKET * ConnectSocketArg)
{
    WSADATA          wsaData;
    SOCKET           ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL, *ptr = NULL, hints;
    INT              IResult;

    //
    // Initialize Winsock
    //
    IResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (IResult != 0)
    {
        ShowMessages("err, WSAStartup failed (%x)\n", IResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    //
    // Resolve the server address and port
    //
    IResult = getaddrinfo(Ip, Port, &hints, &result);
    if (IResult != 0)
    {
        ShowMessages("getaddrinfo failed (%x)\n", IResult);
        WSACleanup();
        return 1;
    }

    //
    // Attempt to connect to an address until one succeeds
    //
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
    {
        //
        // Create a SOCKET for connecting to server
        //
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET)
        {
            ShowMessages("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        //
        // Connect to server.
        //
        IResult = connect(ConnectSocket, ptr->ai_addr, (INT)ptr->ai_addrlen);
        if (IResult == SOCKET_ERROR)
        {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET)
    {
        ShowMessages("unable to connect to the server\n");
        WSACleanup();
        return 1;
    }

    //
    // Store the arguments
    //
    *ConnectSocketArg = ConnectSocket;

    return 0;
}

/**
 * @brief Send message a client
 *
 * @param ConnectSocket
 * @param sendbuf
 * @param buflen
 * @return INT
 */
INT
CommunicationClientSendMessage(SOCKET ConnectSocket, const CHAR * sendbuf, INT buflen)
{
    INT IResult;

    //
    // Send an initial buffer
    //
    IResult = send(ConnectSocket, sendbuf, buflen, 0);
    if (IResult == SOCKET_ERROR)
    {
        ShowMessages("err, send failed (%x)\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    return 0;
}

/**
 * @brief shutdown the connection as a client
 *
 * @param ConnectSocket
 * @return INT
 */
INT
CommunicationClientShutdownConnection(SOCKET ConnectSocket)
{
    INT IResult;

    //
    // shutdown the connection since no more data will be sent
    //
    IResult = shutdown(ConnectSocket, SD_SEND);
    if (IResult == SOCKET_ERROR)
    {
        //
        // We comment this line because the connection might be removed;
        // thus, we don't need to show error
        //

        /*
    ShowMessages("err, shutdown failed (%x)\n", WSAGetLastError());
    */

        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }
    return 0;
}

/**
 * @brief Receive message as a client
 *
 * @param ConnectSocket
 * @param RecvBuf
 * @param MaxBuffLen
 * @param BuffLenRecvd
 * @return INT
 */
INT
CommunicationClientReceiveMessage(SOCKET ConnectSocket, CHAR * RecvBuf, UINT32 MaxBuffLen, PUINT32 BuffLenRecvd)
{
    INT Result;

    //
    // Receive until the peer closes the connection
    //
    Result = recv(ConnectSocket, RecvBuf, MaxBuffLen, 0);
    if (Result > 0)
    {
        //
        // Set recvd buff len
        //
        *BuffLenRecvd = Result;

        /*
    ShowMessages("bytes received: %d\n", iResult);
        */
    }
    else if (Result == 0)
    {
        //
        // Last packet
        //
    }
    else
    {
        ShowMessages("\nrecv failed with error: %d\n", WSAGetLastError());
        ShowMessages("the remote system closes the connection.\n\n");

        return 1;
    }

    return 0;
}

/**
 * @brief cleanup the connection as client
 *
 * @param ConnectSocket
 * @return INT
 */
INT
CommunicationClientCleanup(SOCKET ConnectSocket)
{
    //
    // cleanup
    //
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}

//
// int __cdecl main(int argc, char **argv) {
//
//   SOCKET ConnectSocket;
//   char sendbuf[DEFAULT_BUFLEN] = "I am sinaei";
//   char recvbuf[DEFAULT_BUFLEN] = {0};
//   UINT32 RecvdBuff = 0;
//
//   //
//   // Connect to server
//   //
//   CommunicationClientConnectToServer("127.0.0.1", DEFAULT_PORT,
//   &ConnectSocket);
//
//   while (true) {
//     //
//     // Send Message
//     //
//     if (CommunicationClientSendMessage(ConnectSocket, sendbuf,
//                                        strlen(sendbuf)) != 0) {
//       //
//       // Failed, break
//       //
//       break;
//     }
//
//     //
//     // Receive final message
//     //
//     if (CommunicationClientReceiveMessage(ConnectSocket, recvbuf,
//                                           DEFAULT_BUFLEN,&RecvdBuff) != 0) {
//       //
//       // Failed, break
//       //
//       break;
//     };
//     ShowMessages("%s\n", recvbuf);
//   }
//
//   //
//   // Shutdown connection
//   //
//   CommunicationClientShutdownConnection(ConnectSocket);
//
//   //
//   // Cleanup
//   //
//   CommunicationClientCleanup(ConnectSocket);
//
//   return 0;
// }
//
