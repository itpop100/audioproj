#include "Microphone.h"

using namespace std;
using namespace libZPlay;

// stream player
ZPlay* streamPlayer;

// microphone player
ZPlay* micPlayer;

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    startMicChat
--
-- DATE:        March 8, 2017
--
-- REVISIONS:	
--
-- DESIGNER:	
--
-- PROGRAMMER:	
--
-- INTERFACE:   int startMicChat()
--
-- RETURNS:     TRUE on success, FALSE on failure
--
-- NOTES:
-- This function will start microphone chat session.
----------------------------------------------------------------------------------------------------------------------*/
bool startMicChat()
{
    WSADATA wsadata;
    LPMICSOCKET udata = (LPMICSOCKET) malloc(sizeof(MICSOCKET));
    udata->micsocket = createListenSocket(&wsadata, UDP, &udata->micaddr);

    // streaming player to sample/ the raw audio
    streamPlayer = CreateZPlay();
    streamPlayer->SetSettings(sidSamplerate, SAMPLERATE);
    streamPlayer->SetSettings(sidChannelNumber, CHANNELNUM);
    streamPlayer->SetSettings(sidBitPerSample, BITPERSAMPLE);
    streamPlayer->SetSettings(sidBigEndian, LITTLEEND);

    int streamBlock;    // memory block with stream data
    if (streamPlayer->OpenStream(1, 1, &streamBlock, 1, sfPCM) == 0)
    {
        cerr << "Error opening a microphone stream: " << streamPlayer->GetError() << endl;
        streamPlayer->Release();
        closesocket(udata->micsocket);
        return FALSE;
    }

    // microphone player to open microphone device & collect sounds
    micPlayer = CreateZPlay();
    if (micPlayer->OpenFile("wavein://", sfAutodetect) == 0)
    {
        cerr << "Error in OpenFile: " << micPlayer->GetError() << endl;
        micPlayer->Release();
        closesocket(udata->micsocket);
        return FALSE;
    }

    // setup callback function whenever the microphone collects a sound
    micPlayer->SetCallbackFunc(micCallbackFunc, (TCallbackMessage) (MsgWaveBuffer | MsgStop), (VOID*) udata);
    // start listening to mic
    micPlayer->Play(); 

    while (TRUE)
    {
        char* buffer = new char[MAXBUFSIZE];
        int size = sizeof(udata->micaddr);
        int bytesReceived;
        if ((bytesReceived = recvfrom(udata->micsocket, buffer, MAXBUFSIZE, 0, 
            (SOCKADDR*)&udata->micaddr, &size)) == SOCKET_ERROR)
        {
            cerr << "Error in recvfrom: " << WSAGetLastError() << endl;
            closesocket(udata->micsocket);
            break;
        }

        // push raw data to stream
        streamPlayer->PushDataToStream(buffer, bytesReceived);
        delete buffer;

        if (bytesReceived == 0) {
            closesocket(udata->micsocket);
            break;
        }
        
        // play the sound
        streamPlayer->Play();
        
        // detect the status of the stream
        TStreamStatus status;
        micPlayer->GetStatus(&status);

        // if not playing, then release it 
        if (status.fPlay == 0)
            break;

        // Retrieve current position in TStreamTime format. 
        // If stream is not playing or stream is closed, position is 0.
        TStreamTime pos;
        micPlayer->GetPosition(&pos);
        /*cout << "Pos: " << pos.hms.hour << ":" << pos.hms.minute << ":"
             << pos.hms.second << "." << pos.hms.millisecond << endl;*/
    }

    micPlayer->Release();
    return TRUE;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    micCallbackFunc
--
-- DATE:        March 10, 2017
--
-- REVISIONS:	
--
-- DESIGNER:	
--
-- PROGRAMMER:
--
-- INTERFACE:   int __stdcall micCallbackFunc(void* instance, void* user_data, TCallbackMessage message, 
--              unsigned int param1, unsigned int param2)
--              void* instance - ZPlay instance
--              void* user_data - user data specified by SetCallback
--              TCallbackMessage message - stream message. eg. fetch next song, need more streaming data, etc
--              unsigned int param1 - a pointer to buffer with PCM data
--              unsigned int param2 - number of bytes in PCM buffer
--
-- RETURNS:     Based on the type of the TCallbackMessage (message) passed. See notes below.
--
-- NOTES:
-- This function will mainly listen for the MsgWaveBuffer message, which is a message for when a decoding thread is
-- ready to send data to the soundcard.
-- Important parameters:
--      param1 - pointer to memory PCM buffer
--      param2 - number of bytes in PCM data buffer
--      Returns:
--      0 - send data to soundcard
--      1 - skip sending data to soundcard
--      2 - stop playing
--      For MsgStop, all the parameters and the return type are not used. The message is used after a song stops playing.
----------------------------------------------------------------------------------------------------------------------*/

int __stdcall micCallbackFunc(void * instance, void * user_data, TCallbackMessage message, 
                              unsigned int param1, unsigned int param2)
{
    MICSOCKET* udata = (MICSOCKET *) user_data;

    if ( message == MsgStop)
    {
        micPlayer->Stop();
        streamPlayer->Stop();
        closesocket(udata->micsocket);
        return 2;
    }
    
    if (sendto(udata->micsocket, (const char *) param1, param2, 0, (const SOCKADDR *)& udata->micaddr, 
        sizeof(udata->micaddr)) < 0)
    {
        cerr << "Error in sendto: " << GetLastError() << endl;
        
        return 1;
    }

    return 0;
}