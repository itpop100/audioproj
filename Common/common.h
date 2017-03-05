/*------------------------------------------------------------------------------------------------------------------
-- HEADER FILE: common.h
--
-- DATE:        March 10, 2017
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
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <Commctrl.h>
#include <chrono>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <vector>

#define SCREEN_W        800         // screen width
#define SCREEN_H        570         // screen height
#define TCPPORT         7000        // tcp port#
#define UDPPORT         7001        // udp port#
#define ADDRSIZE        16          // ip address size
#define DATABUFSIZE     4096        // data buffer size
#define MAXBUFSIZE      102400      // max stream/datagram buffer size

#define MULTICAST_ADDR  "234.1.1.5" // multicast ip address
#define MULTICAST_PORT  8910        // multicast port#
#define MAXITEMS        15          // maximus items allowed in the trace listbox
#define SLEEPSPAN       100         // sleep timeout
#define BACKLOG         5           // maximum simultaneous connections allowed in queue
#define MULTICAST_TTL   2           // TTL up to 2 routes to pass through

#define SAMPLERATE      44100       // 1x 44.1K audio CD sampling rate
#define CHANNELNUM      1           // channel 2 for stereo interleaved
#define BITPERSAMPLE    16          // 16 bit per sample

// status bar macros
#define STATUSBAR_MODE      0
#define STATUSBAR_STATUS    1
#define STATUSBAR_PROTO     2
#define STATUSBAR_STATS     3
#define STATUSBAR_TIME      4
#define STATUSBAR_PROG      5

// packet command macros
#define EOT     0x004

extern HWND ghWnd;
class AudioPlayer;

enum {
    TCP,
    UDP
};

// big endian
enum BIGEND {
    LITTLEEND,
    BIGEND
};

// client state
enum ClientState {
    NOTCONNECTED,   // not connected
    WAITFORCOMMAND, // wait for command
    SENTDLREQUEST,  // download request sent
    WAITFORDOWNLOAD,// wait for download (approval from server)
    DOWNLOADING,    // downloading
    SENTULREQUEST,  // upload request sent
    WAITFORUPLOAD,  // wait for upload (approval from server)
    UPLOADING,      // uploading
    STREAMING,      // streaming data from server
    LISTENMULTICAST,// listen to multicast channel
    MICROPHONE,     // microphone mode
    SENTSTREQUEST,  // streaming data request sent
    WAITFORSTREAM,  // wait for streaming (approval from server)
    SENTLISTREQUEST,// getting play list request sent
    WAITFORLIST     // wait for fetching play list
};

// server state
enum ServerState {
    STATEIDLE,           // initial state
    STATELIST,           // fetch play list
    STATESTREAMING,      // streaming
    STATEDOWNLOADING,    // downloading
    STATEUPLOADING,      // uploading
    STATEMICCHATTING,    // microphone chat
    STATEMULTICASTING,   // multicasting
    STATEERR             // error
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

// socket data struct
typedef struct socket_data{
    SOCKET sock;
    WSABUF	wsabuf;
    char databuf[MAXBUFSIZE];
    WSAOVERLAPPED overlap;
}SOCKETDATA, *LPSOCKETDATA;

// request context struct
typedef struct request_context{
    LPSOCKETDATA data;
    AudioPlayer* clnt;
} REQUESTCONTEXT;

// list context struct
typedef struct list_context {
    AudioPlayer* clnt;
    HWND* hwnd;
} LISTCONTEXT;

// upload context struct
typedef struct upload_context {
    AudioPlayer* clnt;
    std::string userReq;
} UPLOADCONTEXT;

#endif 
