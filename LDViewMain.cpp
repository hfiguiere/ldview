#include <windows.h>
#include <windowsx.h>
#include "ModelLoader.h"
#include "SSConfigure.h"
#include <TCFoundation/TCAutoreleasePool.h>
#include <TCFoundation/TCDictionary.h>
#include <TCFoundation/TCUserDefaults.h>
#include <TCFoundation/mystring.h>
#include <CUI/CUIWindow.h>
#include "SSPreview.h"
#include "AppResources.h"
#include "UserDefaultsKeys.h"
#include <stdio.h>

#define NAME "LDView"

void createConsole(void)
{
	COORD size = {80, 1000};
	SMALL_RECT rect = {0, 0, 79, 24};
//	HANDLE hOrigStdOut;
	HANDLE hStdOut;
	SECURITY_ATTRIBUTES securityAttributes;

	AllocConsole();
	securityAttributes.nLength = sizeof SECURITY_ATTRIBUTES;
	securityAttributes.lpSecurityDescriptor = NULL;
	securityAttributes.bInheritHandle = TRUE;
	hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
//	hStdOut = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
//		FILE_SHARE_READ | FILE_SHARE_WRITE, &securityAttributes,
//		CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleScreenBufferSize(hStdOut, size);
	SetConsoleWindowInfo(hStdOut, TRUE, &rect);
	SetConsoleActiveScreenBuffer(hStdOut);
//	SetStdHandle(STD_OUTPUT_HANDLE, hStdOut);
	freopen("CONOUT$", "w", stdout);
	freopen("CONIN$", "r", stdin);
}

bool isScreenSaver(void)
{
	char *programName;
	bool retVal;
	char *commandLine = NULL;

	commandLine = copyString(GetCommandLine());
	if (commandLine[0] == '"')
	{
		if (strchr(commandLine + 1, '"'))
		{
			*strchr(commandLine + 1, '"') = 0;
		}
		programName = commandLine + 1;
	}
	else
	{
		if (strchr(commandLine, ' '))
		{
			*strchr(commandLine, ' ') = 0;
		}
		programName = commandLine;
	}
	retVal = strlen(programName) > 4 &&
		stricmp(programName + strlen(programName) - 4, ".scr") == 0;
	delete commandLine;
	return retVal;
}

void debugOut(char * /*fmt*/, ...)
{
/*
	FILE* debugFile = fopen("C:\\LDViewDebug.txt", "a+");

	if (debugFile)
	{
		va_list marker;

		va_start(marker, fmt);
		vfprintf(debugFile, fmt, marker);
		va_end(marker);
		fclose(debugFile);
	}
*/
}

int mainLoop(HINSTANCE hInstance)
{
	MSG msg;
	HACCEL hAccel;
	bool screenSaver = isScreenSaver();
//	DWORD startTickCount = GetTickCount();

	hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATORS));
	while (1)
	{
		HWND parentWindow;
		HWND topParent;
		HWND newParent;
		DWORD tickCount = GetTickCount();

		debugOut("%d\n", tickCount);
/*
		if (tickCount > startTickCount + 3000 || tickCount < startTickCount)
		{
			OleUninitialize();
			return 0;
		}
*/
		if (!GetMessage(&msg, NULL, 0, 0))
		{
			OleUninitialize();
			return msg.wParam;
		}
#ifdef _DEBUG
//			_CrtDbgReport(_CRT_WARN, NULL, 0, NULL, "hWnd: 0x%6X msg: 0x%X\n", msg.hwnd, msg.message);
#endif // _DEBUG
		parentWindow = GetParent(msg.hwnd);
		topParent = msg.hwnd;
		while ((newParent = GetParent(topParent)) != NULL)
		{
			topParent = newParent;
		}
		if (screenSaver || !TranslateAccelerator( 
		    topParent,     // handle to receiving window 
			hAccel,        // handle to active accelerator table 
			&msg))
		{
			if (!parentWindow || !IsDialogMessage(parentWindow, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				if (screenSaver && msg.message == WM_DESTROY)
				{
					debugOut("WM_DESTROY\n", tickCount);
					PostQuitMessage(0);
				}
			}
		}
		TCAutoreleasePool::processReleases();
	}
}

BOOL flushPreviewEvents(HWND hWindow)
{
    // In preview mode, "pause" (enter a limited message loop) briefly 
    // before proceeding, so the display control panel knows to update itself.
    BOOL waitForInputIdle = TRUE;

    // Post a message to mark the end of the initial group of window messages
    PostMessage( hWindow, WM_USER, 0, 0 );

    MSG msg;
    while( waitForInputIdle )
    {
        // If GetMessage returns FALSE, it's quitting time.
        if( !GetMessage( &msg, NULL, 0, 0 ) )
        {
			return TRUE;
        }

        TranslateMessage( &msg );
		if (msg.message == WM_USER && msg.hwnd == hWindow)
		{
			waitForInputIdle = FALSE;
		}
		else
		{
			DispatchMessage( &msg );
		}
    }
	return TRUE;
}

void screenSaverLoop(HINSTANCE /*hInstance*/, HWND hWindow)
{
    MSG msg;
	BOOL bGotMsg;

    msg.message = WM_NULL;
    while ( msg.message != WM_QUIT && msg.message != WM_DESTROY )
    {
        bGotMsg = PeekMessage( &msg, NULL, 0, 0, PM_REMOVE );
        if( bGotMsg )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
			debugOut("Message: 0x%08X\n", msg.message);
        }
        else
        {
			debugOut("No Message\n");
			if (!GetParent(hWindow))
			{
				PostQuitMessage(0);
			}
            Sleep(100);
        }
    }
