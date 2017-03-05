/*------------------------------------------------------------------------------------------------------------------
-- HEADER FILE: Main.h
--
-- DATE:        March 10, 2017
--
-- DESIGNER:    
--
-- PROGRAMMER:  
--
-- NOTES:
-- This header file includes the required definitions and function prototypes for client GUI
-- implementation (Main.cpp).
--------------------------------------------------------------------------------------------------------------------*/
#ifndef MAIN_H
#define MAIN_H

#ifdef _MSC_VER
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' \
version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#include "AudioPlayer.h"
#pragma comment(lib,"comctl32.lib")

// defaults
char szServer[128] = "localhost"; // hostname
char szPort[5] = "7000"; // port#
char szTitle[30] = "Comm Audio Player v1.0";
char szWarn[128] = "Wait";

HWND hPlayList;     // handle to play listbox
HWND hTraceList;    // handle to trace listbox
SOCKET mcSocket = NULL;  // multicast socket
SOCKET micSocket = NULL; // microphone socket

// microphone server/client addresses
SOCKADDR_IN micServer, micClient;

libZPlay::ZPlay* micPlayer;    // microphone player
libZPlay::ZPlay* streamPlayer; // stream player

HBRUSH hbrBackground = NULL;   // handle to background brush
std::string curItem; // current item selected
bool connected; // connect state

// function prototypes
bool renderUI(HWND hWnd);
bool downloadRequest(AudioPlayer&);
bool uploadRequest(AudioPlayer& clnt, HWND hWnd, OPENFILENAME &ofn);
bool streamRequest(AudioPlayer&);
bool micRequest(AudioPlayer&);
bool mcRequest(AudioPlayer&);
bool listRequest(AudioPlayer&, HWND*) ;
bool createMicSocket();
bool startMicChat();
bool populatePlayList(HWND, std::vector<std::string>);
void openFileDialog(HWND, OPENFILENAME&);
void pauseAudio(AudioPlayer&);
void stopAudio(AudioPlayer&);
void playAudio(HWND, AudioPlayer&);
void closeAudio(HWND, AudioPlayer&);
std::vector<std::string> processPlayList(const std::string&);
std::string getSelectedListBoxItem(HWND*, int);
std::string getFileName(std::string);
SOCKET createMCSocket();
DWORD WINAPI mcThread(LPVOID args);
DWORD WINAPI listThread(LPVOID args);
DWORD WINAPI micThread(LPVOID param);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
int __stdcall micCallBackFunc(void* instance, void* user_data, libZPlay::TCallbackMessage message, 
                              unsigned int param1, unsigned int param2);

#endif