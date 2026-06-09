/**
 * @file tcpcommunication.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Communication over TCP (header)
 * @details
 * @version 0.1
 * @date 2020-08-21
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////
//			Serial Constants             //
//////////////////////////////////////////

#define COM1_PORT 0x03F8
#define COM2_PORT 0x02F8
#define COM3_PORT 0x03E8
#define COM4_PORT 0x02E8

//////////////////////////////////////////
//			   	Server 		            //
//////////////////////////////////////////

int
CommunicationServerCreateServerAndWaitForClient(PCSTR    Port,
                                                SOCKET * ClientSocketArg,
                                                SOCKET * ListenSocketArg);

INT
CommunicationServerReceiveMessage(SOCKET ClientSocket, CHAR * recvbuf, INT recvbuflen);

INT
CommunicationServerSendMessage(SOCKET ClientSocket, const CHAR * sendbuf, INT length);

INT
CommunicationServerShutdownAndCleanupConnection(SOCKET ClientSocket,
                                                SOCKET ListenSocket);

//////////////////////////////////////////
//                Client                //
//////////////////////////////////////////

INT
CommunicationClientConnectToServer(PCSTR Ip, PCSTR Port, SOCKET * ConnectSocketArg);

INT
CommunicationClientSendMessage(SOCKET ConnectSocket, const CHAR * sendbuf, INT buflen);

INT
CommunicationClientShutdownConnection(SOCKET ConnectSocket);

INT
CommunicationClientReceiveMessage(SOCKET ConnectSocket, CHAR * RecvBuf, UINT32 MaxBuffLen, PUINT32 BuffLenRecvd);

INT
CommunicationClientCleanup(SOCKET ConnectSocket);

//////////////////////////////////////////
//     Handle Remote Connection         //
//////////////////////////////////////////

VOID
RemoteConnectionListen(PCSTR Port);

VOID
RemoteConnectionConnect(PCSTR Ip, PCSTR Port);

INT
RemoteConnectionSendCommand(const CHAR * sendbuf, INT len);

INT
RemoteConnectionSendResultsToHost(const CHAR * sendbuf, INT len);

INT
RemoteConnectionCloseTheConnectionWithDebuggee();
