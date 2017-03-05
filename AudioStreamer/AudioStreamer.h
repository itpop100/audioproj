/*------------------------------------------------------------------------------------------------------------------
-- HEADER FILE: AudioStreamer.h
--
-- DATE:        March 12, 2017
--
-- DESIGNER:
--
-- PROGRAMMER:
--
-- NOTES:
-- This header file includes the required macro definitions and function declarations for server side implementation
-- (AudioStreamer.cpp).
--------------------------------------------------------------------------------------------------------------------*/
#ifndef AUDIOSTREAMER_H
#define AUDIOSTREAMER_H

#include <string>
#include <vector>
#include "Microphone.h"

// multicast socket struct
typedef struct {
    std::ifstream* file;
    SOCKET socket;
    SOCKADDR_IN mcaddr;
    int filesize;
} MCSOCKET, *LPMCSOCKET;

// function prototypes
std::string getAudioPath();
std::string getCommand(const int);
DWORD WINAPI listenThread(LPVOID);
DWORD WINAPI mcThread(LPVOID);
DWORD WINAPI listenRequest(LPVOID);
ServerState decodeRequest(char*, std::string&, DWORD&);
void setCursor();
void handleRequest(const ServerState& currentState, SOCKET clntSocket, std::string fileName = "", 
                   DWORD fileSize = 0);
int populatePlayList(std::vector<std::string>& list);
int  __stdcall  mcCallbackFunc(void* instance, void* user_data, libZPlay::TCallbackMessage message, 
                               unsigned int param1, unsigned int param2);

#endif
