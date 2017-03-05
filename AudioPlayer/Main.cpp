/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE:     Main.cpp -  This file contains the implementation of the client GUI.
--
-- PROGRAM:         AudioPlayer
--
-- FUNCTIONS:       bool renderUI(HWND hWnd);
--                  bool downloadRequest(AudioPlayer&);
--                  bool uploadRequest(AudioPlayer& clnt, HWND hWnd, OPENFILENAME &ofn);
--                  bool streamRequest(AudioPlayer&);
--                  bool micRequest(AudioPlayer&);
--                  bool mcRequest(AudioPlayer&);
--                  bool listRequest(AudioPlayer&, HWND*) ;
--                  bool createMicSocket();
--                  bool startMicChat();
--                  bool populatePlayList(HWND, std::vector<std::string>);
--                  void openFileDialog(HWND, OPENFILENAME&);
--                  std::vector<std::string> processPlayList(std::string);
--                  std::string getSelectedListBoxItem(HWND* , int );
--                  std::string getFileName(std::string);
--                  SOCKET createMCSocket();
--                  DWORD WINAPI mcThread(LPVOID args);
--                  DWORD WINAPI listThread(LPVOID args);
--                  DWORD WINAPI micThread(LPVOID param);
--                  LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
--                  int __stdcall micCallBackFunc(void* instance, void* user_data, libZPlay::TCallbackMessage message, 
--                                                unsigned int param1, unsigned int param2);
-- 
-- DATE:            March 9, 2017
--
-- REVISIONS: 
--
-- DESIGNER:
--
-- PROGRAMMER:
--
-- NOTES:
-- The project uses libZPlay multimedia library. libZplay is one for playing mp3, mp2, mp1, ogg, flac, ac3, aac, 
-- oga, wav and pcm files and streams. 
-- In order for program to work, the directory for the libzplay.lib must be placed in the project folder. 
-- To link the libzplay.lib, right click project Porperties, then go to configuration properties>linker>input,
-- add "libzplay.lib" in the field "additional dependencies". 
-- libzplay.dll must also be placed into your windows/system32 and windows/syswow64.
----------------------------------------------------------------------------------------------------------------------*/

#include "Main.h"

