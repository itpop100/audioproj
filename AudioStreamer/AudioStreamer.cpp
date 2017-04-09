/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: AudioStreamer.cpp
--
-- PROGRAM:     AudioStreamer
--
-- FUNCTIONS:
--              int main(int argc, char * argv[])
--              DWORD WINAPI listenThread(LPVOID args)
--              DWORD WINAPI listenRequest(LPVOID param)
--              ServerState decodeRequest(char* request, string& fileName, int& fileSize)
--              void handleRequest(ServerState currentState, SOCKET clntSocket, string fileName, DWORD fileSize)
--              DWORD WINAPI mcThread(LPVOID args)
--              int  __stdcall  mcCallbackFunc(void* instance, void *user_data, libZPlay::TCallbackMessage message, 
--                                             unsigned int param1, unsigned int param2)
--              string getAudioPath()
--              int populatePlayList(vector<string>& list)
--
-- DATE:        March 10, 2017
--
-- REVISIONS:   April 8, 2017
--
-- DESIGNER:    Fred Yang, John Agapeyev, Isaac Morneau, Maitiu Morton
--
-- PROGRAMMER:  Fred Yang, John Agapeyev, Isaac Morneau, Maitiu Morton
--
-- NOTES:
-- The project uses libZPlay multimedia library. libZplay is one for playing mp3, mp2, mp1, ogg, flac, ac3, aac, 
-- oga, wav and pcm files and streams. 
-- In order for program to work, the directory for the libzplay.lib must be placed in your project folder. 
-- To link the libzplay.lib, right click project Porperties, then go to configuration properties>linker>input,
-- add "libzplay.lib" in the field "additional dependencies". 
-- libzplay.dll must also be placed into your windows/system32 and windows/syswow64.
------------------------------------------------------------------------------------------------------------------*/

#include "AudioStreamer.h"

using namespace std;
using namespace libZPlay;

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    main
--
-- DATE:        March 10, 2017
--
-- REVISIONS:   April 8, 2017
--
-- DESIGNER:    John Agapeyev, Fred Yang
--
-- PROGRAMMER:  John Agapeyev, Fred Yang
--
-- INTERFACE:   int main(int argc, char* argv[])
--
-- RETURNS:     EXIT_SUCCESS on success, EXIT_ERROR on failure
--
-- NOTES:
-- Main entry for the server side.
--
------------------------------------------------------------------------------------------------------------------*/
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
-- REVISIONS:   April 8, 2017
--
-- DESIGNER:    John Agapeyev
--
-- PROGRAMMER:  John Agapeyev
--
-- INTERFACE:   DWORD WINAPI listenThread(LPVOID params)
--              LPVOID params: points to the client socket
--
-- RETURNS:     TRUE on success, FALSE on failure
--
-- NOTES: 
-- Thread that listens for new client connection requests. It spawns a new thread to serve the client
-- once the client connected successfully.
------------------------------------------------------------------------------------------------------------------*/

