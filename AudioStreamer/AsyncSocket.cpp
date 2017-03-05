/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE:     AsyncSocket.cpp -  This file contains the implementation of listen socket (server side).
--
-- PROGRAM:         AudioPlayer
--
-- FUNCTIONS:       SOCKET createListenSocket(WSADATA * wsadata, int protocol, SOCKADDR_IN * udpaddr = 0);
--
-- DATE:            March 10, 2017
--
-- REVISIONS:
--
-- DESIGNER:		
--
-- PROGRAMMER:		
--
-- NOTES:
----------------------------------------------------------------------------------------------------------------------*/
#include "AsyncSocket.h"
using namespace std;

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    createListenSocket
--
-- DATE:        March 10, 2017
--
-- REVISIONS:   (Date and Description)
--
-- DESIGNER:
--
-- PROGRAMMER:
--
-- INTERFACE:   SOCKET createListenSocket(WSADATA* wsaData, int protocol, SOCKADDR_IN* udpaddr)
--              WSADATA* wsaData: contains information about the Windows Sockets implementation
--              int protocol: TCP or UDP as specified
--              SOCKADDR_IN* udpaddr: points to UDP socket address created
--
-- RETURNS:     returns a listen socket.
--
-- NOTES: 
-- Creates a listen socket on server side.
--
----------------------------------------------------------------------------------------------------------------------*/
SOCKET createListenSocket(WSADATA* wsaData, int protocol, SOCKADDR_IN* udpaddr)
{
    DWORD result;
    SOCKADDR_IN addr;
    SOCKET listenSocket;

    // The highest version of Windows Sockets spec that the caller can use
    WORD wVersionRequested = MAKEWORD( 2, 2 );

    // Open up a Winsock session
    if ((result = WSAStartup(wVersionRequested, wsaData)) != 0)
    {
        cerr << "WSAStartup failed with error %d" << result << endl;
        WSACleanup();
        return NULL;
    }

    // create socket given socket type and protocol
    if ((listenSocket = WSASocket(AF_INET, (protocol == TCP) ? SOCK_STREAM 
        : SOCK_DGRAM , 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
    {
        cerr << "Failed to get a socket with error " << WSAGetLastError() << endl;
        return NULL;
    }

    // set server socket structure
    addr.sin_family         = AF_INET;
    addr.sin_addr.s_addr    = htonl(INADDR_ANY);
    addr.sin_port           = htons((protocol == TCP) ? TCPPORT : UDPPORT);

    // bind to the listen socket
    if (bind(listenSocket, (PSOCKADDR) &addr, sizeof(addr)) == SOCKET_ERROR)
    {
        cerr << "bind() failed with error " << WSAGetLastError() << endl;
        return NULL;
    }

    cout << "Server bound to port " << ((protocol == TCP) ? TCPPORT : UDPPORT) << endl;

    // listen to the connection requests, allowing incoming connections queue up
    if (protocol != UDP)
    {
        if (listen(listenSocket, BACKLOG))
        {
            cerr << "listen() failed with error " << WSAGetLastError() << endl;
            closesocket(listenSocket);
            return NULL;
        }
    }
    else
    {
        *udpaddr = addr;
    }
    
    return listenSocket;
}