/*
    while( 1 )
    {
		FILE* debugFile;
        // If GetMessage returns FALSE, it's quitting time.
        if( !GetMessage( &msg, NULL, 0, 0 ) )
        {
			debugFile = fopen("LDViewDebug.txt", "a+");

			if (debugFile)
			{
				fprintf(debugFile, "EXIT\n");
				fclose(debugFile);
			}
            // Post the quit message to handle it later
			return;
        }

        TranslateMessage( &msg );
		DispatchMessage( &msg );
		debugFile = fopen("LDViewDebug.txt", "a+");
		if (debugFile)
		{
			fprintf(debugFile, "Message: 0x%08X\n", msg.message);
			fclose(debugFile);
		}
		if (msg.message == WM_DESTROY)
		{
			debugFile = fopen("LDViewDebug.txt", "a+");

			if (debugFile)
			{
				fprintf(debugFile, "WM_DESTROY\n");
				fclose(debugFile);
			}
			return;
//			PostQuitMessage(0);
		}
    }
*/
}

int doPreview(HINSTANCE hInstance, LPSTR lpCmdLine)
{
	char *spot = strchr(lpCmdLine, ' ');

	debugOut("Command line: %s\n", lpCmdLine);
	if (spot)
	{
		HWND hParentWindow;

		spot++;
		if (sscanf(spot, "%d", &hParentWindow) == 1)
		{
			SSPreview* ssPreview = new SSPreview(hParentWindow, hInstance);

			if (ssPreview->run())
			{
				return 0;
			}
			else
			{
				return 1;
			}
		}
	}
	return 1;
}

static void setupUserDefaults(LPSTR lpCmdLine, bool screenSaver)
{
	char *appName = "Travis Cobbs/LDView";
	char *sessionName;

	TCUserDefaults::setCommandLine(lpCmdLine);
	if (screenSaver)
	{
		appName = "Travis Cobbs/LDView Screen Saver";
	}
	TCUserDefaults::setAppName(appName);
	// Set the debug level before selecting a pref set.
	setDebugLevel((int)TCUserDefaults::longForKey(DEBUG_LEVEL_KEY, false));
	sessionName =
		TCUserDefaults::getSavedSessionNameFromKey(PREFERENCE_SET_KEY);
	if (sessionName && sessionName[0])
	{
		TCUserDefaults::setSessionName(sessionName);
	}
	delete sessionName;
}

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/,
				   LPSTR lpCmdLine, int nCmdShow)
{
	ModelLoader* modelLoader;
	bool screenSaver = isScreenSaver();
	int retValue;
//	TCDictionary* testDict = new TCDictionary;

	setupUserDefaults(lpCmdLine, screenSaver);
	if (screenSaver)
	{
		if (strncasecmp(lpCmdLine, "/p", 2) == 0 ||
			strncasecmp(lpCmdLine, "-p", 2) == 0 ||
			strncasecmp(lpCmdLine, "p", 1) == 0)
		{
			// preview mode
			return doPreview(hInstance, lpCmdLine);
		}
		if (strncasecmp(lpCmdLine, "/c", 2) == 0 ||
			strncasecmp(lpCmdLine, "-c", 2) == 0 ||
			strncasecmp(lpCmdLine, "c", 1) == 0 ||
			strlen(lpCmdLine) == 0)
		{
			SSConfigure *configure;

			configure = new SSConfigure(hInstance);
#ifdef _DEBUG
			_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
			createConsole();
#endif // _DEBUG
			configure->run();
			// configure mode
//			MessageBox(NULL, "This screen saver has no options that you can set.", "LDView screen saver", MB_OK | MB_ICONWARNING);
			return 1;
		}
		// This shouldn't be necessary, but I've received a report of a whole
		// bunch of copies of the LDView screensaver running at once.  This
		// might not fix things entirely, but it will at least prevent it
		// from launching multiple times concurrently.
		CreateMutex(NULL, FALSE, "LDView Screensaver");
		if (GetLastError() == ERROR_ALREADY_EXISTS)
		{
			return 0;
		}
	}
#ifdef _DEBUG
	int _debugFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	_debugFlag |= _CRTDBG_LEAK_CHECK_DF;
	_CrtSetDbgFlag(_debugFlag);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	createConsole();
#endif // _DEBUG
#ifdef _LOG_PERFORMANCE
	LARGE_INTEGER frequency;
	if (QueryPerformanceFrequency(&frequency))
	{
		debugPrintf("Frequency: %I64d\n", frequency.QuadPart);
	}
#endif // _LOG_PERFORMANCE
//	_CrtDbgReport(_CRT_WARN, NULL, 0, NULL, "Debug test: %d\n", testDict);
//	testDict->release();
	OleInitialize(NULL);
	modelLoader = new ModelLoader(hInstance, nCmdShow, screenSaver);
/*
	if (screenSaver)
	{
		debugPrintf("Screen Saver.\n");
		debugPrintf("Command Line: <<%s>>\n", lpCmdLine);
		debugPrintf("hPrevInstance: 0x%08x\n", hPrevInstance);
	}
*/
	retValue = mainLoop(hInstance);
	modelLoader->release();
	return retValue;
} // WinMain