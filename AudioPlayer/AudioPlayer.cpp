/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE:		AudioPlayer.cpp -  This file contains the implementations of the client member functions
--
-- PROGRAM:			AudioPlayer
--
-- FUNCTIONS:		bool AudioPlayer::dispatchWSASendRequest(LPSOCKETDATA data) 
--					bool AudioPlayer::dispatchWSARecvRequest(LPSOCKETDATA data)
--					bool AudioPlayer::runClient(WSADATA* wsadata, const char* hostname, const int port)
--					void AudioPlayer::dispatchRecv()
--					void AudioPlayer::freeData(LPSOCKETDATA data)
--					void AudioPlayer::dispatchSend(string usrData)
--					void AudioPlayer::sendComplete (DWORD error, DWORD bytesTransferred, LPWSAOVERLAPPED overlapped, DWORD flags)
--					void AudioPlayer::recvComplete (DWORD error, DWORD bytesTransferred, LPWSAOVERLAPPED overlapped, DWORD flags)
--					void CALLBACK AudioPlayer::runSendComplete (DWORD error, DWORD bytesTransferred, LPWSAOVERLAPPED overlapped, DWORD flags)
--					void CALLBACK AudioPlayer::runRecvComplete (DWORD error, DWORD bytesTransferred, LPWSAOVERLAPPED overlapped, DWORD flags)
--					DWORD WINAPI AudioPlayer::runDLThread(LPVOID params)
--					DWORD AudioPlayer::dlThread(LPVOID params)
--					DWORD WINAPI AudioPlayer::runULThread(LPVOID params)
--					DWORD AudioPlayer::ulThread(LPVOID params)
--					LPSOCKETDATA AudioPlayer::allocData(SOCKET socketFD)
--					SOCKET AudioPlayer::createTCPClient(WSADATA* wsaData, const char* hostname, const int port)
-- 
-- DATE:			March 8, 2017
--
-- REVISIONS: 
--
-- DESIGNER:		Fred Yang
--
-- PROGRAMMER:		Fred Yang
--
-- NOTES: 
----------------------------------------------------------------------------------------------------------------------*/

#include "AudioPlayer.h"

