/*------------------------------------------------------------------------------------------------------------------
-- HEADER FILE: common.h
--
-- DATE:        March 12, 2017
--
-- DESIGNER:
--
-- PROGRAMMER:
--
-- NOTES:
-- This header file includes the common macro definitions and function declarations for the client.
--------------------------------------------------------------------------------------------------------------------*/
#ifndef COMMON_H
#define COMMON_H

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"libzplay.lib")
#pragma warning (disable: 4996)

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <string>
#include <cstring>
#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <vector>

#define SCREEN_W        800         // screen width
#define SCREEN_H        480         // screen height
#define TCPPORT         7000        // tcp port#
#define UDPPORT         7001        // udp port#

#define ADDRSIZE        16          // ip address size
#define TMPBUFSIZE      1024        // temporary buffer size
#define DATABUFSIZE     51200       // datagram buffer size
#define STREAMBUFSIZE   102400      // stream buffer size
#define MULTICAST_ADDR  "234.1.1.5" // multicast ip address
#define MULTICAST_PORT  8910        // multicast port#
#define MULTICAST_TTL   2           // Time-To-Live (TTL)
#define BACKLOG         5           // # of simultaneous requests allowed in queue
#define SLEEPSPAN       100         // sleep timeout

// packet command macros
#define EOT     0x004

enum {
    TCP,
    UDP
};

// server state
enum ServerState {
    IDLE,           // initial state
	LIST,           // fetch play list
	STREAMING,      // streaming
	DOWNLOADING,    // downloading
	UPLOADING,      // uploading
	MICCHATTING,    // microphone chat
	MULTICASTING,   // multicasting
	STATEERR        // error
};

// request type
enum RequestType {
    REQIDLE,        // idle
    REQLIST,        // fetch playlist
    REQSTREAM,      // streaming request
    REQDOWNLOAD,    // download request
    REQUPLOAD,      // upload request
    REQMICCHAT,     // microphone chat request
    REQMULTICAST    // multicast request
};

#endif
