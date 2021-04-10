/**
 * @file tcpclient.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
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
 * @return int 
 */
int
CommunicationClientConnectToServer(PCSTR Ip, PCSTR Port, SOCKET * ConnectSocketArg)
{
    WSADATA          wsaData;
    SOCKET           ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL, *ptr = NULL, hints;
    int              iResult;

    //
    // Initialize Winsock
    //
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        ShowMessages("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    //
    // Resolve the server address and port
    //
    iResult = getaddrinfo(Ip, Port, &hints, &result);
    if (iResult != 0)
    {
        ShowMessages("getaddrinfo failed with error: %d\n", iResult);
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
        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR)
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
        ShowMessages("Unable to connect to server!\n");
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
 * @return int 
 */
int
CommunicationClientSendMessage(SOCKET ConnectSocket, const char * sendbuf, int buflen)
{
    int iResult;

    //
    // Send an initial buffer
    //
    iResult = send(ConnectSocket, sendbuf, buflen, 0);
    if (iResult == SOCKET_ERROR)
    {
        ShowMessages("send failed with error: %d\n", WSAGetLastError());
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
 * @return int 
 */
int
CommunicationClientShutdownConnection(SOCKET ConnectSocket)
{
    int iResult;

    //
    // shutdown the connection since no more data will be sent
    //
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR)
    {
        //
        // We comment this line because the connection might be removed;
        // thus, we don't need to show error
        //

        /*
    ShowMessages("shutdown failed with error: %d\n", WSAGetLastError());
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
 * @param recvbuf 
 * @param recvbuflen 
 * @return int 
 */
int
CommunicationClientReceiveMessage(SOCKET ConnectSocket, char * recvbuf, int recvbuflen)
{
    int iResult;

    //
    // Receive until the peer closes the connection
    //
    iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
    if (iResult > 0)
    {
        /*
    ShowMessages("Bytes received: %d\n", iResult);
    */
    }
    else if (iResult == 0)
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
 * @return int 
 */
int
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
//                                           DEFAULT_BUFLEN) != 0) {
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