using namespace std;
using namespace libZPlay;
HWND ghWnd;
DWORD totalBytesReceived;
DWORD totalBytesSent;

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	createTCPClient
--
-- DATE:		March 8, 2017
--
-- REVISIONS:	
--
-- DESIGNER:	
--
-- PROGRAMMER:	
--
-- INTERFACE:	SOCKET createTCPClient(WSADATA* wsaData, const char* hostname, const int port) 
--				wsaData: pointer to WSADATA struct
--				hostname: the host to connect to
--				port: the port number specified
--
-- RETURNS:		returns an async socket on success,NULL on failure
--
-- NOTES:
-- Called to create a TCP async socket
--
----------------------------------------------------------------------------------------------------------------------*/
SOCKET AudioPlayer::createTCPClient(WSADATA* wsaData, const char* hostname, const int port) 
{
    // The highest version of Windows Sockets spec that the caller can use
	WORD wVersionRequested = MAKEWORD( 2, 2 );

	SOCKET connectSocket;

    // Open up a Winsock session
	if (WSAStartup(wVersionRequested, wsaData) != 0)
	{
		WSACleanup();
		return NULL;
	}

    // create socket given socket type and protocol
	if ((connectSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
	{
		MessageBox(NULL, "Create socket error", "WSASocket Error", MB_ICONERROR);
		return NULL;
	}

	// Initialize and set up the address structure
	memset((char *)&addr_, 0, sizeof(addr_));
	addr_.sin_family = AF_INET;
	addr_.sin_port = htons(port); 

	if ((hp_ = gethostbyname(hostname)) == NULL) 
	{
		MessageBox(NULL, "Unknown server address", "Connection Error", MB_ICONERROR);
		return NULL;
	}

	// Copy the server address
	memcpy((char *)&addr_.sin_addr, hp_->h_addr, hp_->h_length);

	return connectSocket;

}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	runClient
--
-- DATE:		March 8, 2017
--
-- REVISIONS:
--
-- DESIGNER:
--
-- PROGRAMMER:
--
-- INTERFACE:	bool runClient(WSADATA* wsadata, const char* hostname, const int port)
--				wsaData: pointer to WSADATA struct
--				hostname: the host to connect to
--				port: the port number specified
--
-- RETURNS:		true on success and false on failure
--
-- NOTES:		
-- Called to create a TCP client and connect to the server if that call succeeded.
--
----------------------------------------------------------------------------------------------------------------------*/
bool AudioPlayer::runClient(WSADATA* wsadata, const char* hostname, const int port)
{
    //create a socket
    connectSocket_ = createTCPClient(wsadata, hostname, port);

    if (connectSocket_ != NULL) {
        //connect the socket
        if (WSAConnect(connectSocket_, (struct sockaddr *)&addr_, sizeof(addr_), NULL, NULL, NULL, NULL) == INVALID_SOCKET)
        {
            MessageBox(NULL, "Can't connect to server", "Connection Error", MB_ICONERROR);
            return FALSE;
        }

        currentState = WAITFORCOMMAND;

        return TRUE;
    }

    return FALSE;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	dispatchWSARecvRequest
--
-- DATE:		March 9, 2017
--
-- REVISIONS:	
--
-- DESIGNER:	
--
-- PROGRAMMER:	
--
-- INTERFACE:	bool dispatchWSARecvRequest(LPSOCKETDATA data)
--				data: pointer to a LPSOCKETDATA struct which contains the information needed for a WSARecv
--				call, including the socket, buffers, etc.
--
-- RETURNS:		true on success and false on failure
--
-- NOTES:		
-- This function posts a WSARecv (Async Recv call) request specifying a call back function to be 
-- executed upon the completion of recv call.
--
----------------------------------------------------------------------------------------------------------------------*/
bool AudioPlayer::dispatchWSARecvRequest(LPSOCKETDATA data)
{
	DWORD flag = 0;
	DWORD bytesReceived = 0;
	int error;
    char stats[MAX_PATH] = "";

	if(data)
	{
		// create a client request context which includes a client and a data structure
		REQUESTCONTEXT* rc = (REQUESTCONTEXT*) malloc(sizeof(REQUESTCONTEXT));
		rc->clnt = this;
		rc->data = data;
		data->overlap.hEvent = rc;

		// async recv, runRecvComplete will be called upon completion
		error = WSARecv(data->sock, &data->wsabuf, 1, &bytesReceived, &flag, &data->overlap, runRecvComplete);

		if(error == 0 || (error == SOCKET_ERROR && WSAGetLastError() == WSA_IO_PENDING))
		{
            if (bytesReceived > 0 && bytesReceived != totalBytesReceived)
            {
                totalBytesReceived = bytesReceived;
                sprintf(stats, "Received: %d bytes", totalBytesReceived);
                SendMessage(GetDlgItem(ghWnd, IDC_MAIN_STATUS), SB_SETTEXT, STATUSBAR_STATS, (LPARAM)stats);
            }
			return TRUE;
		}
		else
		{
			freeData(data);
			free(rc);
			return FALSE;
		}

	}

	return FALSE;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	runRecvComplete
--
-- DATE:		March 8, 2017
--
-- REVISIONS:	
--
-- DESIGNER:	
--
-- PROGRAMMER:	
--
-- INTERFACE:	void CALLBACK runRecvComplete (DWORD error, DWORD bytesTransferred, LPWSAOVERLAPPED overlapped, DWORD flags)
--				error: indicates if there were any errors in the async receive call
--				bytesTransferred: number of bytes received
--				overlapped: the overlapped structure used to make the async recv call
--				flags: other flags
--
-- RETURNS:		void
--
-- NOTES:		
-- Called whenever an async recv request was completed.
--				
----------------------------------------------------------------------------------------------------------------------*/
void CALLBACK AudioPlayer::runRecvComplete (DWORD error, DWORD bytesTransferred, LPWSAOVERLAPPED overlapped, DWORD flags)
{
	REQUESTCONTEXT* rc = (REQUESTCONTEXT*) overlapped->hEvent;
	AudioPlayer* clnt = (AudioPlayer*) rc->clnt;
	clnt->recvComplete(error, bytesTransferred, overlapped, flags);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	recvComplete
--
-- DATE:		March 9, 2017
--
-- REVISIONS:	
--
-- DESIGNER:	
--
-- PROGRAMMER:	
--
-- INTERFACE:	void recvComplete (DWORD error, DWORD bytesTransferred, LPWSAOVERLAPPED overlapped, DWORD flags)
--				error: indicates if there were any errors in the async receive call
--				bytesTransferred: number of bytes received
--				overlapped: the overlapped structure used to make the asynf recv call
--				flags: other flags
--
-- RETURNS:		void
--
-- NOTES:		
-- Executed after each async recv call is completed. It extracts the received data from
-- the data buffers and then changes the client state.
--
----------------------------------------------------------------------------------------------------------------------*/
void AudioPlayer::recvComplete (DWORD error, DWORD bytesTransferred, LPWSAOVERLAPPED overlapped, DWORD flags)
{
	REQUESTCONTEXT* rc = (REQUESTCONTEXT*) overlapped->hEvent;
	LPSOCKETDATA data = (LPSOCKETDATA) rc->data;
	AudioPlayer* clnt = rc->clnt;
    string tmp, extra;
    DWORD fileSize;
    int reqType;
	bool endOfTransmit = false;
	bool endOfList = false;

	if(error || bytesTransferred == 0)
	{
		freeData(data);
		return;
	}
	
	tmp.append(data->databuf, bytesTransferred);

	// if entire file downloaded, end the transmit
	if(clnt->downloadedAmount >= clnt->dlFileSize)
		endOfTransmit = true;

    // if last character is EOT, end the list
	if(tmp[tmp.size() - 1] == EOT)
		endOfList = true;

	// open a input string stream from the binary string
	istringstream iss(tmp);

	switch(clnt->currentState)
	{
	case WAITFORSTREAM:
		if(iss >> reqType && iss >> fileSize)
		{
			clnt->dlFileSize = fileSize;
			clnt->downloadedAmount = 0;

			// streaming approved
			clnt->currentState = STREAMING; 
			
			char buff[TMPBUFSIZE];
            int  bytesReceived;
			string firstframe;
			
			// check audio format
			TStreamFormat format;
			string::size_type pos = clnt->currentAudioFile.find_last_of(".");
			tmp = clnt->currentAudioFile.substr(pos);

			if (tmp == ".wav") format = sfWav;
			else if (tmp == ".mp3") format = sfMp3;
			else if (tmp == ".ogg") format = sfOgg;
			else {
				MessageBox(NULL, "Invalid audio format!", "ERROR", MB_OK);
				return;
			}

            // open the stream
			while (!player_->OpenStream(true, true, firstframe.data(), firstframe.size(), format))
			{
                bytesReceived = recv(connectSocket_, buff, TMPBUFSIZE, 0);
                if (bytesReceived > 0)
                {
                    clnt->downloadedAmount += bytesReceived;
                    firstframe.append(buff, bytesReceived);
                    bytesReceived = 0;
                }
			}
		}
		else
		{
			clnt->currentState = WAITFORCOMMAND;
		}
		break;

	case WAITFORLIST:
		{
			cachedPlayList.append(tmp);
			cachedPlayList.erase(0, cachedPlayList.find_first_not_of(' '));

			if (endOfList) {
				clnt->currentState = WAITFORCOMMAND;
			}
		}

		break;

	case WAITFORDOWNLOAD:	//after download request was sent in dlThread
		if(iss >> reqType && iss >> fileSize)
		{
			clnt->dlFileSize = fileSize;
			clnt->downloadedAmount = 0;

			// download request Approved
			clnt->currentState = DOWNLOADING; 
			clnt->downloadFileStream.open(clnt->currentAudioFile, ios::binary);
		}
		else
		{
			clnt->currentState = WAITFORCOMMAND;
		}

		break;

	case WAITFORUPLOAD:	// after upload request was sent in ulThread
		if(iss >> reqType && getline(iss, extra)) {	//get request type and file name

            // trim leading white space in file name
			extra.erase(0, extra.find_first_not_of(' ')); 
			if(extra.empty())
			{
				clnt->currentState = WAITFORCOMMAND;
				break;
			}

			// upload request approved
			clnt->currentState = UPLOADING; 
			clnt->uploadedAmount = 0;
		}

		break;

	case DOWNLOADING:
		if(clnt->downloadedAmount >= clnt->dlFileSize)	// the entire file bytes received
		{
			clnt->currentState = WAITFORCOMMAND;	// set state to waiting for command
			clnt->downloadFileStream.close();	    // close the file stream
			clnt->dlFileSize = 0;			        // reset file size
			clnt->downloadedAmount = 0;             // reset downloaded bytes
		}
		else	
		{
			clnt->downloadedAmount += bytesTransferred; // update bytes
			clnt->downloadFileStream.write(tmp.c_str(), tmp.size()); //write to the file
		}
		break;

	case STREAMING:
		if(clnt->downloadedAmount >= clnt->dlFileSize)
		{
			clnt->currentState = WAITFORCOMMAND;
			clnt->downloadFileStream.close();
			clnt->dlFileSize = 0;
			clnt->downloadedAmount = 0;
		}
		else
		{
			clnt->downloadedAmount += bytesTransferred; 
			player_->PushDataToStream(data->wsabuf.buf, bytesTransferred);
			player_->Play();
		}
		break;

	case LISTENMULTICAST:
		break;

	case MICROPHONE:
		break;
	}

	freeData(data);

}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	dispatchWSASendRequest
--
-- DATE:		March 10, 2017
--
-- REVISIONS:	
--
-- DESIGNER:	
--
-- PROGRAMMER:	
--
-- INTERFACE:	TStreamFormat parseFileFormat(const string filename)
--              const string filename: file name specified
--
-- RETURNS:		TStreamFormat -- type of libZplay song format
--				
-- NOTES:		
-- This parses a filename (string) and detects what format it is based on the file extension.
--
----------------------------------------------------------------------------------------------------------------------*/
TStreamFormat parseFileFormat(const string filename)
{
	string format;
	string::size_type pos = filename.find_last_of(".");
	format = filename.substr(pos);

	if (format == ".wav") return sfWav;
	else if (format == ".mp3") return sfMp3;
	else if (format == ".ogg") return sfOgg;
	
	return sfAutodetect; //ERROR: invalid song format
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	dispatchWSASendRequest
--
-- DATE:		March 9, 2017
--
-- REVISIONS:	
--
-- DESIGNER:	
--
-- PROGRAMMER:	
--
-- INTERFACE:	bool dispatchWSASendRequest(LPSOCKETDATA data)
--				data: pointer to a LPSOCKETDATA struct which contains the information needed for a WSARecv,
--			          call, including the socket, buffers, etc.
--
-- RETURNS:		true on success and false on failure
--
-- NOTES:		
-- This function posts a WSASend (Async Send call) request specifying a call back function to be 
-- executed upon the completion of send call
--
----------------------------------------------------------------------------------------------------------------------*/
bool AudioPlayer::dispatchWSASendRequest(LPSOCKETDATA data)
{
	DWORD flag = 0;
	DWORD bytesSent = 0;
	int error;

	// create a client request context which includes a client and a data structure
	REQUESTCONTEXT* rc = (REQUESTCONTEXT*) malloc(sizeof(REQUESTCONTEXT));
	rc->clnt = this;
	rc->data = data;
	data->overlap.hEvent = rc;

	// perform the async send and return right away
	error = WSASend(data->sock, &data->wsabuf, 1, &bytesSent, flag, &data->overlap, runSendComplete);

	if(error == 0 || (error == SOCKET_ERROR && WSAGetLastError() == WSA_IO_PENDING))
	{
		return TRUE;
	}
	else
	{
		freeData(data);
		free(rc);
		//MessageBox(NULL, "WSASend() failed", "Critical Error", MB_ICONERROR);
		return FALSE;
	}

}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	runSendComplete
--
-- DATE:		March 10, 2017
--
-- REVISIONS:	
--
-- DESIGNER:	
--
-- PROGRAMMER:	
--
-- INTERFACE:	void CALLBACK runSendComplete (DWORD error, DWORD bytesTransferred, LPWSAOVERLAPPED overlapped, DWORD flags)
--				error: indicates if there were any errors in the async receive call
--				bytesTransferred: number of bytes received
--				overlapped: the overlapped structure used to make the asynf recv call
--				flags: other flags
--
-- RETURNS:		void
--				
--
-- NOTES:		
-- Called whenever an async send request was completed.
--				
----------------------------------------------------------------------------------------------------------------------*/
void CALLBACK AudioPlayer::runSendComplete (DWORD error, DWORD bytesTransferred, LPWSAOVERLAPPED overlapped, DWORD flags)
{
	REQUESTCONTEXT* rc = (REQUESTCONTEXT*) overlapped->hEvent;
	AudioPlayer* clnt = (AudioPlayer*) rc->clnt;
	clnt->sendComplete(error, bytesTransferred, overlapped, flags);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	sendComplete
--
-- DATE:		March 10, 2017
--
-- REVISIONS:	
--
-- DESIGNER:	
--
-- PROGRAMMER:	
--
-- INTERFACE:	void sendComplete (DWORD error, DWORD bytesTransferred, LPWSAOVERLAPPED overlapped, DWORD flags)
--				error: indicates if there were any errors in the async receive call
--				bytesTransferred: number of bytes received
--				overlapped: the overlapped structure used to make the asynf recv call
--				flags: other flags
--
-- RETURNS:		void
--
-- NOTES:		
-- Executed after each async send call is completed to send the data and then change the client state.
--
----------------------------------------------------------------------------------------------------------------------*/
void AudioPlayer::sendComplete (DWORD error, DWORD bytesTransferred, LPWSAOVERLAPPED overlapped, DWORD flags)
{
	REQUESTCONTEXT* rc = (REQUESTCONTEXT*) overlapped->hEvent;
	LPSOCKETDATA data = (LPSOCKETDATA) rc->data;
	AudioPlayer* clnt = rc->clnt;
	bool endOfTransmit = false;

	if(error || bytesTransferred == 0)
	{
		freeData(data);
		return;
	}

	// check the current state once data transfer completed
	switch(clnt->currentState)
	{
	case SENTLISTREQUEST: // list request
		clnt->currentState = WAITFORLIST;
		dispatchRecv();
		break;
	case SENTSTREQUEST: // streaming request
		clnt->currentState = WAITFORSTREAM;
		dispatchRecv();
		break;
	case SENTDLREQUEST:	// download request
		clnt->currentState = WAITFORDOWNLOAD;
		dispatchRecv();
		break;

	case SENTULREQUEST:	// upload request
		clnt->currentState = WAITFORUPLOAD;
		dispatchRecv();
		break;

	case UPLOADING:		// uploading
		if(clnt->ulFileSize == clnt->uploadedAmount)
		{
			clnt->currentState = WAITFORCOMMAND;
			clnt->uploadFileStream.close();
			clnt->ulFileSize = 0;
			clnt->uploadedAmount = 0;
		}

	    clnt->uploadedAmount += bytesTransferred;
		break;

	}
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	dispatchSend
--
-- DATE:		March 10, 2017
--
-- REVISIONS:	
--
-- DESIGNER:	
--
-- PROGRAMMER:	
--
-- INTERFACE:	void dispatchSend(string usrData)
--				data: the user data to send
--
-- RETURNS:		void
--				
--
-- NOTES:		
-- This function makes an async Send request and then puts the thread in an alertable
-- state, meaning the same thread will handle the remaining stuff till completion.
--
----------------------------------------------------------------------------------------------------------------------*/
void AudioPlayer::dispatchSend(string usrData)
{
	SOCKETDATA* data = allocData(connectSocket_);
	
	// fillup the data buffers
	memcpy(data->databuf, usrData.c_str(), usrData.size());
	data->wsabuf.len = usrData.size();

	if(data)
	{
		dispatchWSASendRequest(data);
	}

    // make this thread alertable
	::SleepEx(INFINITE, TRUE); 

}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	dispatchRecv
--
-- DATE:		March 10, 2017
--
-- REVISIONS:	
--
-- DESIGNER:	
--
-- PROGRAMMER:	
--
-- INTERFACE:	void AudioPlayer::dispatchRecv()
--
-- RETURNS:		void
--
-- NOTES:		
-- This function makes an async Recv request and then puts the thread in an alertable
-- state, meaning the same thread will handle the remaining stuff till completion.
--
----------------------------------------------------------------------------------------------------------------------*/
void AudioPlayer::dispatchRecv()
{
	SOCKETDATA* data = allocData(connectSocket_);
	
	if(data)
	{
		dispatchWSARecvRequest(data);
	}

    //make this thread alertable
	::SleepEx(INFINITE, TRUE); 

}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	runDLThread
--
-- DATE:		March 12, 2017
--
-- REVISIONS:	
--
-- DESIGNER:	
--
-- PROGRAMMER:	
--
-- INTERFACE:	DWORD WINAPI runDLThread(LPVOID params)
--				params: points to the client object
--
-- RETURNS:		The result of the thread operation
--
-- NOTES:		
-- Called to run the download thread.
--				
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI AudioPlayer::runDLThread(LPVOID params)
{
	AudioPlayer* clnt = (AudioPlayer*) params;
	return clnt->dlThread(clnt);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	dlThread
--
-- DATE:		March 11, 2017
--
-- REVISIONS:	
--
-- DESIGNER:	
--
-- PROGRAMMER:	
--
-- INTERFACE:	DWORD dlThread(LPVOID params)
--				params: points to the client object
--
-- RETURNS:		TRUE after download was completed
--
-- NOTES:		
-- A thread posts requests to receive data off the socket, while the client is in download mode.
--				
----------------------------------------------------------------------------------------------------------------------*/
DWORD AudioPlayer::dlThread(LPVOID params)
{
    char tm[MAX_PATH] = "";
    char prog[MAX_PATH] = "";
    clock_t start;
    DWORD elapsed;
    string userRequest;

	AudioPlayer* clnt = (AudioPlayer*) params;

	userRequest += to_string(REQDOWNLOAD) + " ";
	userRequest += clnt->currentAudioFile;
	userRequest += "\n";

	clnt->currentState = SENTDLREQUEST;
	clnt->dispatchSend(userRequest);

    start = clock();

	while(TRUE)
	{
		if(clnt->currentState != DOWNLOADING)
		{
			if(clnt->currentState == WAITFORCOMMAND)
				break;

			continue;
		}
		dispatchRecv();
	}

    elapsed = clock() - start;
    sprintf(tm, "Time elapsed: %ld ms", elapsed);
    sprintf(prog, "Download done.");
    SendMessage(GetDlgItem(ghWnd, IDC_MAIN_STATUS), SB_SETTEXT, STATUSBAR_TIME, (LPARAM)tm);
    SendMessage(GetDlgItem(ghWnd, IDC_MAIN_STATUS), SB_SETTEXT, STATUSBAR_PROG, (LPARAM)prog);

	return TRUE;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	runSTThread
--
-- DATE:		March 11, 2017
--
-- REVISIONS:
--
-- DESIGNER:
--
-- PROGRAMMER:
--
-- INTERFACE:	DWORD runSTThread(LPVOID params)
--				params: points to the client object
--
-- RETURNS:		based on the result of stThread
--
-- NOTES:
-- Called to run the streaming thread.
--
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI AudioPlayer::runSTThread(LPVOID params)
{
	AudioPlayer* clnt = (AudioPlayer*) params;
	return clnt->stThread(clnt);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	stThread
--
-- DATE:		March 11, 2017
--
-- REVISIONS:
--
-- DESIGNER:
--
-- PROGRAMMER:
--
-- INTERFACE:	DWORD stThread(LPVOID params)
--				params: points to the client object
--
-- RETURNS:		TRUE on success, FALSE on failure
--
-- NOTES:
-- A thread posts requests to receive data off the socket, while the client is in streaming mode.
--
----------------------------------------------------------------------------------------------------------------------*/
DWORD AudioPlayer::stThread(LPVOID params)
{
    char tm[MAX_PATH] = "";
    char prog[MAX_PATH] = "";
    clock_t start;
    DWORD elapsed;
    string userRequest;

	AudioPlayer* clnt = (AudioPlayer*) params;

	userRequest += to_string(REQSTREAM) + " ";
	userRequest += clnt->currentAudioFile;
	userRequest += "\n";

	clnt->currentState = SENTSTREQUEST;
	clnt->dispatchSend(userRequest);

    start = clock();

	while(TRUE)
	{
		if(clnt->currentState != STREAMING)
		{
			if(clnt->currentState == WAITFORCOMMAND)
				break;

			continue;
		}
		dispatchRecv();
	}

    elapsed = clock() - start;
    sprintf(tm, "Time elapsed: %ld ms", elapsed);
    sprintf(prog, "Streaming done.");
    SendMessage(GetDlgItem(ghWnd, IDC_MAIN_STATUS), SB_SETTEXT, STATUSBAR_TIME, (LPARAM)tm);
    SendMessage(GetDlgItem(ghWnd, IDC_MAIN_STATUS), SB_SETTEXT, STATUSBAR_PROG, (LPARAM)prog);

	return TRUE;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	runULThread
--
-- DATE:		March 10, 2017
--
-- REVISIONS:	
--
-- DESIGNER:	
--
-- PROGRAMMER:	
--
-- INTERFACE:	DWORD WINAPI runULThread(LPVOID params)
--				params: points to the client object
--
-- RETURNS:		based on the result of ulThread
--
-- NOTES:		
-- Called to run the upload thread.
--				
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI AudioPlayer::runULThread(LPVOID params)
{
	AudioPlayer* clnt = (AudioPlayer*) params;
	return clnt->ulThread(clnt);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	ulThread
--
-- DATE:		March 12, 2017
--
-- REVISIONS:	
--
-- DESIGNER:	
--
-- PROGRAMMER:	
--
-- INTERFACE:	DWORD ulThread(LPVOID params)
--				params: points to the client object
--
-- RETURNS:		TRUE on success, FALSE on failure
--
-- NOTES:		
-- While the client is in upload state, it reads data from the file and sends it to the server
-- by posting async send calls
--
----------------------------------------------------------------------------------------------------------------------*/
DWORD AudioPlayer::ulThread(LPVOID params)
{
	AudioPlayer* clnt = (AudioPlayer*) params;
    char prog[MAX_PATH] = "";
	streamsize bytesRead;
	string userRequest;
	ostringstream oss;

	clnt->uploadFileStream.open(clnt->currentAudioFile, ios::binary);
	streampos begin, end;
	begin = clnt->uploadFileStream.tellg();
	clnt->uploadFileStream.seekg(0, ios::end);
	clnt->ulFileSize = static_cast<long int>(clnt->uploadFileStream.tellg()-begin);
	clnt->uploadFileStream.seekg(begin);

	oss << REQUPLOAD << " " << clnt->ulFileSize << clnt->currentAudioFile << "\n";
	userRequest = oss.str();

	clnt->currentState = SENTULREQUEST;
	clnt->dispatchSend(userRequest);

	while(TRUE)
	{
		if (!uploadFileStream.is_open())
			return FALSE;

		char* tmp;
		string data;

		tmp = new char [STREAMBUFSIZE];
		memset(tmp, 0, STREAMBUFSIZE);
		bytesRead = 0;
		data.clear();

		clnt->uploadFileStream.read(tmp, STREAMBUFSIZE);
		if((bytesRead = clnt->uploadFileStream.gcount()) > 0)
		{
			data.append(tmp, (unsigned long) bytesRead);
			dispatchSend(data);
			data.clear();
		}

		delete[] tmp;
		
		if(clnt->uploadedAmount == clnt->ulFileSize)
		{
			clnt->currentState = WAITFORCOMMAND;
			break;
		
		}
	}
	
    sprintf(prog, "Upload done.");
    //SendMessage(GetDlgItem(ghWnd, IDC_MAIN_STATUS), WM_SETREDRAW, 0, 0);
    //SendMessage(GetDlgItem(ghWnd, IDC_MAIN_STATUS), SB_SETTEXT, STATUSBAR_PROG, (LPARAM)prog);
    //SendMessage(GetDlgItem(ghWnd, IDC_MAIN_STATUS), WM_SETREDRAW, 1, 0);
    MessageBox(NULL, "Upload done", "Upload", MB_ICONINFORMATION);

	return TRUE;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	allocData
--
-- DATE:		March 11, 2017
--
-- REVISIONS:	
--
-- DESIGNER:	
--
-- PROGRAMMER:	
--
-- INTERFACE:	LPSOCKETDATA allocData(SOCKET socket)
--              SOCKET socket: the socket specified
--
-- RETURNS:		returns a pointer to the memory block that was allocated for the LPSOCKETDATA struct
--
-- NOTES:		
-- This function is used to safely allocate memory for a LPSOCKETDATA type variable.
--
----------------------------------------------------------------------------------------------------------------------*/
LPSOCKETDATA AudioPlayer::allocData(SOCKET socket)
{
	LPSOCKETDATA data = NULL;

	try{
		data = new SOCKETDATA();
	} catch(std::bad_alloc&) {
		MessageBox(NULL, "Allocate socket data failed", "Error!", MB_ICONERROR);
		return NULL;
	}

	data->overlap.hEvent = (WSAEVENT)data;
	data->sock = socket;
	data->wsabuf.buf = data->databuf;
	data->wsabuf.len = sizeof(data->databuf);

	return data;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	freeData
--
-- DATE:		March 10, 2017
--
-- REVISIONS:	
--
-- DESIGNER:	
--
-- PROGRAMMER:	
--
-- INTERFACE:	void freeData(LPSOCKETDATA data)
--				data: pointer to a LPSOCKETDATA struct that contain the information needed to 
--					  perform an async call
--
-- RETURNS:		void
--
-- NOTES:		
-- This function is used to safely free the memory block that was allocated for the 
-- LPSOCKETDATA struct.
----------------------------------------------------------------------------------------------------------------------*/
void AudioPlayer::freeData(LPSOCKETDATA data)
{
	if(data)
	{
		delete data;
	}
}
