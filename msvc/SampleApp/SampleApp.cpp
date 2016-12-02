// SampleApp.cpp
//

#include "stdafx.h"
#include "SampleApp.h"
#include "Julius.h"

// Use a Julius class
cJulius julius;

#define MAX_LOADSTRING 100

// Global variables
HINSTANCE hInst;			// Current interface
TCHAR szTitle[MAX_LOADSTRING];		// Text on the title bar
TCHAR szWindowClass[MAX_LOADSTRING];	// Main windows class name

// Function prototype definitions
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	MSG msg;
	HACCEL hAccelTable;

	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_SAMPLEAPP, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Initialize application
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SAMPLEAPP));

	// Main message loop
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
//  Function: MyRegisterClass()
//
//  Register a window class.
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
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SAMPLEAPP));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_SAMPLEAPP);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   Function: InitInstance(HINSTANCE, int)
//
//   Save instance handle and create a main window
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   // CHANGE THIS TO YOUR ENVIRONMENT AND YOUR MODEL ENCODING!
   setlocale(LC_CTYPE, "Japanese_Japan.20932"); // Japanese EUC-JP

   hInst = hInstance; // store instance to global

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  Function: GetJconfFileName( char *, int )
//
//  Open file dialog to get jconf file name
//
#include "CommDlg.h"
bool GetJconfFileName( char *buf, int buflen )
{
	wchar_t	wszFileName[MAX_PATH];
	bool ret;
	static OPENFILENAME ofn;

	ZeroMemory( &wszFileName, sizeof(wchar_t) * MAX_PATH );
	ZeroMemory( &ofn, sizeof(OPENFILENAME) );
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = NULL;
	ofn.lpstrFilter = L"Jconf File(*.jconf)\0*.jconf\0\0";
	ofn.lpstrFile = wszFileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = L"jconf";
	ofn.lpstrTitle = L"Open Jconf";

	if( GetOpenFileName( &ofn ) ) {
		// convert wide-char to multi-byte char (limitation of JuliusLib...)
		size_t converted = 0;
		wcstombs_s(&converted, buf, buflen, wszFileName, _TRUNCATE);
		ret = true;
	} else {
		ret = false;
	}
	return ret;
}

//
//  Function: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  Process messages received at the main window
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	int jEventId;
	int jResultId;
	static char conffile[1024];

	switch (message)
	{
	case WM_CREATE:
		break;
	case WM_JULIUS:
		jEventId = LOWORD(wParam);
		switch (jEventId) {
		case JEVENT_ENGINE_ACTIVE:	DebugOut(hWnd, L"Engine Active"); break;
		case JEVENT_ENGINE_INACTIVE:DebugOut(hWnd, L"Engine Inactive"); break;
		case JEVENT_ENGINE_PAUSE:	DebugOut(hWnd, L"Engine Pause"); break;
		case JEVENT_ENGINE_RESUME:	DebugOut(hWnd, L"Engine Resume"); break;
		case JEVENT_AUDIO_READY:	DebugOut(hWnd, L"Audio Input Ready"); break;
		case JEVENT_AUDIO_BEGIN:	DebugOut(hWnd, L"Audio Input Begin"); break;
		case JEVENT_AUDIO_END:		DebugOut(hWnd, L"Audio Input End"); break;
		case JEVENT_RECOG_BEGIN:	DebugOut(hWnd, L"Recognition Begin"); break;
		case JEVENT_RECOG_END:		DebugOut(hWnd, L"Recognition End"); break;
		case JEVENT_RECOG_FRAME:	/*DebugOut(hWnd, L"Recognition Frame")*/; break;
		case JEVENT_RESULT_FRAME:	/* DebugOut(hWnd, L"Result Frame"); */ break;
		case JEVENT_RESULT_PASS1:	DebugOut(hWnd, L"Result Pass1"); break;
		case JEVENT_RESULT_FINAL:	DebugOut(hWnd, L"Result Final");
			jResultId = HIWORD(wParam);
			if (jResultId != 0) {
				DebugOut(hWnd, L"No result");
			} else {
				DebugOut(hWnd, (wchar_t *)lParam);
			}
			break;
		case JEVENT_GRAM_UPDATE:	DebugOut(hWnd, L"Grammar changed"); break;
		default:					DebugOut(hWnd, L"! unknown event"); break;
		}
		//InvalidateRect(hWnd, NULL, TRUE);
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		switch (wmId)
		{
		case IDM_OPENJCONF:
			// (re-)open jconf file and prepare for recognition
			if (GetJconfFileName( conffile, 1024 )) {
				// initialize Julius engine to prepare for recognition
				DebugOut(hWnd, L"Loading Julius Engine...");
				if (! julius.initialize( conffile ) ) {
					MessageBox(hWnd, L"Error while loading Julius engine.\n", L"Error", MB_OK);
					break;
				}
				DebugOut(hWnd, L"Done.");
				DebugOut(hWnd, L"Do [Command]-[Start] to start recognition.");
			}
			break;
		case IDM_STARTPROCESS:
			// open audio stream and start recognition thread
			if (! julius.startProcess(hWnd)) {
				MessageBox(hWnd, L"failed to start process", L"Error", MB_OK);
			}
			break;
		case IDM_STOPPROCESS:
			// close audio stream and finish recognition thread
			julius.stopProcess();
			break;
		case IDM_PAUSE:
			// pause engine
			julius.pause();
			break;
		case IDM_RESUME:
			// resume paused engine
			julius.resume();
			break;
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
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for the version box
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

// Output a debug string to the main window
void DebugOut(HWND hWnd, wchar_t *str)
{
	static int line = 0;
	HDC hdc = GetDC(hWnd);
	TextOut(hdc , 10 , 10 + line * 20, str, lstrlen(str));
	ReleaseDC(hWnd , hdc);
	if (++line > 22) line = 0;
}
