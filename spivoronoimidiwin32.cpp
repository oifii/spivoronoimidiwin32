// spivoronoimidiwin32.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "spivoronoimidiwin32.h"
#include <mmsystem.h> //for timeSetEvent()
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
using namespace std;
#include "oifiilib.h" //note: oifiilib.lib/.dll is an MFC extension and resource DLL

#include <list>
#include "portmidi.h"
#include <map>
#include <stdio.h>

#include "resource.h"

#include <direct.h> //for _mkdir

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
//TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
//TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
TCHAR szTitle[1024]={"spivoronoimidiwin32title"};					// The title bar text
TCHAR szWindowClass[1024]={"spivoronoimidiwin32class"};			// the main window class name

//new parameters
string global_begin="begin.ahk";
string global_end="end.ahk";

int global_titlebardisplay=1; //0 for off, 1 for on
int global_acceleratoractive=0; //0 for off, 1 for on
int global_menubardisplay=0; //0 for off, 1 for on

DWORD global_startstamp_ms;
string global_imagefolder="."; //the local folder of the .scr
//string global_imagefolder="d:\\zero\\zero06\\";
//string global_imagefolder="c:\\temp\\spivideocaptureframetodisk\\";
//string global_imagefolder="c:\\temp\\spissmandelbrotmidi\\";
//string global_imagefolder="c:\\temp\\spicapturewebcam\\";
float global_duration_sec=3600;
float global_sleeptimepervoronoi_sec=5;
//float global_sleeptimepervoronoi_sec=2;
//float global_sleeptimepervoronoi_sec=16;
//float global_sleeptimepervoronoi_sec=8;
int global_x=100;
int global_y=100;
int global_xwidth=300;
int global_yheight=300;
BYTE global_alpha=255;

int global_imageid=0;
HWND global_hwnd=NULL;
MMRESULT global_timer=0;
int global_imageheight=-1; //will be computed within WM_SIZE handler
int global_imagewidth=-1; //will be computed within WM_SIZE handler 
vector<string> global_txtfilenames;
COW2Doc* global_pOW2Doc=NULL;
COW2View* global_pOW2View=NULL;

const int global_pmeventlistsize = 64;
list<PmEvent*> global_pmeventlist[global_pmeventlistsize]; 
list<PmEvent*>::iterator it_pmeventlist;
bool global_pmevenlistbusyfrommiditimer[global_pmeventlistsize];
bool global_pmevenlistbusyfromglobaltimer[global_pmeventlistsize];
bool global_miditimerskip=false;
UINT global_miditimer=2;
UINT global_miditimer_programchange=3;
int global_prevstep=-1;
int global_midistep_ms=250;
//int global_midistep_ms=125;
int global_midiprogramchangeperiod_ms=1000*3*60;
FILE* pFILE = NULL;

map<string,int> global_midioutputdevicemap;
string global_midioutputdevicename="Out To MIDI Yoke:  1";
//string global_midioutputdevicename="E-DSP MIDI Port [FFC0]";
//string global_midioutputdevicename="E-DSP MIDI Port 2 [FFC0]";
int global_outputmidichannel=0;
int global_midicontrolnumber=9; //9 is undefined, so it is available for us to use
bool global_bsendmidi=true;
bool global_bsendmidi_usingremap=true;
PmStream* global_pPmStream = NULL; // midi output



// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
void OnTimerMidi();
void OnTimerMidiProgramChange();

