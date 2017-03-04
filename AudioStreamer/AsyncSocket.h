/*------------------------------------------------------------------------------------------------------------------
-- HEADER FILE: AsyncSocket.h
--
-- DATE:        March 12, 2017
--
-- DESIGNER:
--
-- PROGRAMMER:
--
-- NOTES:
-- This header file includes the required function prototypes for asynchronous socket implementation.
--------------------------------------------------------------------------------------------------------------------*/
#ifndef ASYNCSOCKET_H
#define ASYNCSOCKET_H

#include "common.h"

// function prototypes
SOCKET createListenSocket(WSADATA * wsadata, int protocol, SOCKADDR_IN * udpaddr = 0);

#endif
