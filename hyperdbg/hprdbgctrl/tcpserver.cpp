/**
 * @file tcpserver.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Server functions over TCP
 * @details
 * @version 0.1
 * @date 2020-08-21
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#undef UNICODE
#define WIN32_LEAN_AND_MEAN
#include "pch.h"

//
// Disable it because we want to use inet_ntoa
//
#pragma warning(disable : 4996)

/**
 * @brief Create server and wait for a client to connect
 * @details this function only accepts one client not multiple clients
 * 
 * @param Port 
 * @param ClientSocketArg 
 * @param ListenSocketArg 
 * @return int 
 */
int
CommunicationServerCreateServerAndWaitForClient(PCSTR    Port,
                                                SOCKET * ClientSocketArg,
                                                SOCKET * ListenSocketArg)
{
    WSADATA wsaData;
    int     iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo * result = NULL;
    struct addrinfo   hints;

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
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags    = AI_PASSIVE;

    //
    // Resolve the server address and port
    //
    iResult = getaddrinfo(NULL, Port, &hints, &result);
    if (iResult != 0)
    {
        ShowMessages("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    //
    // Create a SOCKET for connecting to server
    //
    ListenSocket =
        socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET)
    {
        ShowMessages("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    //
    // Setup the TCP listening socket
    //
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        ShowMessages("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
        ShowMessages("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    //
    // Accept a client socket
    //
    sockaddr_in name    = {0};
    int         addrlen = sizeof(name);

    ClientSocket = accept(ListenSocket, (struct sockaddr *)&name, &addrlen);

    if (ClientSocket == INVALID_SOCKET)
    {
        ShowMessages("accept failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    //
    // Show that we connected to a client
    //
    ShowMessages("connected to : %s:%d\n", inet_ntoa(name.sin_addr), ntohs(name.sin_port));

    //
    // Set the argument
    //
    *ClientSocketArg = ClientSocket;
    *ListenSocketArg = ListenSocket;

    return 0;
}

/**
 * @brief listen and receive message as the server
 * 
 * @param ClientSocket 
 * @param recvbuf 
 * @param recvbuflen 
 * @return int 
 */
int
CommunicationServerReceiveMessage(SOCKET ClientSocket, char * recvbuf, int recvbuflen)
{
    int iResult;

    //
    // Receive until the peer shuts down the connection
    //
    iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
    if (iResult > 0)
    {
        //
        // ShowMessages("Bytes received: %d\n", iResult);
        //
    }
    else if (iResult == 0)
    {
        //
        // ShowMessages("Connection closing...\n");
        //
    }
    else
    {
        ShowMessages("recv failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();

        return 1;
    }

    return 0;
}

/**
 * @brief send message as the server
 * 
 * @param ClientSocket 
 * @param sendbuf 
 * @param length 
 * @return int 
 */
int
CommunicationServerSendMessage(SOCKET ClientSocket, const char * sendbuf, int length)
{
    int iSendResult;

    //
    // Echo the buffer back to the sender
    //
    iSendResult = send(ClientSocket, sendbuf, length, 0);
    if (iSendResult == SOCKET_ERROR)
    {
        /*
    ShowMessages("send failed with error: %d\n", WSAGetLastError());
    closesocket(ClientSocket);
    WSACleanup();
    */
        return 1;
    }
    return 0;
}

/**
 * @brief Shutdown and cleanup connection as server
 * 
 * @param ClientSocket 
 * @param ListenSocket 
 * @return int 
 */
int
CommunicationServerShutdownAndCleanupConnection(SOCKET ClientSocket,
                                                SOCKET ListenSocket)
{
    int iResult;

    //
    // No longer need server socket
    //
    closesocket(ListenSocket);

    //
    // shutdown the connection since we're done
    //
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR)
    {
        //
        // We comment this line because the connection might be removed;
        // thus, we don't need to show error
        //

        /*
    ShowMessages("shutdown failed with error: %d\n", WSAGetLastError());
    */

        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    //
    // cleanup
    //
    closesocket(ClientSocket);
    WSACleanup();

    return 0;
}

//
// int __cdecl main(void) {
//
//   SOCKET ClientSocket;
//   SOCKET ListenSocket;
//
//   char recvbuf[DEFAULT_BUFLEN] = {0};
//   char sendbuf[DEFAULT_BUFLEN] = "hello, I love taylor";
//
//   //
//   // Start server and wait for client
//   //
//   CommunicationServerCreateServerAndWaitForClient(DEFAULT_PORT,
//   &ClientSocket,
//   &ListenSocket);
//
//   while (true) {
//
//     //
//     // Recieve message
//     //
//     if (CommunicationServerReceiveMessage(ClientSocket, recvbuf,
//                                           DEFAULT_BUFLEN) != 0) {
//       //
//       // Failed, break
//       //
//       break;
//     }
//
//     ShowMessages("%s\n", recvbuf);
//
//     //
//     // Send the message
//     //
//     if (CommunicationServerSendMessage(ClientSocket, sendbuf,
//                                        strlen(sendbuf)) != 0) {
//       //
//       // Failed, break
//       //
//       break;
//     }
//   }
//
//   ShowMessages("close conntection\n");
//
//   //
//   // Close the connection
//   //
//   CommunicationServerShutdownAndCleanupConnection(ClientSocket,
//   ListenSocket);
//
//   return 0;
// }
//