using namespace std;
using namespace libZPlay;

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    WinMain
--
-- DATE:        March 8, 2017
--
-- REVISIONS:   
--
-- DESIGNER:    
--
-- PROGRAMMER:  
--
-- INTERFACE:   int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int nCmdShow)
--              HINSTANCE hInst: Handle to the current instance of the program.
--              HINSTANCE hPrevInstance: Handle to the previous instance of the program.
--              LPSTR lspszCmdParam: Command line for the application.
--              int nCmdShow: Control of how the window should be shown (minimized, maximized, or shown normally).
--
-- RETURNS:     Returns the exit value upon exit.
--
-- NOTES:
-- This function is the entry point for a graphical Windows-based application.
----------------------------------------------------------------------------------------------------------------------*/
int WINAPI WinMain(HINSTANCE hInst,HINSTANCE hPrevInst,LPSTR lpCmdLine,int nCmdShow)
{
    // Hold message(s) from message queue
    MSG msg;

    // Window class which ontains the window class attributes
    // that are registered by the RegisterClass function
    WNDCLASSEX wClass;
    ZeroMemory(&wClass,sizeof(WNDCLASSEX));
    wClass.cbClsExtra = NULL;
    wClass.cbSize = sizeof(WNDCLASSEX);
    wClass.cbWndExtra = NULL;
    wClass.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wClass.hCursor = LoadCursor(NULL,IDC_ARROW);
    wClass.hIcon = NULL;
    wClass.hIconSm = NULL;
    wClass.hInstance = hInst;
    wClass.lpfnWndProc = (WNDPROC)WndProc;
    wClass.lpszClassName = "Window Class";
    wClass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1) ;
    wClass.style = CS_HREDRAW|CS_VREDRAW;

    if (!RegisterClassEx(&wClass))
    {
        int nResult = GetLastError();
        MessageBox(NULL,
            "Window class creation failed\r\nError code:",
            "Window Class Failed",
            MB_ICONERROR);
    }

    ghWnd = CreateWindowEx(NULL, "Window Class", szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, SCREEN_W, SCREEN_H, NULL, NULL, hInst, NULL);

    if (!ghWnd)
    {
        int nResult = GetLastError();

        MessageBox(NULL,
            "Window creation failed\r\nError code:",
            "Window Creation Failed",
            MB_ICONERROR);
    }

    ShowWindow(ghWnd, nCmdShow);
    UpdateWindow(ghWnd);

    while (GetMessage(&msg,NULL,0,0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    WndProc
--
-- DATE:        March 8, 2017
--
-- REVISIONS: 
--
-- DESIGNER: 
--
-- PROGRAMMER: 
--
-- INTERFACE:   LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
--              HWND hWnd: a handle to the window.
--              UNIT Message: system-defined message when it communicates with an application.
--              WPARAM wParam: additional message information. The contents of this parameter
--                             depend on the value of the uMsg parameter.
--              LPARAM lParam: additional message information. The contents of this parameter
--                             depend on the value of the uMsg parameter.
--
-- RETURNS:     Result of the message processing.
--
-- NOTES:
-- Main callback function that processes messages sent to a window.
----------------------------------------------------------------------------------------------------------------------*/
LRESULT CALLBACK WndProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    static OPENFILENAME ofn = {0};  // common dialog box structure to open a file
    static HANDLE hFile;            // handle to file
    static AudioPlayer clnt;        // client instance
    HMENU hMenu;                    // handle to menu
    HDC   hdc;                      // handle to the device context
    PAINTSTRUCT  ps;                // struct used to paint the client area of a window

    switch(msg)
    {
        case WM_CREATE:
        {
            // render GUI
            if (!renderUI(hWnd)) {
                PostQuitMessage(1);
            }

            connected = false;
            break;
        }
        case WM_CTLCOLORSTATIC: // static controls color settings
        {
            DWORD ctlId = GetDlgCtrlID((HWND)lParam);
            if (ctlId != IDC_PLAYLIST && ctlId != IDC_TRACELIST) {
                HDC hdcStatic = (HDC)wParam;
                SetTextColor(hdcStatic, RGB(102, 0, 204));
                SetBkColor(hdcStatic, RGB(238, 238, 238));
                hbrBackground = CreateSolidBrush(RGB(238, 238, 238));
                return (INT_PTR)hbrBackground;
            }
            break;
        }
        case WM_CTLCOLORLISTBOX: // listbox controls color settings
        {
            DWORD ctlId = GetDlgCtrlID((HWND)lParam);
            if (ctlId != IDC_TOOL_HELP) {
                HDC hdcStatic = (HDC)wParam;
                SetTextColor(hdcStatic, RGB(102, 0, 204));
                hbrBackground = CreateSolidBrush(RGB(204, 255, 204));
                return (INT_PTR)hbrBackground;
            }
            break;
        }
        case WM_COMMAND:
            hMenu = GetMenu (hWnd) ;
            hdc = GetDC(hWnd);
            SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));

            switch(LOWORD(wParam))
            {
            case ID_FILE_EXIT:
                {
                    // quit
                    PostQuitMessage(0);
                }
                break;

            case ID_FILE_CONNECT:
                {
                    if (!connected)
                    {
                        // get user input: hostname & port
                        SendMessage(GetDlgItem(hWnd,IDC_EDIT_PORT), WM_GETTEXT,sizeof(szPort)/sizeof(szPort[0]),(LPARAM)szPort);
                        SendMessage(GetDlgItem(hWnd,IDC_EDIT_HOSTNAME), WM_GETTEXT,sizeof(szServer)/sizeof(szServer[0]),(LPARAM)szServer);

                        WSADATA wsaData;
                        if (clnt.runClient(&wsaData, szServer, atoi(szPort)))
                        {
                            SendMessage(GetDlgItem(hWnd,IDC_MAIN_STATUS), SB_SETTEXT, STATUSBAR_STATUS, (LPARAM)"Connected");
                            EnableWindow(GetDlgItem(hWnd,IDC_BUTTON_OK), TRUE); 
                            EnableWindow(GetDlgItem(hWnd, IDC_EDIT_PORT), FALSE);
                            EnableWindow(GetDlgItem(hWnd, IDC_EDIT_HOSTNAME), FALSE);
                            connected = true;

                            listRequest(clnt,&hWnd);

                        }
                        else
                            MessageBox(hWnd, "Connect failed!" , "Warning" , MB_ICONWARNING);
                    }
                    else
                    {
                        MessageBox(hWnd, "Already connected as a client!" , "Warning" , MB_ICONWARNING);
                    }

                }
                break;

            case ID_FILE_DISCONNECT:
            {
                // disconnect
            }
            break;

            case ID_HELP_ABOUT:
            {
                MessageBox(hWnd, szTitle, "About", MB_ICONINFORMATION|MB_OK);
            }
            break;

            case IDC_BUTTON_PLAY:
            case IDC_BUTTON_REWIND:
            {
                playAudio(hWnd, clnt);
            }
            break;

            case IDC_BUTTON_FORWARD:
            case IDC_BUTTON_STOP:
            {
                stopAudio(clnt);
            }
            break;

            case IDC_BUTTON_PAUSE:
            {
                pauseAudio(clnt);
            }
            break;

            case IDC_BUTTON_OK:
            {
                if (IsDlgButtonChecked(hWnd, IDC_RADIO_DOWNLOAD) == BST_CHECKED )
                {
                    if (clnt.currentState != WAITFORCOMMAND)
                    {
                        MessageBox(hWnd, szWarn , "Warning" , MB_ICONWARNING);
                    }
                    else {
                        clnt.currentAudioFile = getSelectedListBoxItem(&hWnd, IDC_PLAYLIST);

                        if (clnt.currentAudioFile != "ERROR")
                            downloadRequest(clnt);
                    }
                    break;
                }
                else if (IsDlgButtonChecked(hWnd, IDC_RADIO_UPLOAD) == BST_CHECKED)
                {
                    if (clnt.currentState != WAITFORCOMMAND)
                    {
                        MessageBox(hWnd, szWarn , "Warning" , MB_ICONWARNING);
                    }
                    else {
                        uploadRequest(clnt, hWnd, ofn);
                        
                        DWORD result = WaitForSingleObject(clnt.ulThreadHandle, INFINITE);
                        if (result == WAIT_OBJECT_0)
                            listRequest(clnt, &hWnd);
                    }
                    break;
                }
                else if (IsDlgButtonChecked(hWnd, IDC_RADIO_MIC) == BST_CHECKED)
                {
                    if (clnt.currentState != WAITFORCOMMAND)
                    {
                        MessageBox(hWnd, szWarn , "Warning" , MB_ICONWARNING);
                    }
                    else {
                        micRequest(clnt);
                    }
                    break;
                }
                else if (IsDlgButtonChecked(hWnd, IDC_RADIO_MULTICAST) == BST_CHECKED)
                {
                    if (clnt.currentState != WAITFORCOMMAND)
                    {
                        MessageBox(hWnd, szWarn , "Warning" , MB_ICONWARNING);
                    }
                    else {
                        mcRequest(clnt);
                    }
                    break;
                }
            }
            break;

            case IDC_BUTTON_CANCEL:
            {
                closeAudio(hWnd, clnt);
            }
            break;
        }
        break;

    case WM_PAINT:
        {
            hdc = BeginPaint(hWnd, &ps); // acquire DC
            SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));
            SetBkMode(hdc, TRANSPARENT);
            ReleaseDC(hWnd, hdc); // release DC
            EndPaint(hWnd, &ps);
        }
        break;

    case WM_DESTROY:
        {
            // quit
            PostQuitMessage(0);
            WSACleanup();
            return 0;
        }
        break;

    case WM_SIZE:
        {
            // dynamically resize status bar
            SendMessage(GetDlgItem(hWnd, IDC_MAIN_STATUS), WM_SIZE, 0, 0);
        }
        break;
    }

    return DefWindowProc(hWnd,msg,wParam,lParam);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    getFileName
