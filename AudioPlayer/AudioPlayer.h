/*------------------------------------------------------------------------------------------------------------------
-- HEADER FILE: AudioPlayer.h
--
-- DATE:        March 10, 2017
--
-- DESIGNER:
--
-- PROGRAMMER:
--
-- NOTES:
-- This header file includes the definition of AudioPlayer class.
--------------------------------------------------------------------------------------------------------------------*/
#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include "../Common/common.h"
#include "../Common/libzplay.h"
#include "resource.h"

class AudioPlayer {

public:
    // constructor
    AudioPlayer()
    { 
        currentState = NOTCONNECTED;        // current state. Initial state: NOTCONNECTED
        connectSocket_ = 0;                 // connect socket
        player_ = libZPlay::CreateZPlay();  // libZPlay player instance
    }

    // destructor
    ~AudioPlayer()
    {
        player_->Release();
        shutdown(connectSocket_, SD_BOTH);
        closesocket(connectSocket_);
        WSACleanup();
    }

    bool runClient(WSADATA *wsadata, const char*, const int);
    DWORD stThread(LPVOID);
    DWORD dlThread(LPVOID);
    DWORD ulThread(LPVOID);
    static DWORD WINAPI runSTThread(LPVOID);
    static DWORD WINAPI runDLThread(LPVOID);
    static DWORD WINAPI runULThread(LPVOID);
    void dispatchSend(std::string);
    void dispatchRecv();

    long int dlFileSize, ulFileSize;        // size of download/upload file
    int currentState;                       // current state
    int downloadedAmount, uploadedAmount;   // bytes downloaded/uploaded
    std::string cachedPlayList;             // cached play list
    std::vector<std::string> localPlayList; // local play list
    std::string currentAudioFile;           // current audio file
    std::ofstream downloadFileStream;       // download file stream
    std::ifstream uploadFileStream;         // upload file stream
    libZPlay::ZPlay *player_;               // ZPlay instance
    SOCKET connectSocket_;                  // connect socket
    SOCKADDR_IN addr_;                      // socket address
    hostent *hp_;                           // hostent struct
    DWORD dlThreadId, stThreadId, ulThreadId, listThreadId; // thread IDs
    HANDLE dlThreadHandle, stThreadHandle, ulThreadHandle, listThreadHandle; // thread handles

private:
    SOCKET createTCPClient(WSADATA*, const char*, const int);
    LPSOCKETDATA allocData(SOCKET fd);
    void freeData(LPSOCKETDATA data);
    bool dispatchWSASendRequest(LPSOCKETDATA data);
    bool dispatchWSARecvRequest(LPSOCKETDATA data);
    void recvComplete (DWORD Error, DWORD bytesTransferred, LPWSAOVERLAPPED overlapped, DWORD flags);
    void sendComplete (DWORD Error, DWORD bytesTransferred, LPWSAOVERLAPPED overlapped, DWORD flags);
    static void CALLBACK runRecvComplete (DWORD Error, DWORD bytesTransferred, LPWSAOVERLAPPED overlapped, DWORD flags);
    static void CALLBACK runSendComplete (DWORD Error, DWORD bytesTransferred, LPWSAOVERLAPPED overlapped, DWORD flags);
};

#endif