// Convert a wide Unicode string to an UTF8 string
std::string utf8_encode(const std::wstring &wstr)
{
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo( size_needed, 0 );
    WideCharToMultiByte                  (CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// Convert an UTF8 string to a wide Unicode String
std::wstring utf8_decode(const std::string &str)
{
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo( size_needed, 0 );
    MultiByteToWideChar                  (CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}


void CALLBACK StartGlobalProcess(UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	//WavSetLib_Initialize(global_hwnd, IDC_MAIN_STATIC, global_staticwidth, global_staticheight, global_fontwidth, global_fontheight, global_staticalignment);

	DWORD nowstamp_ms = GetTickCount();
	while( (global_duration_sec<0.0f) || ((nowstamp_ms-global_startstamp_ms)/1000.0f)<global_duration_sec )
	{
		if(global_pOW2Doc) delete global_pOW2Doc;
		if(global_pOW2View) delete global_pOW2View;

		if(global_txtfilenames.size()>0)
		{
			//reset image id if end of file
			if(global_imageid>=global_txtfilenames.size()) global_imageid=0;

				
				global_pOW2Doc = new COW2Doc(global_hwnd);
				global_pOW2View = new COW2View(global_hwnd, (COWDocument*)global_pOW2Doc);
				global_pOW2View->OnCreate(NULL);
				if(global_pOW2Doc!=NULL && global_pOW2View!=NULL) 
				{
					if(pFILE) 
					{
						fprintf(pFILE, "%s\n", (global_txtfilenames[global_imageid]).c_str());
						fflush(pFILE);
					}
					bool bresult=global_pOW2Doc->OnOpenDocument((global_txtfilenames[global_imageid]).c_str());
					if(bresult) 
					{
						global_pOW2View->OnInitialUpdate();
						
						//global_pOW2View->m_pViewDataSettings->bDrawPixelset = FALSE;
						//global_pOW2View->m_pViewDataSettings->bDrawPixelset = TRUE;

						InvalidateRect(global_hwnd, NULL, false);
						global_pOW2Doc->OnAnalysisTextureextraction();
						
						//global_pOW2View->m_pViewDataSettings->bDrawPointset = TRUE;
						global_pOW2View->m_pViewDataSettings->bDrawPointset = FALSE;
						global_pOW2View->m_pViewDataSettings->bDrawVoronoiset = TRUE;
						InvalidateRect(global_hwnd, NULL, false);

						//spi, avril 2015, begin
						string fullpath = "D:\\temp\\";
						_mkdir(fullpath.c_str());
						fullpath = fullpath + global_txtfilenames[global_imageid].c_str();
						global_pOW2Doc->SaveVoronoiDiagramColoredWithPixelsetAsImagefile(fullpath.c_str(), global_pOW2View);
						
						//spi, avril 2015, end
					}
				}
				//trigger WM_PAINT
				//InvalidateRect(global_hwnd, NULL, false);
				global_imageid++;

		}
		//create midi events, various notes with various durations
		if(global_bsendmidi)
		{
			//global_miditimerskip=true;
			global_miditimerskip=false;
			for(int k=0; k<global_pmeventlistsize; k++)
			{
				/**/ //bypass random for testing
				//-1) only 25% of the time
				int random_integer;
				int lowest=0;
				//int highest=global_pmeventlistsize-1;
				int highest=4-1;
				int range=(highest-lowest)+1;
				//random_integer = lowest+int(range*rand()/(RAND_MAX + 1.0));
				random_integer = lowest+rand()%range;
				if(random_integer!=1) continue;
				
				/* //for testing
				if(k%4!=0) continue;
				*/

				/* //for testing
				//0) create constant note midi message
				int note=60;
				*/
				//0) create note midi message
				lowest=0;
				highest=global_pOW2Doc->GetNumberOfPoint()-1;
				range=(highest-lowest)+1;
				random_integer = lowest+rand()%range;
				int idpointsetobject = random_integer;
				double dfVoronoiArea = global_pOW2Doc->GetPointsetObjectStatAsDouble(idpointsetobject, POINTSET_OFFSETSTAT_VOROAREA);
				int note = dfVoronoiArea*127;
				if(note<0) note = 0;
				if(note >127) note =127;
				double dfIntensity = global_pOW2Doc->GetPointsetObjectStatAsDouble(idpointsetobject, POINTSET_EXTRA_NUMBEROFSTAT + POINTSET_XYZI_INTENSITY);
				int velocity = dfIntensity*127;
				if(velocity<0) velocity=0;
				if(velocity>127) velocity=127;
				PmEvent* pPmEvent = new PmEvent;
				pPmEvent->timestamp = 0;
				pPmEvent->message = Pm_Message(0x90+global_outputmidichannel, note, velocity);
				while(global_pmevenlistbusyfrommiditimer[k]==true) Sleep(10);
				global_pmevenlistbusyfromglobaltimer[k]=true;
				global_pmeventlist[k].push_back(pPmEvent);
				global_pmevenlistbusyfromglobaltimer[k]=false;
				//1) set duration
				//double dfArea = global_pOW2Doc->GetPointsetObjectStatAsDouble(idpointsetobject, POINTSET_EXTRA_NUMBEROFSTAT + POINTSET_XYZI_TREEAREA);
				//1) pick a duration at random
				//int random_integer;
				lowest=1;
				//int highest=global_pmeventlistsize-1;
				highest=10-1;
				range=(highest-lowest)+1;
				//random_integer = lowest+int(range*rand()/(RAND_MAX + 1.0));
				random_integer = lowest+rand()%range;
				/*
				random_integer = 4; //bypass random for testing
				*/
				int kk=k+random_integer;
				if(kk>(global_pmeventlistsize-1)) kk=kk-global_pmeventlistsize;
				//2) create note off midi message
				pPmEvent = new PmEvent;
				pPmEvent->timestamp = 0;
				pPmEvent->message = Pm_Message(0x90+global_outputmidichannel, note, 0);
				while(global_pmevenlistbusyfrommiditimer[kk]==true) Sleep(10);
				global_pmevenlistbusyfromglobaltimer[kk]=true;
				global_pmeventlist[kk].push_back(pPmEvent);
				global_pmevenlistbusyfromglobaltimer[kk]=false;
			}
			global_miditimerskip=false;
		}

		Sleep((int)(global_sleeptimepervoronoi_sec*1000));
		//StatusAddText(global_line.c_str());



		nowstamp_ms = GetTickCount();
	}
	PostMessage(global_hwnd, WM_DESTROY, 0, 0);
}




PCHAR*
    CommandLineToArgvA(
        PCHAR CmdLine,
        int* _argc
        )
    {
        PCHAR* argv;
        PCHAR  _argv;
        ULONG   len;
        ULONG   argc;
        CHAR   a;
        ULONG   i, j;

        BOOLEAN  in_QM;
        BOOLEAN  in_TEXT;
        BOOLEAN  in_SPACE;

        len = strlen(CmdLine);
        i = ((len+2)/2)*sizeof(PVOID) + sizeof(PVOID);

        argv = (PCHAR*)GlobalAlloc(GMEM_FIXED,
            i + (len+2)*sizeof(CHAR));

        _argv = (PCHAR)(((PUCHAR)argv)+i);

        argc = 0;
        argv[argc] = _argv;
        in_QM = FALSE;
        in_TEXT = FALSE;
        in_SPACE = TRUE;
        i = 0;
        j = 0;

        while( a = CmdLine[i] ) {
            if(in_QM) {
                if(a == '\"') {
                    in_QM = FALSE;
                } else {
                    _argv[j] = a;
                    j++;
                }
            } else {
                switch(a) {
                case '\"':
                    in_QM = TRUE;
                    in_TEXT = TRUE;
                    if(in_SPACE) {
                        argv[argc] = _argv+j;
                        argc++;
                    }
                    in_SPACE = FALSE;
                    break;
                case ' ':
                case '\t':
                case '\n':
                case '\r':
                    if(in_TEXT) {
                        _argv[j] = '\0';
                        j++;
                    }
                    in_TEXT = FALSE;
                    in_SPACE = TRUE;
                    break;
                default:
                    in_TEXT = TRUE;
                    if(in_SPACE) {
                        argv[argc] = _argv+j;
                        argc++;
                    }
                    _argv[j] = a;
                    j++;
                    in_SPACE = FALSE;
                    break;
                }
            }
            i++;
        }
        _argv[j] = '\0';
        argv[argc] = NULL;

        (*_argc) = argc;
        return argv;
    }


int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	global_startstamp_ms = GetTickCount();

	//LPWSTR *szArgList;
	LPSTR *szArgList;
	int nArgs;

	//szArgList = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	szArgList = CommandLineToArgvA(GetCommandLineA(), &nArgs);
	if( NULL == szArgList )
	{
		//wprintf(L"CommandLineToArgvW failed\n");
		return FALSE;
	}
	if(nArgs>1)
	{
		global_imagefolder = szArgList[1];
	}
	if(nArgs>2)
	{
		global_duration_sec = atof(szArgList[2]);
	}
	if(nArgs>3)
	{
		global_sleeptimepervoronoi_sec = atof(szArgList[3]);
	}
	if(nArgs>4)
	{
		global_x = atoi(szArgList[4]);
	}
	if(nArgs>5)
	{
		global_y = atoi(szArgList[5]);
	}
	if(nArgs>6)
	{
		global_xwidth = atoi(szArgList[6]);
	}
	if(nArgs>7)
	{
		global_yheight = atoi(szArgList[7]);
	}
	if(nArgs>8)
	{
		global_alpha = atoi(szArgList[8]);
	}
	if(nArgs>9)
	{
		global_titlebardisplay = atoi(szArgList[9]);
	}
	if(nArgs>10)
	{
		global_menubardisplay = atoi(szArgList[10]);
	}
	if(nArgs>11)
	{
		global_acceleratoractive = atoi(szArgList[11]);
	}
	//new parameters
	if(nArgs>12)
	{
		strcpy(szWindowClass, szArgList[12]); 
	}
	if(nArgs>13)
	{
		strcpy(szTitle, szArgList[13]); 
	}
	if(nArgs>14)
	{
		global_begin = szArgList[14]; 
	}
	if(nArgs>15)
	{
		global_end = szArgList[15]; 
	}
	LocalFree(szArgList);

	int nShowCmd = false;
	//ShellExecuteA(NULL, "open", "begin.bat", "", NULL, nShowCmd);
	ShellExecuteA(NULL, "open", global_begin.c_str(), "", NULL, nCmdShow);

	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	//LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	//LoadString(hInstance, IDC_SPIVORONOIMIDIWIN32, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	if(global_acceleratoractive)
	{
		hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SPIVORONOIMIDIWIN32));
	}
	else
	{
		hAccelTable = NULL;
	}

	pFILE = fopen("debug.txt", "w");

	//////////////////////////
	//initialize random number
	//////////////////////////
	srand((unsigned)time(0));


	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SPIVORONOIMIDIWIN32));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);

	if(global_menubardisplay)
	{
		wcex.lpszMenuName = MAKEINTRESOURCE(IDC_SPIVORONOIMIDIWIN32);
	}
	else
	{
		wcex.lpszMenuName = NULL; //no menu
	}
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

	//global_hFont=CreateFontW(global_fontheight,0,0,0,FW_NORMAL,0,0,0,0,0,0,2,0,L"SYSTEM_FIXED_FONT");
	//global_hFont=CreateFontW(global_fontheight,0,0,0,FW_NORMAL,0,0,0,0,0,0,2,0,L"Segoe Script");

	if(global_titlebardisplay)
	{
		hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, //original with WS_CAPTION etc.
			global_x, global_y, global_xwidth, global_yheight, NULL, NULL, hInstance, NULL);
	}
	else
	{
		hWnd = CreateWindow(szWindowClass, szTitle, WS_POPUP | WS_VISIBLE, //no WS_CAPTION etc.
			global_x, global_y, global_xwidth, global_yheight, NULL, NULL, hInstance, NULL);
	}
	if (!hWnd)
	{
		return FALSE;
	}
	global_hwnd = hWnd;

	SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
	SetLayeredWindowAttributes(hWnd, 0, global_alpha, LWA_ALPHA);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	global_timer=timeSetEvent(100,25,(LPTIMECALLBACK)&StartGlobalProcess,0,TIME_ONESHOT);
	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_CREATE:
		{
			//0) execute cmd line to get all folder's .bmp filenames
			string quote = "\"";
			string pathfilter;
			string path=global_imagefolder;
			pathfilter = path + "\\*.bmp";
			//pathfilter = path + "\\*.jpg";
			string systemcommand;
			//systemcommand = "DIR " + quote + pathfilter + quote + "/B /O:N > wsic_filenames.txt"; //wsip tag standing for wav set (library) instrumentset (class) populate (function)
			//systemcommand = "DIR " + quote + pathfilter + quote + "/B /S /O:N > spivoronoi_filenames.txt"; // /S for adding path into "spivoronoi_filenames.txt"
			systemcommand = "DIR " + quote + pathfilter + quote + "/B /O:N > spivoronoi_filenames.txt"; 
			system(systemcommand.c_str());

			//2) load in all "spiss_filenames.txt" file
			//vector<string> global_txtfilenames;
			ifstream ifs("spivoronoi_filenames.txt");
			string temp;
			while(getline(ifs,temp))
			{
				//txtfilenames.push_back(path + "\\" + temp);
				global_txtfilenames.push_back(temp);
			}

			//3) initialize portmidi
			if(global_bsendmidi)
			{
				/////////////////////////
				//portmidi initialization
				/////////////////////////
				PmError err;
				Pm_Initialize();
				// list device information 
				if(pFILE) fprintf(pFILE, "MIDI output devices:\n");
				for (int i = 0; i < Pm_CountDevices(); i++) 
				{
					const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
					if (info->output) 
					{
						if(pFILE) fprintf(pFILE, "%d: %s, %s\n", i, info->interf, info->name);
						string devicename = info->name;
						global_midioutputdevicemap.insert(pair<string,int>(devicename,i));
					}
				}
				int midioutputdeviceid = 13;
				map<string,int>::iterator it;
				it = global_midioutputdevicemap.find(global_midioutputdevicename);
				if(it!=global_midioutputdevicemap.end())
				{
					midioutputdeviceid = (*it).second;
					if(pFILE) fprintf(pFILE, "%s maps to %d\n", global_midioutputdevicename.c_str(), midioutputdeviceid);
				}
				if(pFILE) fprintf(pFILE, "device %d selected\n", midioutputdeviceid);
				//err = Pm_OpenInput(&midi_in, inp, NULL, 512, NULL, NULL);
				err = Pm_OpenOutput(&global_pPmStream, midioutputdeviceid, NULL, 512, NULL, NULL, 0); //0 latency
				if (err) 
				{
					fprintf(pFILE, Pm_GetErrorText(err));
					//Pt_Stop();
					//Terminate();
					//mmexit(1);
					global_bsendmidi = false;
				}
				//2.5)
				for(int k=0; k<global_pmeventlistsize; k++)
				{
					global_pmevenlistbusyfrommiditimer[k]=false;
					global_pmevenlistbusyfromglobaltimer[k]=false;
				}
				//3) set midi timers
				SetTimer( hWnd, global_miditimer, global_midistep_ms, NULL );
				SetTimer( hWnd, global_miditimer_programchange, global_midiprogramchangeperiod_ms, NULL );
			}


			
		}
		break;
	case WM_SIZE:
		{
			RECT rcClient;
			GetClientRect(hWnd, &rcClient);

			global_imagewidth = rcClient.right - 0;
			global_imageheight = rcClient.bottom - 0; 
		}
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		if(global_pOW2Doc!=NULL && global_pOW2View!=NULL)
		{
			CDC* pDC = CDC::FromHandle(hdc);
			global_pOW2View->OnDraw(pDC);
		}
		EndPaint(hWnd, &ps);
		break;
	  case WM_TIMER:
		  if(wParam==global_miditimer)
		  {
			  OnTimerMidi();
		  }
		  else if(wParam==global_miditimer_programchange)
		  {
			  OnTimerMidiProgramChange();
		  }
		return 0;                           

	case WM_DESTROY:
		{
			if (global_timer) timeKillEvent(global_timer);
			if(global_pOW2Doc) delete global_pOW2Doc;
			if(global_pOW2View) delete global_pOW2View;
			for(int k=0; k<global_pmeventlistsize; k++)
			{
				for (it_pmeventlist = global_pmeventlist[k].begin(); it_pmeventlist != global_pmeventlist[k].end(); it_pmeventlist++)
				{
					if(*it_pmeventlist) delete *it_pmeventlist;
				}
			}
			if(pFILE) fclose(pFILE);
			if(global_bsendmidi)
			{
				KillTimer(hWnd, global_miditimer);
				KillTimer(hWnd, global_miditimer_programchange);
				if(global_pPmStream) 
				{
					//send all note off
					for(int k=0; k<128; k++)
					{
						PmEvent myPmEvent;
						myPmEvent.timestamp = 0;
						myPmEvent.message = Pm_Message(0x90+global_outputmidichannel, k, 0);
						//send midi event
						Pm_Write(global_pPmStream, &myPmEvent, 1);
					}
					Pm_Close(global_pPmStream);
				}
				//Pt_Stop();
				Pm_Terminate();
			}

			int nShowCmd = false;
			//ShellExecuteA(NULL, "open", "end.bat", "", NULL, nShowCmd);
			ShellExecuteA(NULL, "open", global_end.c_str(), "", NULL, 0);
			PostQuitMessage(0);
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

void OnTimerMidi()
{
	if(global_bsendmidi)
	{
		global_prevstep++;
		if(global_prevstep<0) global_prevstep=0;
		if(global_prevstep>(global_pmeventlistsize-1)) global_prevstep=0; //restart to begining

		if(global_miditimerskip==true || global_pmevenlistbusyfromglobaltimer[global_prevstep]==false)
		{
			global_pmevenlistbusyfrommiditimer[global_prevstep]=true;
			for (it_pmeventlist = global_pmeventlist[global_prevstep].begin(); it_pmeventlist != global_pmeventlist[global_prevstep].end(); it_pmeventlist++)
			{
				if((*it_pmeventlist)!=NULL)
				{
					*it_pmeventlist;
					//send midi event
					Pm_Write(global_pPmStream, (*it_pmeventlist), 1);
					//delete pmevent
					delete *it_pmeventlist;
					*it_pmeventlist = NULL;
				}
			}
			//all midi event sent and deleted, empty list
			global_pmeventlist[global_prevstep].empty();
			global_pmevenlistbusyfrommiditimer[global_prevstep]=false;
		}
	}
}

void OnTimerMidiProgramChange()
{
	if(global_bsendmidi)
	{
		PmEvent myPmEvent;

		int random_integer;
		int lowest=0;
		int highest=128-1;
		//int highest=7-1;
		int range=(highest-lowest)+1;
		//random_integer = lowest+int(range*rand()/(RAND_MAX + 1.0));
		random_integer = lowest+rand()%range;
		//fprintf(pFILE, "random_integer=%d\n", random_integer);	
		
		//PmEvent myPmEvent;
		myPmEvent.timestamp = 0;
		int midiprogramnumber = random_integer;
		if(midiprogramnumber>=128) midiprogramnumber=127;
		if(midiprogramnumber<=0) midiprogramnumber=0;
		int notused = 0;
		myPmEvent.message = Pm_Message(0xC0+global_outputmidichannel, midiprogramnumber, 0x00);
		//myPmEvent.message = Pm_Message(192+global_outputmidichannel, midiprogramnumber, 0);
		//send midi event
		Pm_Write(global_pPmStream, &myPmEvent, 1);
		
	}
}
