/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: AudioStreamer.cpp
--
-- PROGRAM:     AudioStreamer
--
-- FUNCTIONS:
--		        int main(int argc, char * argv[])
--		        DWORD WINAPI listenThread(LPVOID args)
--		        DWORD WINAPI listenRequest(LPVOID param)
--		        ServerState decodeRequest(char * request, string& fileName, int& fileSize)
--		        void handleRequest(ServerState prevState, ServerState currentState, SOCKET clntSocket, string fileName, int fileSize)
--		        DWORD WINAPI mcThread(LPVOID args)
--		        int  __stdcall  mcCallbackFunc(void* instance, void *user_data, libZPlay::TCallbackMessage message, unsigned int param1, unsigned int param2)
--		        string getAudioPath()
--		        int populatePlayList(vector<string>& list)
--
-- DATE:        March 10, 2017
--
-- REVISIONS:   (Date and Description)
--
-- DESIGNER:    
--
-- PROGRAMMER: 
--
-- NOTES:
-- The project uses libZPlay multimedia library. libZplay is one for playing mp3, mp2, mp1, ogg, flac, ac3, aac, 
-- oga, wav and pcm files and streams. 
-- In order for program to work, the directory for the libzplay.lib must be placed in your project folder. 
-- To link the libzplay.lib, right click project Porperties, then go to configuration properties>linker>input,
-- add "libzplay.lib" in the field "additional dependencies". 
-- libzplay.dll must also be placed into your windows/system32 and windows/syswow64.
----------------------------------------------------------------------------------------------------------------------*/

#include "AudioStreamer.h"

