/*------------------------------------------------------------------------------------------------------------------
-- HEADER FILE: AsyncSocket.h
--
-- DATE:        March 12, 2017
--
-- DESIGNER:    Fred Yang, John Agapeyev, Isaac Morneau, Maitiu Morton
--
-- PROGRAMMER:  Fred Yang, John Agapeyev, Isaac Morneau, Maitiu Morton
--
-- NOTES:
-- This header file includes the required function prototypes for asynchronous socket implementation.
------------------------------------------------------------------------------------------------------------------ */
#ifndef ASYNCSOCKET_H
#define ASYNCSOCKET_H

#include "../Common/common.h"

// function prototypes
SOCKET createListenSocket(WSADATA * wsadata, int protocol, SOCKADDR_IN * udpaddr = 0);

#endif