DWORD WINAPI listenThread(LPVOID params)
{
    SOCKET* listenSocket = (SOCKET*) params;

    cout << "Server started, listening on socket " << *listenSocket << endl;

    while(TRUE)
    {
        SOCKADDR_IN addr = {};
        int addrLen = sizeof(addr);

        // Once listen() call has returned, the accept() call should be issued, this will block
        // until a connection request is received from a remote host.
        SOCKET clientSocket = WSAAccept(*listenSocket, (sockaddr*)&addr, &addrLen, NULL, NULL);

        if(clientSocket == INVALID_SOCKET)
        {
            if(WSAGetLastError() != WSAEWOULDBLOCK)
            {
                cerr << "accept() failed with error " << WSAGetLastError() << endl;
                break;
            }
        }
        else
        {
            cout << "Socket " << clientSocket << " accepted." << endl;
            // spawn a new thread to serve the client
            CreateThread(NULL, 0, listenRequest, (LPVOID) &clientSocket, 0, 0);
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
-- REVISIONS:   April 8, 2017
--
-- DESIGNER:    Fred Yang
--
-- PROGRAMMER:  Fred Yang
--
-- INTERFACE:   DWORD WINAPI listenRequest(LPVOID params)
--              LPVOID params: points to client socket
--
-- RETURNS:     TRUE on success, FALSE on failure
--
-- NOTES: 
-- This function listens for the connection requests from clients. Calls decodeRequest to parse the request received 
-- before handle it.
------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI listenRequest(LPVOID params)
{
    SOCKET clntSocket = *((SOCKET*) params);
    int bytesReceived, bytesToRead;
    char request[MAXBUFSIZE];
    char* req;
    string fileName;
    DWORD fileSize;
    ServerState newState = STATEIDLE, prevState = STATEIDLE;

    while (TRUE)
    {
        memset(request, 0, MAXBUFSIZE);
        req = request;
        bytesToRead = MAXBUFSIZE;

        // continuously handle bytes received
        while ((bytesReceived = recv(clntSocket, req, bytesToRead, 0)) > 0)
        {
            req += bytesReceived;
            bytesToRead -= bytesReceived;

            if (bytesReceived <= MAXBUFSIZE)
                break;
        }

        // no bytes received
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
        if (prevState != STATEIDLE)
            prevState = newState;

        //get the new current state
        newState = decodeRequest(request, fileName, fileSize);

        if (newState == STATEERR)
            break;

        // call handleRequest
        handleRequest(newState, clntSocket, fileName, fileSize);
    }

    return TRUE;
}


/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    decodeRequest
--
-- DATE:        March 10, 2017
--
-- REVISIONS:   April 8, 2017
--
-- DESIGNER:    John Agapeyev
--
-- PROGRAMMER:  John Agapeyev
--
-- INTERFACE:   ServerState decodeRequest(char* request, string& fileName, DWORD& fileSize)
--              request - the request packet to parse
--              fileName - the name of the file
--              fileSize - the size of the file
--
-- RETURNS: return ServerState which will indicate one of the specified ServerStates (STREAMING, DOWNLOADING, etc.),
--          return STATEERR on failure.
--
-- NOTES: 
-- This function parses a request and returns the current state of the server.
------------------------------------------------------------------------------------------------------------------*/
ServerState decodeRequest(char* request, string& fileName, DWORD& fileSize)
{
    stringstream ss(request);
    int reqType = REQIDLE;
    ServerState state = STATEIDLE;

    if (ss >> reqType)
    cout << getCommand(reqType) << ">>>>>";

    // handle various types of requests
    switch (reqType) {
    case REQLIST:   // list request
        state = STATELIST;
        break;
    case REQSTREAM: // streaming request
        state = STATESTREAMING;
        getline(ss, fileName);
        cout << fileName << endl;
        break;
    case REQDOWNLOAD: // download request
        state = STATEDOWNLOADING;
        getline(ss, fileName);
        cout << fileName << endl;
        break;
    case REQUPLOAD:  // upload request
        state = STATEUPLOADING;
        ss >> fileSize;
        getline(ss, fileName);
        cout << fileName << " (" << fileSize << " b)" << endl;
        break;
    case REQMICCHAT: // microphone chat request
        state = STATEMICCHATTING;
        cout << "Starting 2 way chatting" << endl;
        break;
    case REQMULTICAST: // join multicast group request
        state = STATEMULTICASTING;
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
-- REVISIONS:   April 8, 2017
--
-- DESIGNER:    Fred Yang
--
-- PROGRAMMER:  Fred Yang
--
-- INTERFACE:   void handleRequest(const ServerState& currentState, SOCKET clntSocket, 
--                                 string fileName, DWORD fileSize)
--              currentState - the current state of the server
--              clntSocket - the socket of the client to send
--              fileName - the name of the file being transferred
--              fileSize - size of the file being transferred
--
-- RETURNS:     void
--
-- NOTES:   
-- This function keeps track of the current and previous server state, then executes the steps to handle the current
-- request.
------------------------------------------------------------------------------------------------------------------*/
void handleRequest(const ServerState& currentState, SOCKET clntSocket, string fileName, DWORD fileSize)
{
    char*           tmp;
    int             bytesSent = 0,          // bytes sent
                    bytesReceived = 0;      // bytes received
    int             totalbytesSent = 0,     // total bytes sent
                    totalbytesReceived = 0; // total bytes received
    string          line;                   // line to hold the file name
    ifstream        fileToSend;             // file read stream
    ofstream        fileReceived;           // file write stream
    streamsize      bytesRead;              // bytes to read
    vector<string>  list;                   // play list
    int             count;                  // number of songs
    DWORD           fsize = 0;
    ostringstream oss;
    streampos begin, end;

    switch (currentState)
    {
        case STATELIST:  // listing
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

            // send EOT denoting end of the list
            line = EOT;
            send(clntSocket, line.c_str(), line.size(), 0);
            cout << "Play list sent" << endl;
            break;

        case STATESTREAMING: // streaming
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
                tmp = new char[DATABUFSIZE];

                bytesRead = 0;
                fileToSend.read(tmp, DATABUFSIZE);
                
                // send data to the socket
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

        case STATEDOWNLOADING: // downloading
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
                tmp = new char [MAXBUFSIZE];

                bytesRead = 0;
                fileToSend.read(tmp, MAXBUFSIZE);
                
                // send data to the socket
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

        case STATEUPLOADING: // uploading
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
                tmp = new char[MAXBUFSIZE];
                memset(tmp, 0, MAXBUFSIZE);

                // read data from the socket
                if (((bytesReceived = recv(clntSocket, tmp, MAXBUFSIZE, 0)) == 0) || (bytesReceived == -1))
                {
                    cerr << "recv failed with error " << GetLastError() << endl;
                    cout << "Ending upload session..." << endl;
                    fileReceived.close();
                    delete[] tmp;
                    return;
                }

                // save the data received into a file
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

        case STATEMICCHATTING: // microphone chatting
            cout << "Mic session started..." << endl;
            startMicChat();
        break;

        case STATEMULTICASTING: // multicasting
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
-- REVISIONS:   April 8, 2017
--
-- DESIGNER:    John Agapeyev, Fred Yang
--  
-- PROGRAMMER:  John Agapeyev, Fred Yang
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
------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI mcThread(LPVOID params)
{
    char            mcAddr[ADDRSIZE] = MULTICAST_ADDR;   // multicast address
    u_short         nPort = MULTICAST_PORT;              // multicast port
    u_long          mcTTL = MULTICAST_TTL;               // multicast TTL
    DWORD           result,                              // result
                    count;                               // number of songs
    BOOL            flag = false;
    SOCKADDR_IN     server,
                    destination;

    struct ip_mreq  stMreq;     // struct for IP_MREQ (IP_ADD_MEMBERSHIP, IP_DROP_MEMBERSHIP)
    SOCKET          stSocket;   // socket to create
    WSADATA         stWSAData;
    vector<string>  list;       // play list

    string          dir,        // audio absolute path
                    line;       // line to hold the file name
    ifstream*       fileToSend; // file read stream
    fileToSend = new ifstream;

    // multicast socket information struct
    LPMCSOCKET mcSocket = (LPMCSOCKET) malloc(sizeof(MCSOCKET));

    result = WSAStartup(0x0202, &stWSAData);
    if (result)
    {
        cerr << "WSAStartup failed: " << result << endl;
        return FALSE;
    }

     // create socket given socket type and protocol
    stSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (stSocket == INVALID_SOCKET)
    {
        cerr << "socket() failed: " << WSAGetLastError() << endl;
        return FALSE;
    }

    // set socket structure
    server.sin_family      = AF_INET; 
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port        = 0;

    // bind to the socket
    result = bind(stSocket, (struct sockaddr*)&server, sizeof(server));
    if (result == SOCKET_ERROR) 
    {
        cerr << "bind() port: " << nPort << " failed: " << WSAGetLastError() << endl;
        return FALSE;
    }

    stMreq.imr_multiaddr.s_addr = inet_addr(mcAddr);
    stMreq.imr_interface.s_addr = INADDR_ANY;

    // Use the IP_ADD_MEMBERSHIP option to join an IPv4 multicast group on a local IPv4 interface.
    // Use the SETSOCKOPT API and specify the address of the IP_MREQ structure that contains these addresses.
    result = setsockopt(stSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&stMreq, sizeof(stMreq));
    if (result == SOCKET_ERROR)
    {
        cerr << "setsockopt() IP_ADD_MEMBERSHIP address " << mcAddr << " failed: " << WSAGetLastError() << endl;
        return FALSE;
    }

    // Set TTL up to specified number of routes to pass through
    result = setsockopt(stSocket, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&mcTTL, sizeof(mcTTL));
    if (result == SOCKET_ERROR)
    {
        cerr << "setsockopt() IP_MULTICAST_TTL failed: " << WSAGetLastError() << endl;
        return FALSE;
    }

    //  no need to be looped back to your host
    result = setsockopt(stSocket, IPPROTO_IP,  IP_MULTICAST_LOOP, (char *)&flag, sizeof(flag));
    if (result == SOCKET_ERROR)
    {
        cerr << "setsockopt() IP_MULTICAST_LOOP failed: " << WSAGetLastError() << endl;
        return FALSE;
    }

    // set destination socket structure
    destination.sin_family =      AF_INET;
    destination.sin_addr.s_addr = inet_addr(mcAddr);
    destination.sin_port =        htons(nPort);

    dir = getAudioPath();
    count = populatePlayList(list);

    if (count > 0)
    {
        // Open libzplay stream and settings
        ZPlay * mcStreamPlayer = CreateZPlay();
        mcStreamPlayer->SetSettings(sidSamplerate, SAMPLERATE);
        mcStreamPlayer->SetSettings(sidChannelNumber, CHANNELNUM);
        mcStreamPlayer->SetSettings(sidBitPerSample, BITPERSAMPLE);
        mcStreamPlayer->SetSettings(sidBigEndian, LITTLEEND);

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

            fileToSend->open(path, ios::binary);

            if (!fileToSend->is_open())
                continue;

            // read bytes from the file
            begin = fileToSend->tellg();
            fileToSend->seekg(0, ios::end);
            end = fileToSend->tellg();
            fileToSend->seekg(0, ios::beg);
            filesize = static_cast<long int>(end - begin);

            // add bytes to the socket
            mcSocket->file = fileToSend;
            mcSocket->mcaddr = destination;
            mcSocket->socket = stSocket;
            mcSocket->filesize = filesize;
            
            // set up the multicast callback
            mcStreamPlayer->SetCallbackFunc(mcCallbackFunc, (TCallbackMessage) (MsgStreamNeedMoreData | MsgWaveBuffer), 
                                      (void *) mcSocket);

            int streamBlock;    // memory block with stream data
            if (mcStreamPlayer->OpenStream(1, 1, &streamBlock, 1, sfPCM) == 0)
            {
                cerr << "Error in opening a multicast stream: " << mcStreamPlayer->GetError() << endl;
                mcStreamPlayer->Release();
                return FALSE;
            }

            // turn down the volume
            mcStreamPlayer->SetMasterVolume(0, 0);

            // start streaming
            mcStreamPlayer->Play();

            while (TRUE)
            {
                TStreamStatus status;
                mcStreamPlayer->GetStatus(&status);
                if (status.fPlay == 0)
                    break;

                // Retrieve current position in TStreamTime format. 
                // If stream is not playing or stream is closed, position is 0.
                /*TStreamTime pos;
                mcStreamPlayer->GetPosition(&pos);
                cout << "Pos: " << pos.hms.hour << ":" << pos.hms.minute << ":"
                    << pos.hms.second << "." << pos.hms.millisecond << endl;*/

            }

            fileToSend->close();
        }

        mcStreamPlayer->Release();
        free(mcSocket);
    }

    return TRUE;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    mcCallbackFunc
--
-- DATE:        March 10, 2017
--
-- REVISIONS:   April 8, 2017
--
-- DESIGNER:    John Agapeyev, Fred Yang
--
-- PROGRAMMER:  John Agapeyev, Fred Yang
--
-- INTERFACE:   int  __stdcall  mcCallbackFunc(void* instance, void *user_data, libZPlay::TCallbackMessage message, 
                                               unsigned int param1, unsigned int param2)
--              void* instance - ZPlay instance
--              void* user_data - user data specified by SetCallback
--              TCallbackMessage message - stream message. eg. fetch next song, need more streaming data, etc
--              unsigned int param1 - a pointer to buffer with PCM data
--              unsigned int param2 - number of bytes in PCM buffer
--
-- RETURNS:     0 on succeed, 1 on failure
--
-- NOTES:
-- This function will mainly listen for the MsgWaveBuffer & MsgStreamNeedMoreData message to determine when the
-  multicast thread is ready to multicast the next song.
--
------------------------------------------------------------------------------------------------------------------*/
int  __stdcall  mcCallbackFunc(void* instance, void *user_data, libZPlay::TCallbackMessage message, 
                               unsigned int param1, unsigned int param2)
{
    ZPlay* mcStreamPlayer = (ZPlay*) instance;
    LPMCSOCKET udata = (LPMCSOCKET) user_data;
    char* buffer = new char[MAXBUFSIZE];

    switch (message)
    {
        case MsgStreamNeedMoreData:
            udata->file->read(buffer, sizeof(buffer));
            mcStreamPlayer->PushDataToStream(buffer, static_cast<unsigned int>(udata->file->gcount()));
        break;

        case MsgWaveBuffer:
            if (sendto(udata->socket, (const char *) param1, param2, 0,(const SOCKADDR *)& udata->mcaddr, 
                sizeof(udata->mcaddr)) < 0)
            {
                cerr << "Error in sendto: " << GetLastError();
                free(udata);
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
-- REVISIONS:   April 8, 2017
--
-- DESIGNER:    Isaac Morneau
--
-- PROGRAMMER:  Isaac Morneau
--
-- INTERFACE:   string getAudioPath()
--
-- RETURNS:     string - the absolute path of the Audio directory
--
-- NOTES:       
-- This function returns the absolute path of the Audio directory.
------------------------------------------------------------------------------------------------------------------*/
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
-- REVISIONS:   April 5, 2017
--
-- DESIGNER:    Maitiu Morton
--
-- PROGRAMMER:  Maitiu Morton
--
-- INTERFACE:   int populatePlayList(vector<string>& list)
--              vector<string>& list: the vector of strings to populate
--
-- RETURNS:     return the number of songs on the list
--
-- NOTES:       
-- This function scans the Audio directory, and appends each audio file to the play list.
------------------------------------------------------------------------------------------------------------------*/
int populatePlayList(vector<string>& list)
{
    int count = 0;
    HANDLE hFind;
    WIN32_FIND_DATA data;
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
-- REVISIONS:   April 8, 2017
--
-- DESIGNER:    Fred Yang
--
-- PROGRAMMER:  Fred Yang
--
-- INTERFACE:   string getCommand(const int reqType)
--              string reqType: request type specified
--
-- RETURNS:     returns command mapping to the request type
--
-- NOTES:
-- returns command mapping to the request type.
------------------------------------------------------------------------------------------------------------------*/
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
-- DESIGNER:    John Agapeyev
--
-- PROGRAMMER:  John Agapeyev
--
-- INTERFACE:   void setCursor()
--
-- RETURNS:     void
--
-- NOTES:
-- Called to set cursor style on the window.
------------------------------------------------------------------------------------------------------------------*/
void setCursor()
{
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 100;
    info.bVisible = FALSE;
    SetConsoleCursorInfo(consoleHandle, &info);
}