using namespace std;
using namespace libZPlay;

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    main
--
-- DATE:        March 10, 2017
--
-- REVISIONS:   (Date and Description)
--
-- DESIGNER:
--
-- PROGRAMMER:
--
-- INTERFACE:   int main(int argc, char* argv[])
--
-- RETURNS:     EXIT_SUCCESS on success, EXIT_ERROR on failure
--
-- NOTES:
-- Main entry for the server side.
--
----------------------------------------------------------------------------------------------------------------------*/
int main(int argc, char* argv[])
{
    string title = "Comm Audio Streamer v1.0";

    // windows size and color style
    HWND console = GetConsoleWindow();
    RECT rect;
    GetWindowRect(console, &rect); //stores the console's current dimensions
    MoveWindow(console, rect.left, rect.top, SCREEN_W, SCREEN_H, TRUE);
    SetConsoleTitle(title.c_str());
    system("Color 1A");
    setCursor();

	cout << "--------------------------------------------------" << endl;
	cout << "   *** " << title << " ***" << endl;
	cout << "--------------------------------------------------" << endl;

    SOCKET listenSocket;
    vector<SOCKET> clientList;

    // contains information about the Windows Sockets implementation.
	WSADATA wsadata;

    // create listen socket
	listenSocket = createListenSocket(&wsadata, TCP);

	CreateThread(NULL, 0, listenThread, (LPVOID) &listenSocket, 0, 0);
	CreateThread(NULL, 0, mcThread, NULL, 0, 0);

	getchar();
	return EXIT_SUCCESS;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    listenThread
--
-- DATE:        March 10, 2017
--
-- REVISIONS:   (Date and Description)
--
-- DESIGNER: 
--
-- PROGRAMMER: 
--
-- INTERFACE:   DWORD WINAPI listenThread(LPVOID params)
--              LPVOID params: points to the client socket
--
-- RETURNS:     TRUE on success, FALSE on failure
--
-- NOTES: 
-- Thread that listens for new client connection requests. It spawns a new thread to serve the client
-- once the client connected successfully.
----------------------------------------------------------------------------------------------------------------------*/

DWORD WINAPI listenThread(LPVOID params)
{
    SOCKET* pListenSock = (SOCKET*) params;

    cout << "Server started, listening on socket " << *pListenSock << endl;

    while(TRUE)
    {
        SOCKADDR_IN addr = {};
        int addrLen = sizeof(addr);

        // Once listen() call has returned, the accept() call should be issued, this will block
        // until a connection request is received from a remote host.
        SOCKET newClientSocket = WSAAccept(*pListenSock, (sockaddr*)&addr, &addrLen, NULL, NULL);

        if(newClientSocket == INVALID_SOCKET)
        {
            if(WSAGetLastError() != WSAEWOULDBLOCK)
            {
                cerr << "accept() failed with error " << WSAGetLastError() << endl;
                break;
            }
        }
        else
        {
            cout << "Socket " << newClientSocket << " accepted." << endl;
			// spawn a new thread to serve the client
			CreateThread(NULL, 0, listenRequest, (LPVOID) &newClientSocket, 0, 0);
        }

        // make the thread alertable, the same thread will handle the remaining stuff till completion
        ::SleepEx(SLEEPSPAN, TRUE);
    }

    return TRUE;
}


/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    listenRequest
--
-- DATE:        March 10, 2017
--
-- REVISIONS:   (Date and Description)
--
-- DESIGNER: 
--
-- PROGRAMMER: 
--
-- INTERFACE:   DWORD WINAPI listenRequest(LPVOID params)
--			    LPVOID params: points to client socket
--
-- RETURNS:     TRUE on success, FALSE on failure
--
-- NOTES: 
-- This function listens for the connection requests from clients. Calls decodeRequest to parse the request received 
-- before handle it.
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI listenRequest(LPVOID params)
{
	SOCKET clntSocket = *((SOCKET*) params);
	int bytesReceived, bytesToRead;
	char request[STREAMBUFSIZE];
	char* req;
	string fileName;
	DWORD fileSize;
    ServerState newState = IDLE, prevState = IDLE;

	while (TRUE)
	{
		memset(request, 0, STREAMBUFSIZE);
		req = request;
		bytesToRead = STREAMBUFSIZE;

		while ((bytesReceived = recv(clntSocket, req, bytesToRead, 0)) > 0)
		{
			req += bytesReceived;
			bytesToRead -= bytesReceived;

			if (bytesReceived <= STREAMBUFSIZE)
				break;
		}

		if (bytesReceived < 0)
		{
			if (GetLastError() == WSAECONNRESET)
			{
				cerr << "client disconnected" << endl;
				return FALSE;
			}

			cout << "Error: " << WSAGetLastError() << endl;
			return FALSE;
		}

		// updated previous state if necessary
		if (prevState != IDLE)
			prevState = newState;

		//get the new current state
		newState = decodeRequest(request, fileName, fileSize);

		if (newState == STATEERR)
			break;

		handleRequest(newState, clntSocket, fileName, fileSize);
	}

	return TRUE;
}


/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    decodeRequest
--
-- DATE:        March 10, 2017
--
-- REVISIONS:   (Date and Description)
--
-- DESIGNER: 
--
-- PROGRAMMER: 
--
-- INTERFACE:   ServerState decodeRequest(char* request, string& fileName, DWORD& fileSize)
--				request - the request packet to parse
--				fileName - the name of the file
--				fileSize - the size of the file
--				
--
-- RETURNS: return ServerState which will indicate one of the specified ServerStates (STREAMING, DOWNLOADING, etc.),
--			return STATEERR on failure.
--
-- NOTES: 
-- This function parses a request and returns the current state of the server.
----------------------------------------------------------------------------------------------------------------------*/
ServerState decodeRequest(char* request, string& fileName, DWORD& fileSize)
{
	//const string req = request;
	stringstream ss(request);
	int reqType = REQIDLE;
    ServerState state = IDLE;

    if (ss >> reqType)
    cout << getCommand(reqType) << ">>>>>";

    switch (reqType) {
    case REQLIST:
        state = LIST;
        break;
    case REQSTREAM:
        state = STREAMING;
        getline(ss, fileName);
        cout << fileName << endl;
        break;
    case REQDOWNLOAD:
        state = DOWNLOADING;
        getline(ss, fileName);
        cout << fileName << endl;
        break;
    case REQUPLOAD:
        state = UPLOADING;
        ss >> fileSize;
        getline(ss, fileName);
        cout << fileName << " (" << fileSize << " b)" << endl;
        break;
    case REQMICCHAT:
        state = MICCHATTING;
        cout << "Starting 2 way chatting" << endl;
        break;
    case REQMULTICAST:
        state = MULTICASTING;
        cout << "Put client on the multicast channel" << endl;
        break;
    default:
        break;
    }
    
    return state;
}


/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    handleRequest
--
-- DATE:        March 10, 2017
--
-- REVISIONS:   (Date and Description)
--
-- DESIGNER: 
--
-- PROGRAMMER: 
--
-- INTERFACE:   void handleRequest(const ServerState& currentState, SOCKET clntSocket, string fileName, DWORD fileSize)
--				currentState - the current state of the server
--				clntSocket - the socket of the client to send
--				fileName - the name of the file being transferred
--				fileSize - size of the file being transferred
--
-- RETURNS:     void
--
-- NOTES:   
-- This function keeps track of the current and previous server state, then executes the steps to handle the current
-- request.
----------------------------------------------------------------------------------------------------------------------*/
void handleRequest(const ServerState& currentState, SOCKET clntSocket, string fileName, DWORD fileSize)
{
    char*			tmp;
	int				bytesSent = 0,          // bytes sent
					bytesReceived = 0;      // bytes received
	int				totalbytesSent	= 0,    // total bytes sent
					totalbytesReceived = 0; // total bytes received
	string			line;                   // line to hold the file name
	ifstream		fileToSend;             // file read stream
	ofstream		fileReceived;           // file write stream
	streamsize		bytesRead;              // bytes to read
	vector<string>	list;                   // play list
	int				count;                  // number of songs
    DWORD           fsize = 0;
	ostringstream oss;
	streampos begin, end;

	switch (currentState)
	{
		case LIST:
			count = populatePlayList((vector<string>&)list);

			if (count > 0)
			{
				for (vector<string>::iterator it = list.begin(); it != list.end(); ++it)
				{
					line += *it;
					line += '\n';
				}

				if (((bytesSent = send(clntSocket, line.c_str(), line.size(), 0))) == 0 || (bytesSent == -1))
				{
					cerr << "Failed to send packet, Error: " << GetLastError() << endl;
					return;
				}
			}

			// send EOT
			line = EOT;
			send(clntSocket, line.c_str(), line.size(), 0);
			cout << "Play list sent" << endl;
		    break;

		case STREAMING:
			// open the audio file
			fileToSend.open(getAudioPath().substr(0,getAudioPath().size()-1).insert(getAudioPath().size()-1,
                            fileName.substr(1,fileName.size())), ios::binary);

			if (!fileToSend.is_open())
			{
                line = to_string(REQSTREAM) + "\n";
				send(clntSocket, line.c_str(), line.size(), 0);
				line = "";
				cout << "Streaming request denied. Can't open file." << endl;
				break;
			}

			// compute size of the file
			begin = fileToSend.tellg();
			fileToSend.seekg(0, ios::end);
			fsize = static_cast<long int>(fileToSend.tellg() - begin);
			fileToSend.seekg(begin);
			cout << fileName << " (" << fsize << " b)" << endl;

			// echo the file size to the client
			oss << REQSTREAM << " " << fsize << "\n";
			line = oss.str();
			send(clntSocket, line.c_str(), line.size(), 0);
			line = "";

			cout << "Streaming..." << endl;

			while (TRUE)
			{
				tmp = new char[TMPBUFSIZE];

				bytesRead = 0;
				fileToSend.read(tmp, TMPBUFSIZE);
				
				if((bytesRead = fileToSend.gcount()) > 0)
				{
					line.append(tmp, static_cast<unsigned int>(bytesRead));
					if (((bytesSent = send(clntSocket, line.c_str(), line.size(), 0))) == 0 || (bytesSent == -1))
					{
						cerr << "Failed to send! Exited with error " << GetLastError() << endl;
						cerr << "Ending streaming session..." << endl;
						fileToSend.close();
						delete[] tmp;
						return;
					}

					totalbytesSent += bytesSent;
					cout << "Bytes sent: " << bytesSent << endl;
					cout << "Total bytes sent: " << totalbytesSent << endl;
					line.clear();
				} 
				
				if (totalbytesSent == fsize)
					break;
			}

			cout << "Done streaming" << endl;
            line = EOT;
			send(clntSocket, line.c_str(), line.size(), 0);
			fileToSend.close();
			delete[] tmp;
			break;

		case DOWNLOADING:
			// open the audio file
			fileToSend.open(getAudioPath().substr(0,getAudioPath().size()-1).insert(getAudioPath().size()-1,
                            fileName.substr(1,fileName.size())), ios::binary);

			if (!fileToSend.is_open())
			{
				line = to_string(REQDOWNLOAD) + "\n";
				send(clntSocket, line.c_str(), line.size(), 0);
				line = "";
				cout << "Download request denied. Can't open file" << endl;
				break;
			}

			// compute size of the file
			begin = fileToSend.tellg();
			fileToSend.seekg(0, ios::end);
			fsize = static_cast<long int>(fileToSend.tellg() - begin);
			fileToSend.seekg(begin);
			cout << fileName << " (" << fsize << " b)" << endl;

			// echo the file size to the client
			oss << REQDOWNLOAD << " " << fsize << "\n";
			line = oss.str();
			send(clntSocket, line.c_str(), line.size(), 0);
			line = "";

			while (TRUE)
			{
				tmp = new char [STREAMBUFSIZE];

				bytesRead = 0;
				fileToSend.read(tmp, STREAMBUFSIZE);
				
				if((bytesRead = fileToSend.gcount()) > 0)
				{
					line.append(tmp, static_cast<unsigned int>(bytesRead));
					if (((bytesSent = send(clntSocket, line.c_str(), line.size(), 0))) == 0 || (bytesSent == -1))
					{
						cerr << "Failed to send! Exited with error " << GetLastError() << endl;
						cerr << "Ending download session..." << endl;
						fileToSend.close();
						delete[] tmp;
						return;
					}

					totalbytesSent += bytesSent;
					cout << "Bytes sent: " << bytesSent << endl;
					cout << "Total bytes sent: " << totalbytesSent << endl;
					line.clear();
				} 
				
				if (totalbytesSent == fsize)
					break;
			}

			cout << "Done downloading" << endl;
            line = EOT;
			send(clntSocket, line.c_str(), line.size(), 0);
			fileToSend.close();
			delete[] tmp;
		    break;

		case UPLOADING:
			cout << "Uploading..." << endl;
			cout << "File size: " << fsize << endl;

			line = to_string(REQUPLOAD) + " " + fileName + "\n";
			send(clntSocket, line.c_str(), line.size(), 0);
			line = "";

			// save file to the folder specified (eg. \\audio)
			fileReceived.open(getAudioPath().substr(0,getAudioPath().size()-1).append(fileName), ios::binary);
			
			if (!fileReceived.is_open())
			{
				line = to_string(REQUPLOAD) + "\n";
				send(clntSocket, line.c_str(), line.size(), 0);
				line = "";
				break;
			}
			
			while (TRUE)
			{
				tmp = new char[STREAMBUFSIZE];
				memset(tmp, 0, STREAMBUFSIZE);

				if (((bytesReceived = recv(clntSocket, tmp, STREAMBUFSIZE, 0)) == 0) || (bytesReceived == -1))
				{
					cerr << "recv failed with error " << GetLastError() << endl;
					cout << "Ending upload session..." << endl;
					fileReceived.close();
					delete[] tmp;
					return;
				}

				fileReceived.write(tmp, bytesReceived);
				totalbytesReceived += bytesReceived;
				cout << "Bytes received: " << bytesReceived << endl;
				cout << "Total bytes received: " << totalbytesReceived << endl;

				if (totalbytesReceived == fileSize) // uploading done
				{
					cout << "Done uploading" << endl;
					fileReceived.close();
					delete[] tmp;
					break;
				}
				delete[] tmp;
			}

		break;

		case MICCHATTING:
			cout << "Mic session started..." << endl;
			startMicChat();
		break;

		case MULTICASTING:
			// Stream audio to the multicast address
            cout << "Multicast started..." << endl;
		break;
	}
}


/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    mcThread
--
-- DATE:        March 11, 2017
--
-- REVISIONS:   (Date and Description)
--
-- DESIGNER: 
--
-- PROGRAMMER: 
--
-- INTERFACE:   DWORD WINAPI mcThread(LPVOID params)
--              LPVOID params: points to client socket
--
-- RETURNS:     TRUE on success, FALSE on failure
--
-- NOTES:
-- This thread starts up a multicast server.  It then iterates through the list of songs available
-- on the server and broadcasts each song to the multicast destination address. If a song cannot be
-- read, it is simply skipped.
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI mcThread(LPVOID params)
{
	char			mcAddr[ADDRSIZE] = MULTICAST_ADDR;   // multicast address
	u_short			nPort = MULTICAST_PORT;              // multicast port
	u_long			mcTTL = MULTICAST_TTL;               // multicast TTL
	DWORD		    nRet,                                // result
					count;                               // number of audios
	BOOL			fFlag;
	SOCKADDR_IN		server,
					destination;

	struct ip_mreq	stMreq;     // struct for IP_MREQ (IP_ADD_MEMBERSHIP, IP_DROP_MEMBERSHIP)
	SOCKET			hSocket;    // socket to create
	WSADATA			stWSAData;
	vector<string>	list;       // play list

	string			dir,        // audio absolute path
					line;       // line to hold the file name
	ifstream*		fileToSend; // file read stream
	fileToSend = new ifstream;

	LPMCSOCKET mcSocket = (LPMCSOCKET) malloc(sizeof(MCSOCKET));

	nRet = WSAStartup(0x0202, &stWSAData);
	if (nRet)
	{
		cerr << "WSAStartup failed: " << nRet << endl;	
		return FALSE;
	}

	hSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (hSocket == INVALID_SOCKET)
	{
		cerr << "socket() failed, Err: " << WSAGetLastError() << endl;
		return FALSE;
	}

	server.sin_family      = AF_INET; 
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port        = 0;

	nRet = bind(hSocket, (struct sockaddr*)&server, sizeof(server));
	if (nRet == SOCKET_ERROR) 
	{
		cerr << "bind() port: " << nPort << " failed, Err: " << WSAGetLastError() << endl;
		return FALSE;
	}

	stMreq.imr_multiaddr.s_addr = inet_addr(mcAddr);
	stMreq.imr_interface.s_addr = INADDR_ANY;

	nRet = setsockopt(hSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&stMreq, sizeof(stMreq));
	if (nRet == SOCKET_ERROR)
	{
		cerr << "setsockopt() IP_ADD_MEMBERSHIP address " << mcAddr << " failed, Err: " << WSAGetLastError() << endl;
		return FALSE;
	}

	nRet = setsockopt(hSocket, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&mcTTL, sizeof(mcTTL));
	if (nRet == SOCKET_ERROR)
	{
		cerr << "setsockopt() IP_MULTICAST_TTL failed, Err: " << WSAGetLastError() << endl;
		return FALSE;
	}

	fFlag = FALSE;
	nRet = setsockopt(hSocket, IPPROTO_IP,  IP_MULTICAST_LOOP, (char *)&fFlag, sizeof(fFlag));
	if (nRet == SOCKET_ERROR)
	{
		cerr << "setsockopt() IP_MULTICAST_LOOP failed, Err: " << WSAGetLastError() << endl;
		return FALSE;
	}

	destination.sin_family =      AF_INET;
	destination.sin_addr.s_addr = inet_addr(mcAddr);
	destination.sin_port =        htons(nPort);

	dir = getAudioPath();
	count = populatePlayList(list);

	if (count > 0)
	{
		// Open libzplay stream and settings
		ZPlay * mcStream = CreateZPlay();
		mcStream->SetSettings(sidSamplerate, 44100); // 44.1K sampling rate
		mcStream->SetSettings(sidChannelNumber, 2);  // 2 channels
		mcStream->SetSettings(sidBitPerSample, 16);  // 16 bit sample
		mcStream->SetSettings(sidBigEndian, 0);      // big endian

		for (vector<string>::iterator it = list.begin(); it != list.end(); ++it)
		{
			std::streampos begin, end;
			long int bytesSent = 0,
				filesize,
				totalbytesSent = 0;

			string path = dir;
			string::size_type pos = path.find_last_of("*");
			path = path.substr(0, pos);
			path += *it;

			begin = fileToSend->tellg();
			fileToSend->seekg(0, ios::end);
			end = fileToSend->tellg();
			fileToSend->seekg(0, ios::beg);
			filesize = static_cast<long int>(end - begin);

			fileToSend->open(path, ios::binary);

			if (!fileToSend->is_open())
				continue;

			mcSocket->file = fileToSend;
			mcSocket->mcaddr = destination;
			mcSocket->socket = hSocket;
			mcSocket->filesize = filesize;
			
			// set up the multicast callback
			mcStream->SetCallbackFunc(mcCallbackFunc, (TCallbackMessage) (MsgStreamNeedMoreData | MsgWaveBuffer), 
                                      (void *) mcSocket);

            int streamBlock;    // memory block with stream data
			if (mcStream->OpenStream(1, 1, &streamBlock, 1, sfPCM) == 0)
			{
				cerr << "Error in opening a multicast stream: " << mcStream->GetError() << endl;
				mcStream->Release();
				return FALSE;
			}

            // turn down the volume
			mcStream->SetMasterVolume(0,0);

            //start streaming
			mcStream->Play();

			while (TRUE)
			{
				TStreamStatus status;
				mcStream->GetStatus(&status);
				if (status.fPlay == 0)
					break; //exit the loop
			}

			fileToSend->close();
		}

		mcStream->Release();
		free(mcSocket);
	}

	return TRUE;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    mcCallbackFunc
--
-- DATE:        March 10, 2017
--
-- REVISIONS:   (Date and Description)
--
-- DESIGNER: 
--
-- PROGRAMMER: 
--
-- INTERFACE:   int  __stdcall  mcCallbackFunc(void* instance, void *user_data, libZPlay::TCallbackMessage message, 
                                               unsigned int param1, unsigned int param2)
--              void* instance - ZPlay instance
--              void* user_data - user data specified by SetCallback
--              TCallbackMessage message - stream message. eg. fetch next song, need more streaming data, etc
--              unsigned int param1 - a pointer to buffer with PCM data
--              unsigned int param2 - number of bytes in PCM buffer
--
-- RETURNS:		0 on succeed, 1 on failure
--
-- NOTES:
-- This function will mainly listen for the MsgWaveBuffer & MsgStreamNeedMoreData message to determine when the multicast
-- thread is ready to multicast the next song.
--
----------------------------------------------------------------------------------------------------------------------*/
int  __stdcall  mcCallbackFunc(void* instance, void *user_data, libZPlay::TCallbackMessage message, 
                               unsigned int param1, unsigned int param2)
{
	ZPlay* mcStream = (ZPlay*) instance;
	LPMCSOCKET mcv = (LPMCSOCKET) user_data;
	char* buffer = new char[STREAMBUFSIZE];

	switch (message)
	{
		case MsgStreamNeedMoreData:
			mcv->file->read(buffer, TMPBUFSIZE);
			//cout << "Read " << mcv->file->gcount() << endl;
			mcStream->PushDataToStream(buffer, static_cast<unsigned int>(mcv->file->gcount()));
		break;

		case MsgWaveBuffer:
			if (sendto(mcv->socket, (const char *) param1, param2, 0,(const SOCKADDR *)& mcv->mcaddr, sizeof(mcv->mcaddr)) < 0)
			{
				cerr << "Error in sendto: " << GetLastError();
				free(mcv);
				return 1;
			}
		break;
	}
	
	delete buffer;
	return 0;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    getAudioPath
--
-- DATE:        March 12, 2017
--
-- REVISIONS:   (Date and Description)
--
-- DESIGNER:    
--
-- PROGRAMMER: 
--
-- INTERFACE:   string getAudioPath()
--
-- RETURNS:     string - the absolute path of the Audio directory
--
-- NOTES:       
-- This function returns the absolute path of the Audio directory.
----------------------------------------------------------------------------------------------------------------------*/
string getAudioPath()
{
	char buf[MAX_PATH];
	string dir;

	GetModuleFileName(NULL, buf, MAX_PATH);
	string::size_type pos = string(buf).find_last_of("\\/");
	dir = string(buf).substr(0, pos);
	dir += "\\audio\\*";

	return dir;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    populatePlayList
--
-- DATE:        March 12, 2017
--
-- REVISIONS:   (Date and Description)
--
-- DESIGNER: 
--
-- PROGRAMMER: 
--
-- INTERFACE:   int populatePlayList(vector<string>& list)
--              vector<string>& list: the vector of strings to populate
--
-- RETURNS:     return the number of songs on the list
--
-- NOTES:       
-- This function scans the Audio directory, and appends each audio file to the play list.
----------------------------------------------------------------------------------------------------------------------*/
int populatePlayList(vector<string>& list)
{
	HANDLE hFind;
	WIN32_FIND_DATA data;
	int count = 0;
	string dir;

	dir = getAudioPath();
	
	hFind = FindFirstFile(dir.c_str(), &data);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (data.cFileName[0] != '.')
			{
				count++;
				list.push_back(data.cFileName);
			}
		} while (FindNextFile(hFind, &data));

		FindClose(hFind);
	}

	return count;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    getCommand
--
-- DATE:        March 12, 2017
--
-- REVISIONS:   (Date and Description)
--
-- DESIGNER:
--
-- PROGRAMMER:
--
-- INTERFACE:   string getCommand(const int reqType)
--              string reqType: request type specified
--
-- RETURNS:     returns command mapping to the request type
--
-- NOTES:
-- returns command mapping to the request type.
----------------------------------------------------------------------------------------------------------------------*/
string getCommand(const int reqType)
{
    string cmd;

    switch (reqType) {
    case REQLIST:
        cmd = "FETCHING PLAYLIST";
        break;
    case REQSTREAM:
        cmd = "STREAMING";
        break;
    case REQDOWNLOAD:
        cmd = "DOWNLOADING";
        break;
    case REQUPLOAD:
        cmd = "UPLOADING";
        break;
    case REQMICCHAT:
        cmd = "MICROPHONE CHATTING";
        break;
    case REQMULTICAST:
        cmd = "MULTICASTING";
        break;
    default:
        break;
    }

    return cmd;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    setCursor
--
-- DATE:        March 12, 2017
--
-- REVISIONS:   (Date and Description)
--
-- DESIGNER:
--
-- PROGRAMMER:
--
-- INTERFACE:   void setCursor()
--
-- RETURNS:     void
--
-- NOTES:
-- Called to set cursor style on the window.
----------------------------------------------------------------------------------------------------------------------*/
void setCursor()
{
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 100;
    info.bVisible = FALSE;
    SetConsoleCursorInfo(consoleHandle, &info);
}