--
-- DATE:        March 10, 2017
--
-- REVISIONS: 
--
-- DESIGNER: 
--
-- PROGRAMMER: 
--
-- INTERFACE: string getFileName(string path)
--            string path: absolute path of a file
--
-- RETURNS:   filename and extension
--
-- NOTES:
-- Called to retrieve filename given absolute path.
----------------------------------------------------------------------------------------------------------------------*/
string getFileName(string path)
{
    int i = path.find_last_of('\\');
    if (i != std::string::npos) {
        path = path.substr(i + 1);
    }

    return path;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    listThread
--
-- DATE:        March 10, 2017
--
-- REVISIONS:	
--
-- DESIGNER:	
--
-- PROGRAMMER:	
--
-- INTERFACE:   DWORD WINAPI listThread (LPVOID params)
--              LPVOID params: points to UPLOADCONTEXT struct
--
-- RETURNS:     returns TRUE if no error occurs; otherwise returns FALSE
--
-- NOTES:
-- A thread to fetch play list from server, then populate in list box.
--
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI listThread (LPVOID params)
{
    UPLOADCONTEXT* uc = (UPLOADCONTEXT*) params;  // upload context
    AudioPlayer* clnt = uc->clnt;   // client instance
    string userReq = uc->userReq;   // user request
    
    clnt->currentState = SENTLISTREQUEST;
    clnt->dispatchSend(userReq);

    while (TRUE)
    {
        if (clnt->currentState != WAITFORLIST)
        {
            // operation completed
            if (clnt->currentState == WAITFORCOMMAND)
            {
                break;
            }

            continue;
        }

        // dispatch async receive request
        clnt->dispatchRecv();
    }

    // fetch play list from the server
    clnt->localPlayList.erase(clnt->localPlayList.begin(), clnt->localPlayList.end());
    clnt->localPlayList = processPlayList(clnt->cachedPlayList.substr(0,clnt->cachedPlayList.size()-1));
    clnt->cachedPlayList.clear();

    if (clnt->localPlayList.size() > 0)
    {
        populatePlayList(hPlayList, clnt->localPlayList); // populate play list
        return TRUE;
    }

    return FALSE;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    uploadRequest
--
-- DATE:        March 10, 2017
--
-- REVISIONS:
--
-- DESIGNER:
--
-- PROGRAMMER:
--
-- INTERFACE:   uploadRequest(AudioPlayer& clnt, HWND hWnd, OPENFILENAME& ofn)
--              AudioPlayer& clnt: client object that connects to server
--              HWND hWnd: handle to main window
--              OPENFILENAME& ofn: common dialog to open a file
--
-- RETURNS:     returns TRUE if no error occurs; otherwise returns FALSE
--
-- NOTES:
-- Request to upload an audio file.
--
----------------------------------------------------------------------------------------------------------------------*/
bool uploadRequest(AudioPlayer& clnt, HWND hWnd, OPENFILENAME &ofn)
{
    openFileDialog(hWnd, ofn);

    if (GetOpenFileName(&ofn))
    {
        clnt.currentAudioFile = getFileName(ofn.lpstrFile);
        clnt.ulThreadHandle = CreateThread(NULL, 0, clnt.runULThread, &clnt, 0, &clnt.ulThreadId);
        return TRUE;
    }

    return FALSE;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    downloadRequest
--
-- DATE:        March 11, 2017
--
-- REVISIONS:
--
-- DESIGNER:
--
-- PROGRAMMER:
--
-- INTERFACE:   downloadRequest(AudioPlayer &clnt)
--              AudioPlayer& clnt: client object that connects to server
--
-- RETURNS:     returns TRUE if no error occurs; otherwise returns FALSE
--
-- NOTES:
-- Request to download an audio file from the server.
--
----------------------------------------------------------------------------------------------------------------------*/
bool downloadRequest(AudioPlayer &clnt)
{
    clnt.dlThreadHandle = CreateThread(NULL, 0, clnt.runDLThread, &clnt, 0, &clnt.dlThreadId);

    return TRUE;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    listRequest
--
-- DATE:        March 10, 2017
--
-- REVISIONS:	
--
-- DESIGNER:	
--
-- PROGRAMMER:	
--
-- INTERFACE:   bool listRequest(AudioPlayer& clnt, HWND* hWnd)
--              AudioPlayer& clnt: client object that connects to server
--              HWND hWnd: handle to main window
--
-- RETURNS:     returns TRUE if no error occurs; otherwise returns FALSE
--
-- NOTES:
-- Fetch song list from server and populate in the list box.
--
----------------------------------------------------------------------------------------------------------------------*/
bool listRequest(AudioPlayer& clnt, HWND* hWnd)
{
    UPLOADCONTEXT* uc = new UPLOADCONTEXT;
    uc->clnt = &clnt;
    uc->userReq = to_string(REQLIST);

    HANDLE listThreadHandle = CreateThread(NULL, 0, listThread, uc, 0, NULL);

    return TRUE;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    streamRequest
--
-- DATE:        March 11, 2017
--
-- REVISIONS:	
--
-- DESIGNER:	
--
-- PROGRAMMER:	
--
-- INTERFACE:   bool streamRequest(AudioPlayer& clnt)
--              AudioPlayer& clnt: client object that connects to server
--
-- RETURNS:     returns TRUE if no error occurs; otherwise returns FALSE
--
-- NOTES:
-- Called to request streaming data from the server.
--
----------------------------------------------------------------------------------------------------------------------*/
bool streamRequest(AudioPlayer& clnt)
{
    clnt.stThreadHandle = CreateThread(NULL, 0, clnt.runSTThread, &clnt, 0, &clnt.stThreadId);
    return TRUE;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    micRequest
--
-- DATE:        March 11, 2017
--
-- REVISIONS:	
--
-- DESIGNER:	
--
-- PROGRAMMER:	
--
-- INTERFACE:   bool micRequest(AudioPlayer& clnt)
--              AudioPlayer& clnt: client object that connects to server
--
-- RETURNS:     returns TRUE if no error occurs; otherwise returns FALSE
--
-- NOTES:
-- Called to start the microphone chat thread.
--
----------------------------------------------------------------------------------------------------------------------*/
bool micRequest(AudioPlayer& clnt)
{
    UPLOADCONTEXT* uc = new UPLOADCONTEXT;
    uc->clnt = &clnt;
    uc->userReq = to_string(REQMICCHAT);

    HANDLE hMicSessionThread = CreateThread(NULL, 0, micThread, uc, 0, NULL);

    return TRUE;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    micThread
--
-- DATE:        March 11, 2017
--
-- REVISIONS:	
--
-- DESIGNER:	
--
-- PROGRAMMER:	
--
-- INTERFACE:   DWORD WINAPI micThread(LPVOID param)
--              LPVOID param: upload context specified
--
-- RETURNS:     TRUE on success, FALSE on failure.
--
-- NOTES:
-- A thread that starts the microphone chat session.
--
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI micThread(LPVOID param)
{

    UPLOADCONTEXT* uc = (UPLOADCONTEXT*) param;
    AudioPlayer* clnt = uc->clnt;
    string userReq = uc->userReq;
    
    clnt->currentState = MICROPHONE;
    clnt->dispatchSend(userReq);

    if(!createMicSocket())
    {
        clnt->currentState = WAITFORCOMMAND;
        return FALSE;
    }

    // start microphone chat session
    startMicChat();
    clnt->currentState = WAITFORCOMMAND;

    return TRUE;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    startMicChat
--
-- DATE:        March 11, 2017
--
-- REVISIONS:	
--
-- DESIGNER:	
--
-- PROGRAMMER:	
--
-- INTERFACE:   bool startMicChat()
--
-- RETURNS: TRUE on success, FALSE on failure
--
-- NOTES:
-- This function starts the microphone chat session and transfers data between the 
-- server and client.
--
----------------------------------------------------------------------------------------------------------------------*/
bool startMicChat()
{
    // stream player & settings
    streamPlayer = CreateZPlay();
    streamPlayer->SetSettings(sidSamplerate, SAMPLERATE);
    streamPlayer->SetSettings(sidChannelNumber, CHANNELNUM);
    streamPlayer->SetSettings(sidBitPerSample, BITPERSAMPLE);
    streamPlayer->SetSettings(sidBigEndian, BIGEND);

    int bytesReceived;  // bytes received
    int streamBlock;    // memory block with stream data
    int result = streamPlayer->OpenStream(1, 1, &streamBlock, 1, sfPCM); 

    if(result == 0) {
        printf("Error: %s\n", streamPlayer->GetError());
        streamPlayer->Release();
        closesocket(micSocket);
        return FALSE;
    }

    // microphone player to open microphone device & collect sounds
    micPlayer = CreateZPlay();
    result = micPlayer->OpenFile("wavein://", sfAutodetect);

    if(result == 0) {
        printf("Error: %s\n", micPlayer->GetError());
        micPlayer->Release();
        closesocket(micSocket);
        return FALSE;
    }

    // set callback function whenever the microphone collects a sound
    micPlayer->SetCallbackFunc(micCallBackFunc, (TCallbackMessage)(MsgWaveBuffer|MsgStop), NULL);
    // start listening to mic
    micPlayer->Play();

    while(TRUE) {
        char * buffer = new char[MAXBUFSIZE];
        int size = sizeof(micServer);

        if (bytesReceived = recvfrom(micSocket, buffer, MAXBUFSIZE, 0, (sockaddr*)&micServer, &size) == SOCKET_ERROR)
        {
            int errNo = WSAGetLastError();

            if (errNo == 10054)
            {
                printf("Connection reset by peer.\n");
            }
            else { 
                printf("get last error %d\n", errNo); 
            }

            closesocket(micSocket);
            break;
        }

        //  push raw data to stream
        streamPlayer->PushDataToStream(buffer, result);
        delete buffer;

        // play the stream
        streamPlayer->Play();

        // detect the status of the stream
        TStreamStatus status;
        micPlayer->GetStatus(&status);

        // if not playing, then release it 
        if(status.fPlay == 0)
            break; 

        //  retrieve current position in TStreamTime format. 
        // If stream is not playing or stream is closed, position is 0.
        TStreamTime pos;
        micPlayer->GetPosition(&pos);
    }

    micPlayer->Release();
    return TRUE;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    createMicSocket
--
-- DATE:        March 11, 2017
--
-- REVISIONS:	
--
-- DESIGNER:	
--
-- PROGRAMMER:	
--
-- INTERFACE:   bool createMicSocket() 
--
-- RETURNS:     TRUE on success, FALSE on failure
--
-- NOTES:
-- This function creates a UDP socket for the mictophone session
--
----------------------------------------------------------------------------------------------------------------------*/
bool createMicSocket() {
    // contains information about the Windows Sockets implementation.
    WSADATA wsaData;

    // The highest version of Windows Sockets spec that the caller can use
    WORD wVersionRequested = MAKEWORD(2,2);

    // Open up a Winsock session
    WSAStartup(wVersionRequested, &wsaData);

    // set sockaddr structure
    struct hostent  *hp;
    memset((char *)&micServer, 0, sizeof(struct sockaddr_in));
    micServer.sin_family = AF_INET;
    micServer.sin_port = htons(UDPPORT);

    if ((hp = gethostbyname(szServer)) == NULL)
    {
        MessageBox(NULL, "Unknown server address", NULL, NULL);
        closesocket(micSocket);
        return false;
    }

    memcpy((char *)&micServer.sin_addr, hp->h_addr, hp->h_length);
    micSocket = socket(AF_INET, SOCK_DGRAM, 0);
    memset((char *)&micClient, 0, sizeof(micClient));
    micClient.sin_family = AF_INET;
    micClient.sin_port = htons(0);
    micClient.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind to the socket
    if (bind(micSocket, (struct sockaddr *)&micClient, sizeof(micClient)) == SOCKET_ERROR) {
        MessageBox(NULL, "Can't bind socket", NULL, NULL);
        closesocket(micSocket);
        return FALSE;
    }

    return TRUE;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    mcRequest
--
-- DATE:        March 11, 2017
--
-- REVISIONS:   (Date and Description)
--
-- DESIGNER:
--
-- PROGRAMMER:
--
-- INTERFACE:   bool mcRequest(AudioPlayer& clnt)
--              AudioPlayer& clnt: client object that connects to server
--
-- RETURNS:     TRUE on success, FALSE on failure
--
-- NOTES:
-- Called to send multicast request to the server.
----------------------------------------------------------------------------------------------------------------------*/
bool mcRequest(AudioPlayer& clnt)
{
    UPLOADCONTEXT* uc = new UPLOADCONTEXT;
    uc->clnt = &clnt;
    uc->userReq = to_string(REQMULTICAST);

    CreateThread(NULL, 0, mcThread, uc, NULL, NULL);
    return TRUE;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    createMCSocket
--
-- DATE:        March 11, 2017
--
-- REVISIONS:   (Date and Description)
--
-- DESIGNER: 
--
-- PROGRAMMER: 
--
-- INTERFACE:   SOCKET createMCSocket()
--
-- RETURNS:     returns a Multicase socket created.
--
-- NOTES:  
-- This function creates a multicast socket and sets up the appropriate structures.  Upon completion,
-- it returns the SOCKET created.
----------------------------------------------------------------------------------------------------------------------*/
SOCKET createMCSocket()
{
    SOCKET			hSocket;
    WSADATA			stWSAData;
    SOCKADDR_IN		local_address;
    BOOL			fFlag;
    int				nRet;
    u_short nPort = MULTICAST_PORT;

    nRet = WSAStartup(0x0202, &stWSAData);
    if (nRet)
    {
        MessageBox(NULL, "WSAStartup failed", "Error", MB_OK);
        return NULL;
    }

    hSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (hSocket == INVALID_SOCKET)
    {
        MessageBox(NULL, "socket() failed", "Error", MB_OK);
        WSACleanup();
        return NULL;
    }

    fFlag = TRUE;
    nRet = setsockopt(hSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&fFlag, sizeof(fFlag));
    if (nRet == SOCKET_ERROR)
    {
        MessageBox(NULL, "setsockopt() SO_REUSEADDR failed", "Error", MB_OK);
    }

    local_address.sin_family      = AF_INET;
    local_address.sin_addr.s_addr = htonl(INADDR_ANY);
    local_address.sin_port        = htons(nPort);

    // bind to the socket
    nRet = bind(hSocket, (struct sockaddr*)&local_address, sizeof(local_address));
    if (nRet == SOCKET_ERROR)
    {
        MessageBox(NULL, "bind() port failed", "Error", MB_OK);
    }

    return hSocket;
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
-- INTERFACE:   DWORD WINAPI mcThread(LPVOID args)
--              LPVOID args: the argument passed in
--
-- RETURNS:     TRUE on success, FALSE on failure
--
-- NOTES:  
-- This function creates a multicast socket, sets up the multicast group, and plays the multicasted data
-- received from the sever.  Upon completion, the host is removed from the group and the socket is closed.
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI mcThread(LPVOID args)
{
    SOCKET			hSocket;
    int				result;
    struct ip_mreq	stMreq;
    SOCKADDR_IN		server;
    char			mcAddr[ADDRSIZE] = MULTICAST_ADDR;
    u_short			nPort = MULTICAST_PORT;

    streamPlayer = CreateZPlay();
    streamPlayer->SetSettings(sidSamplerate, SAMPLERATE);
    streamPlayer->SetSettings(sidChannelNumber, CHANNELNUM);
    streamPlayer->SetSettings(sidBitPerSample, BITPERSAMPLE);
    streamPlayer->SetSettings(sidBigEndian, BIGEND);

    if ((hSocket = createMCSocket()) == NULL)
    {
        MessageBox(NULL, "Could not create UDP socket", "Error", MB_OK);
        return FALSE;
    }

    stMreq.imr_multiaddr.s_addr = inet_addr(mcAddr);
    stMreq.imr_interface.s_addr = INADDR_ANY;

    // Use the IP_ADD_MEMBERSHIP option to join an IPv4 multicast group on a local IPv4 interface.
    // Use the SETSOCKOPT API and specify the address of the IP_MREQ structure that contains these addresses.
    result = setsockopt(hSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&stMreq, sizeof(stMreq));
    if (result == SOCKET_ERROR)
    {
        MessageBox(NULL, "setsockopt() IP_ADD_MEMBERSHIP failed", "Error", MB_OK);
        return FALSE;
    }

    // assign the socket created to multicast socket
    mcSocket = hSocket;

    char buff[MAXBUFSIZE] = "";
    int addr_size = sizeof(struct sockaddr_in);

    // open the stream
    if (!streamPlayer->OpenStream(1, 1, buff, sizeof(buff), sfPCM))
    {
        MessageBox(NULL, "OpenStream() failed", "Error", MB_OK);
        return FALSE;
    }
    
    // play the stream
    streamPlayer->Play();

    // read continuously, playing data
    while (TRUE)
    {
        memset(buff, 0, sizeof(buff));
        result = recvfrom(hSocket, buff, sizeof(buff), 0, (struct sockaddr*)&server, &addr_size);
        if (result < 0)
        {
            MessageBox(NULL, "Stopped receiving from multicast", "Error", MB_OK);
            WSACleanup();
            return FALSE;
        }

        streamPlayer->PushDataToStream(buff, result);

        if (result == 0)
            break;
    }

    streamPlayer->Release();

    stMreq.imr_multiaddr.s_addr = inet_addr(mcAddr);
    stMreq.imr_interface.s_addr = INADDR_ANY;

    // remove the host from the multicast group
    result = setsockopt(hSocket, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&stMreq, sizeof(stMreq));
    if (result == SOCKET_ERROR)
    {
        MessageBox(NULL, "setsockopt() IP_DROP_MEMBERSHIP failed", "Error", MB_OK);
    }

    closesocket(hSocket);
    WSACleanup();

    return TRUE;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    renderUI
--
-- DATE:        March 11, 2017
--
-- REVISIONS: 
--
-- DESIGNER: 
--
-- PROGRAMMER: 
--
-- INTERFACE:   bool renderUI(HWND hWnd)
--              HWND hWnd: the handle to the main window
--
-- RETURNS:     TRUE on success, FALSE on failure
--
-- NOTES:
-- Called to render client GUI.
----------------------------------------------------------------------------------------------------------------------*/
bool renderUI(HWND hWnd)
{
    HANDLE hFont = (HFONT) GetStockObject(DEFAULT_GUI_FONT);

    // Setup to create a progress bar for playback, download, upload status
    INITCOMMONCONTROLSEX InitCtrlEx;
    InitCtrlEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
    InitCtrlEx.dwICC  = ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&InitCtrlEx);

    // connection settings groupbox
    SendMessage(
        CreateWindowEx(NULL, "Button", "Connection Settings", WS_CHILD|WS_VISIBLE|BS_GROUPBOX,
        10, 10, 760, 75, hWnd, 0, GetModuleHandle(NULL), NULL)
        ,WM_SETFONT, (WPARAM)hFont, TRUE);
        
    // server/hostname label
    SendMessage(
        CreateWindow(TEXT("STATIC"), TEXT("Audio Hostname/IP"), WS_CHILD | WS_VISIBLE | SS_LEFT, 
        20, 30, 200, 25, hWnd, 0, GetModuleHandle(NULL), NULL)
        ,WM_SETFONT, (WPARAM)hFont, TRUE);
    
    // port label
    SendMessage(
        CreateWindow(TEXT("STATIC"), TEXT("Port"), WS_CHILD | WS_VISIBLE | SS_LEFT,
        310, 30, 100, 25, hWnd, 0, GetModuleHandle(NULL), NULL)
        ,WM_SETFONT, (WPARAM)hFont, TRUE);
        
    // server/hostname box
    SendMessage(
        CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_AUTOVSCROLL|ES_AUTOHSCROLL,
        20, 48, 280, 25, hWnd, (HMENU)IDC_EDIT_HOSTNAME, GetModuleHandle(NULL), NULL),
        WM_SETFONT, (WPARAM)hFont, TRUE);
    // set default value
    SendMessage(GetDlgItem(hWnd,IDC_EDIT_HOSTNAME), WM_SETTEXT, NULL, (LPARAM)szServer);
    SendMessage(GetDlgItem(hWnd,IDC_EDIT_HOSTNAME), EM_LIMITTEXT, WPARAM(128), 0);

    // port box
    SendMessage(
        CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_AUTOVSCROLL|ES_AUTOHSCROLL|ES_NUMBER,
        310, 48, 100, 25, hWnd, (HMENU)IDC_EDIT_PORT, GetModuleHandle(NULL), NULL)
        ,WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(GetDlgItem(hWnd,IDC_EDIT_PORT), WM_SETTEXT, NULL, (LPARAM)szPort);
    SendMessage(GetDlgItem(hWnd,IDC_EDIT_PORT), EM_LIMITTEXT, WPARAM(5), 0);
    
    // playlist groupbox
    SendMessage(
        CreateWindowEx(NULL, "Button", "Play List", WS_CHILD|WS_VISIBLE|BS_GROUPBOX,
        10, 90, 385, 268, hWnd, 0, GetModuleHandle(NULL), NULL)
        ,WM_SETFONT, (WPARAM)hFont, TRUE);

    // play listbox
    hPlayList = CreateWindowEx(WS_EX_CLIENTEDGE, "LISTBOX", "", WS_CHILD|WS_VISIBLE|LBS_STANDARD|LBS_HASSTRINGS,
                               20, 108, 365, 260, hWnd, (HMENU)IDC_PLAYLIST, GetModuleHandle(NULL), NULL);
    SendMessage(hPlayList, WM_SETFONT, (WPARAM)hFont, TRUE);

    // trace listbox
    hTraceList = CreateWindowEx(WS_EX_CLIENTEDGE, "LISTBOX", "", WS_CHILD | WS_VISIBLE | LBS_STANDARD | LBS_HASSTRINGS,
                                405, 108, 363, 220, hWnd, (HMENU)IDC_TRACELIST, GetModuleHandle(NULL), NULL);
    SendMessage(hTraceList, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    // create music control buttons
    SendMessage(
        CreateWindow("BUTTON", "&Rewind", WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_DEFPUSHBUTTON,
        405, 328, 60, 30, hWnd, (HMENU)IDC_BUTTON_REWIND, GetModuleHandle(NULL), NULL)
        ,WM_SETFONT, (WPARAM)hFont, TRUE);

    SendMessage(
        CreateWindow("BUTTON", "&Play", WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_DEFPUSHBUTTON,
        480, 328, 60, 30, hWnd, (HMENU)IDC_BUTTON_PLAY, GetModuleHandle(NULL), NULL)
        ,WM_SETFONT, (WPARAM)hFont, TRUE);

    SendMessage(
        CreateWindow("BUTTON", "P&ause", WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_DEFPUSHBUTTON,
        555, 328, 60, 30, hWnd, (HMENU)IDC_BUTTON_PAUSE, GetModuleHandle(NULL), NULL)
        ,WM_SETFONT, (WPARAM)hFont, TRUE);

    SendMessage(
        CreateWindow("BUTTON", "&Forward", WS_TABSTOP|WS_VISIBLE| WS_CHILD|BS_DEFPUSHBUTTON,
        630, 328, 60, 30, hWnd, (HMENU)IDC_BUTTON_FORWARD, GetModuleHandle(NULL), NULL)
        ,WM_SETFONT, (WPARAM)hFont, TRUE);

    SendMessage(
        CreateWindow("BUTTON", "&Stop", WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_DEFPUSHBUTTON,
        705, 328, 60, 30, hWnd, (HMENU)IDC_BUTTON_STOP, GetModuleHandle(NULL), NULL)
        ,WM_SETFONT, (WPARAM)hFont, TRUE);
    
    // Options groupbox
    SendMessage(
        CreateWindowEx(NULL, "Button", "Options", WS_CHILD|WS_VISIBLE|BS_GROUPBOX,
        10, 370, 760, 59, hWnd, 0, GetModuleHandle(NULL), NULL)
        ,WM_SETFONT, (WPARAM)hFont, TRUE);
        
    // radio buttons
    SendMessage(
        CreateWindowEx(0, "BUTTON", "Download", WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON,
        20, 390, 120, 25, hWnd, (HMENU)IDC_RADIO_DOWNLOAD, GetModuleHandle(NULL), NULL)
        ,WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(GetDlgItem(hWnd,IDC_RADIO_DOWNLOAD), BM_SETCHECK, 1, 0);
    
    SendMessage(
        CreateWindowEx(0, "BUTTON", "Upload", WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON,
        170, 390, 120, 25, hWnd, (HMENU)IDC_RADIO_UPLOAD, GetModuleHandle(NULL), NULL)
        ,WM_SETFONT, (WPARAM)hFont, TRUE);

    SendMessage(
        CreateWindowEx(0, "BUTTON", "Multicast", WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON, 
        320, 390, 120, 25, hWnd, (HMENU)IDC_RADIO_MULTICAST, GetModuleHandle(NULL), NULL)
        ,WM_SETFONT, (WPARAM)hFont, TRUE);

    SendMessage(
        CreateWindowEx(0, "BUTTON", "Microphone Chat", WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON,
        470, 390, 170, 25, hWnd, (HMENU)IDC_RADIO_MIC, GetModuleHandle(NULL), NULL)
        ,WM_SETFONT, (WPARAM)hFont, TRUE);

    SendMessage(
        CreateWindow("BUTTON", "&OK", WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_DEFPUSHBUTTON,
        550, 442, 100, 30, hWnd, (HMENU)IDC_BUTTON_OK, GetModuleHandle(NULL), NULL)
        ,WM_SETFONT, (WPARAM)hFont, TRUE);
    EnableWindow(GetDlgItem(hWnd,IDC_BUTTON_OK), FALSE); 

    SendMessage(
        CreateWindow("BUTTON", "&Cancel", WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_DEFPUSHBUTTON,
        670, 442, 100, 30, hWnd, (HMENU)IDC_BUTTON_CANCEL, GetModuleHandle(NULL), NULL)
        ,WM_SETFONT, (WPARAM)hFont, TRUE);

    // status bar
    int statwidths[] = {80, 200, 320, 455, 590, -1};
    SendMessage(
        CreateWindowEx(0, STATUSCLASSNAME, NULL, WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, hWnd, 
            (HMENU)IDC_MAIN_STATUS, GetModuleHandle(NULL), NULL), WM_SETFONT, (WPARAM)hFont, TRUE);

    SendMessage(GetDlgItem(hWnd,IDC_MAIN_STATUS), SB_SETPARTS, sizeof(statwidths)/sizeof(int), (LPARAM)statwidths);
    SendMessage(GetDlgItem(hWnd,IDC_MAIN_STATUS), SB_SETTEXT, STATUSBAR_MODE, (LPARAM)"Client");
    SendMessage(GetDlgItem(hWnd,IDC_MAIN_STATUS), SB_SETTEXT, STATUSBAR_STATUS, (LPARAM)"Not Connected");
    SendMessage(GetDlgItem(hWnd,IDC_MAIN_STATUS), SB_SETTEXT, STATUSBAR_PROTO, (LPARAM)"TCP");
    SendMessage(GetDlgItem(hWnd,IDC_MAIN_STATUS), SB_SETTEXT, STATUSBAR_STATS, (LPARAM)"Received: 0 bytes");
    SendMessage(GetDlgItem(hWnd,IDC_MAIN_STATUS), SB_SETTEXT, STATUSBAR_TIME, (LPARAM)"Time elapsed: 0 ms");

    return TRUE;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    openFileDialog
--
-- DATE:        March 11, 2017
--
-- REVISIONS: 
--
-- DESIGNER: 
--
-- PROGRAMMER: 
--
-- INTERFACE:   int openFileDialog(HWND hWnd, OPENFILENAME& ofn)
--              HWND hWnd: handle to the main window
--              OPENFILENAME& ofn: common dialog to open a file
--
-- RETURNS:     void
--
-- NOTES:
-- Initializes a OpenFileStruct Dialog for .txt files.
----------------------------------------------------------------------------------------------------------------------*/
void openFileDialog(HWND hWnd, OPENFILENAME &ofn)
{
    static char szFileName[MAX_PATH] = "";

    // initalize open file structure
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = "Waveform Files (*.wav)\0*.wav\0MP3 Files (*.mp3)\0*.mp3\0OGG Vorbis (*.ogg)\0*.ogg\0Advanced Audio (*.aac)\0*.aac\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = "txt";

}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    processPlayList
--
-- DATE:        March 11, 2017
--
-- REVISIONS: 
--
-- DESIGNER: 
--
-- PROGRAMMER: 
--
-- INTERFACE:   vector<string> processPlayList(string str)
--              string -- string of songs separated by new-line characters
--
-- RETURNS:     vector<string> -- vector of audio files
--
-- NOTES:
-- Converts a string of audio files separated by newlines into a vector
----------------------------------------------------------------------------------------------------------------------*/
vector<string> processPlayList(const string& str)
{
    vector<string> result; // store the audio files

    std::string audioname;
    std::istringstream iss(str);
    while (std::getline(iss, audioname))
    {
        result.push_back(audioname);
    }

    return result;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    populatePlayList
--
-- DATE:        March 11, 2017
--
-- REVISIONS: 
--
-- DESIGNER: 
--
-- PROGRAMMER: 
--
-- INTERFACE:   bool populatePlayList(HWND hList, vector<string> list)
--              HWND hList -- handle to the playlist box
--              vector<string> -- vector of items to load into listbox
--
-- RETURNS:     TRUE on success, FALSE on failure
--
-- NOTES:
-- Populates a vector into the playlist box.
----------------------------------------------------------------------------------------------------------------------*/
bool populatePlayList(HWND hList, vector<string> list)
{
    // clear list box
    SendMessage(hPlayList, WM_SETREDRAW, 0, 0);
    SendMessage (hPlayList,LB_RESETCONTENT,0, 0);

    // populate list box
    for (vector<string>::iterator it = list.begin(); it != list.end(); ++it)
    {
        string s = *it;
        SendMessage (hPlayList,LB_INSERTSTRING, 0, (LPARAM)s.c_str());

    }

    SendMessage(hPlayList, WM_SETREDRAW, 1, 0);

    return TRUE;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    getSelectedListBoxItem
--
-- DATE:        March 11, 2017
--
-- REVISIONS: 
--
-- DESIGNER: 
--
-- PROGRAMMER: 
--
-- INTERFACE:   string getSelectedListBoxItem(HWND* hWnd, int idListBox)
--              HWND* hWnd: pointer to main window
--              int idListBox: resource id of list box
--
-- RETURNS:     string -- audio name of selected item
--              "ERROR" if nothing selected
--
-- NOTES:
-- Gets the text of the item selected by the user in the playlist box.
----------------------------------------------------------------------------------------------------------------------*/
string getSelectedListBoxItem(HWND* hWnd, int idListBox)
{
    char* tmp = new char[MAXBUFSIZE];

    // get the number of items in the box
    int count = SendMessage(GetDlgItem(*hWnd, idListBox), LB_GETCOUNT, 0, 0);

    int iSelected = -1;

    // go through the items and find the first one selected
    for (int i = 0; i < count; i++)
    {
        // check if this item is selected or not
        if (SendMessage(GetDlgItem(*hWnd, idListBox), LB_GETSEL, i, 0) > 0)
        {
            iSelected = i;
            break;
        }
    }

    // get the text of the selected item
    if (iSelected == -1)
    {
        MessageBox(NULL, "No file selected to download", "Error", MB_OK);
        return "ERROR";
    }

    SendMessage(GetDlgItem(*hWnd, idListBox), LB_GETTEXT, (WPARAM)iSelected , (LPARAM)tmp);

    // char to string
    string result(tmp);

    return result;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    micCallBackFunc
--
-- DATE:        March 11, 2017
--
-- REVISIONS:	
--
-- DESIGNER:	
--
-- PROGRAMMER:	
--
-- INTERFACE:   int __stdcall micCallBackFunc (void* instance, void* user_data, TCallbackMessage message, 
--                                             unsigned int param1, unsigned int param2)
--              void* instance - ZPlay instance
--              void* user_data - user data specified by SetCallback
--              TCallbackMessage message - stream message. eg. fetch next song, need more streaming data, etc
--              unsigned int param1 - a pointer to buffer with PCM data
--              unsigned int param2 - number of bytes in PCM buffer
--
-- RETURNS:     based on the type of the TCallbackMessage (message) passed. See notes below.
--
-- NOTES:
-- This function will mainly listen for the MsgWaveBuffer message, which is a message for when a decoding thread is
-- ready to send data to the soundcard.
--      Returns:
--      0 - send data to soundcard
--      1 - skip sending data to soundcard
--      2 - stop playing
--      For MsgStop, all the parameters and the return type are not used. The message is used after a song stops playing.
--
----------------------------------------------------------------------------------------------------------------------*/
int __stdcall micCallBackFunc (void* instance, void *user_data, TCallbackMessage message, 
                               unsigned int param1, unsigned int param2)
{
    if ( message == MsgStop )
    {
        streamPlayer->Stop();
        micPlayer->Stop();
        closesocket(micSocket);
        return 2;
    }

    if (sendto(micSocket, (const char *)param1, param2, 0, 
        (const struct sockaddr*)&micServer, sizeof(micServer)) < 0)
        return 1;

    return 0;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    playAudio
--
-- DATE:        March 11, 2017
--
-- REVISIONS:
--
-- DESIGNER:
--
-- PROGRAMMER:
--
-- INTERFACE:   void playAudio(HWND hWnd, AudioPlayer& audioClnt)
--              HWND* hWnd: pointer to main window
--              AudioPlayer& audioClnt: client object
--
-- RETURNS:     void
--
-- NOTES:
-- Plays the audio specified.
----------------------------------------------------------------------------------------------------------------------*/
void playAudio(HWND hWnd, AudioPlayer& audioClnt)
{
    if (audioClnt.currentState != WAITFORCOMMAND)
    {
        MessageBox(hWnd, szWarn, "Warning", MB_ICONWARNING);
        return;
    }

    string item = getSelectedListBoxItem(&hWnd, IDC_PLAYLIST);
    
    if (item.compare(curItem) != 0)
    {
        curItem = item;
        // get the number of items in trace listbox
        int count = SendMessage(GetDlgItem(ghWnd, IDC_TRACELIST), LB_GETCOUNT, 0, 0);
        if (count > MAXITEMS) {
            SendMessage(hTraceList, LB_RESETCONTENT, 0, 0);
        }
        SendMessage(hTraceList, LB_INSERTSTRING, 0, (LPARAM)item.c_str());
    }

    audioClnt.currentAudioFile = item;
    streamRequest(audioClnt);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    pauseAudio
--
-- DATE:        March 11, 2017
--
-- REVISIONS:
--
-- DESIGNER:
--
-- PROGRAMMER:
--
-- INTERFACE:   void pauseAudio(AudioPlayer& audioClnt)
--              AudioPlayer& audioClnt: client object
--
-- RETURNS:     void
--
-- NOTES:
-- Pauses the audio specified.
----------------------------------------------------------------------------------------------------------------------*/
void pauseAudio(AudioPlayer& audioClnt)
{
    audioClnt.player_->Pause();
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    stopAudio
--
-- DATE:        March 11, 2017
--
-- REVISIONS:
--
-- DESIGNER:
--
-- PROGRAMMER:
--
-- INTERFACE:   void stopAudio(AudioPlayer& audioClnt)
--              AudioPlayer& audioClnt: client object
--
-- RETURNS:     void
--
-- NOTES:
-- Stops the audio specified
----------------------------------------------------------------------------------------------------------------------*/
void stopAudio(AudioPlayer& audioClnt)
{
    audioClnt.player_->Stop();
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:    closeAudio
--
-- DATE:        March 11, 2017
--
-- REVISIONS:
--
-- DESIGNER:
--
-- PROGRAMMER:
--
-- INTERFACE:   void closeAudio(HWND hWnd, AudioPlayer& audioClnt)
--              HWND* hWnd: pointer to main window
--              AudioPlayer& audioClnt: client object
--
-- RETURNS:     void
--
-- NOTES:
-- closes the audio specified and free up the resource associated with it.
----------------------------------------------------------------------------------------------------------------------*/
void closeAudio(HWND hWnd, AudioPlayer& audioClnt)
{
    if ((streamPlayer != NULL) || (micPlayer != NULL))
    {
        if (audioClnt.currentState == MICROPHONE) {
            sendto(micSocket, 0, 0, 0, (const sockaddr*)&micServer, sizeof(sockaddr_in));
            closesocket(micSocket);
        }

        // clean up stream player
        streamPlayer->Stop();
    }

    // return to idle state
    connected = false;
    audioClnt.currentState = NOTCONNECTED;
    audioClnt.player_->Stop();

    closesocket(mcSocket);
    closesocket(audioClnt.connectSocket_);

    // get user input
    SendMessage(GetDlgItem(hWnd, IDC_EDIT_PORT), WM_GETTEXT, sizeof(szPort) / sizeof(szPort[0]), (LPARAM)szPort);
    SendMessage(GetDlgItem(hWnd, IDC_EDIT_HOSTNAME), WM_GETTEXT, sizeof(szServer) / sizeof(szServer[0]), (LPARAM)szServer);

    WSADATA wsaData;
    if (audioClnt.runClient(&wsaData, szServer, atoi(szPort)))
    {
        SendMessage(GetDlgItem(hWnd, IDC_MAIN_STATUS), SB_SETTEXT, STATUSBAR_STATUS, (LPARAM)"Connected");
        EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_OK), TRUE);
        EnableWindow(GetDlgItem(hWnd, IDC_EDIT_PORT), FALSE);
        EnableWindow(GetDlgItem(hWnd, IDC_EDIT_HOSTNAME), FALSE);
        connected = true;

        listRequest(audioClnt, &hWnd);

    }
    else {
        MessageBox(hWnd, szWarn, "Warning", MB_ICONWARNING);
    }
}
