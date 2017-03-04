#include "Microphone.h"

using namespace std;
using namespace libZPlay;

// stream player
ZPlay* streamPlayer;

// microphone player
ZPlay* micPlayer;

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	startMicChat
--
-- DATE:		March 8, 2017
--
-- REVISIONS:	
--
-- DESIGNER:	
--
-- PROGRAMMER:	
--
-- INTERFACE:	int startMicChat()
--
-- RETURNS:		TRUE on success, FALSE on failure
--				
--
-- NOTES:
-- This function will start microphone chat session.
----------------------------------------------------------------------------------------------------------------------*/
bool startMicChat()
{
    WSADATA wsadata;
	LPMICSOCKET micvar = (LPMICSOCKET) malloc(sizeof(MICSOCKET));
	micvar->micsocket = createListenSocket(&wsadata, UDP, &micvar->micaddr);

    // streaming player to sample/ the raw audio
	streamPlayer = CreateZPlay();
    streamPlayer->SetSettings(sidSamplerate, 44100);// 44.1K sampling rate
    streamPlayer->SetSettings(sidChannelNumber, 2); // 2 channels
    streamPlayer->SetSettings(sidBitPerSample, 16); // 16 bit sample
    streamPlayer->SetSettings(sidBigEndian, 1);     // little endian

    int streamBlock;    // memory block with stream data
	if (streamPlayer->OpenStream(1, 1, &streamBlock, 1, sfPCM) == 0)
	{
		cerr << "Error opening a microphone stream: " << streamPlayer->GetError() << endl;
		streamPlayer->Release();
		closesocket(micvar->micsocket);
		return FALSE;
	}

    // microphone player to open microphone device & collect sounds
	micPlayer = CreateZPlay();
	if (micPlayer->OpenFile("wavein://", sfAutodetect) == 0)
	{
		cerr << "Error in OpenFile: " << micPlayer->GetError() << endl;
		micPlayer->Release();
		closesocket(micvar->micsocket);
		return FALSE;
	}

    // setup callback function whenever the microphone collects a sound
	micPlayer->SetCallbackFunc(micCallbackFunc, (TCallbackMessage) (MsgWaveBuffer | MsgStop), (VOID*) micvar);
    // start listening to mic
	micPlayer->Play(); 

	while (TRUE)
	{
		char* buffer = new char[DATABUFSIZE];
		int size = sizeof(micvar->micaddr);
		int bytesReceived;
		if ((bytesReceived = recvfrom(micvar->micsocket, buffer, DATABUFSIZE, 0, 
            (SOCKADDR*)&micvar->micaddr, &size)) == SOCKET_ERROR)
		{
			cerr << "Error in recvfrom: " << WSAGetLastError() << endl;
			closesocket(micvar->micsocket);
			break;
		}

        // push raw data to stream
		streamPlayer->PushDataToStream(buffer, bytesReceived);
		delete buffer;

		if (bytesReceived == 0) {
			closesocket(micvar->micsocket);
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
		//TStreamTime pos;
		//micPlayer->GetPosition(&pos);
		/*cout << "Pos: " << pos.hms.hour << ":" << pos.hms.minute << ":" <<
			pos.hms.second << "." << pos.hms.millisecond << endl;*/
	}

	micPlayer->Release();
	return TRUE;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	micCallbackFunc
--
-- DATE:		March 10, 2017
--
-- REVISIONS:	
--
-- DESIGNER:	
--
-- PROGRAMMER:	
--
-- INTERFACE:	int __stdcall micCallbackFunc(void* instance, void* user_data, TCallbackMessage message, 
--              unsigned int param1, unsigned int param2)
--              void* instance - ZPlay instance
--              void* user_data - user data specified by SetCallback
--              TCallbackMessage message - stream message. eg. fetch next song, need more streaming data, etc
--              unsigned int param1 - a pointer to buffer with PCM data
--              unsigned int param2 - number of bytes in PCM buffer
--
-- RETURNS:		Dependent on the type of the TCallbackMessage (message) passed. See NOTES below.
--
-- NOTES:
-- This function will mainly listen for the MsgWaveBuffer message, which is a message for when a decoding thread is
-- ready to send data to the soundcard.
-- Important parameters:
--      param1 - pointer to memory PCM buffer
--		param2 - number of bytes in PCM data buffer
--	    Returns:
--		0 - send data to soundcard
--		1 - skip sending data to soundcard
--		2 - stop playing
--      For MsgStop, all the parameters and the return type are not used. The message is used after a song stops playing.
----------------------------------------------------------------------------------------------------------------------*/

int __stdcall micCallbackFunc(void * instance, void * user_data, TCallbackMessage message, 
                              unsigned int param1, unsigned int param2)
{
	MICSOCKET* micvar = (MICSOCKET *) user_data;

	if ( message == MsgStop)
	{
		micPlayer->Stop();
		streamPlayer->Stop();
		closesocket(micvar->micsocket);
		return 2;
	}
	
	if (sendto(micvar->micsocket, (const char *) param1, param2, 0, (const SOCKADDR *)& micvar->micaddr, 
        sizeof(micvar->micaddr)) < 0)
	{
		cerr << "Error in sendto: " << GetLastError() << endl;
		
		return 1;
	}

	return 0;
}