/* Borland C++ - (C) Copyright 1992 by Borland International               */

/*
         External References
*/

extern HANDLE      hInst;
extern HWND        hWnd;

/*
         Resource Defines
*/

#define ID_DDECLIENT                   100
#define ID_DDESERVER                   101

/*
         Menu Selection Defines
*/

#define IDM_EXIT                       100
#define IDM_SHOW_CONNECTIONS           101
#define IDM_ABOUT                      103

/*
         Forward References
*/

LRESULT CALLBACK MainWndProc ( HWND, UINT, WPARAM, LPARAM );
BOOL FAR PASCAL InitApplication ( HANDLE );
BOOL InitInstance ( HANDLE hInstance, int nCmdShow );
BOOL CALLBACK About ( HWND, UINT, WPARAM, LPARAM );
HDDEDATA EXPENTRY DDECallback ( WORD, WORD, HCONV, HSZ, HSZ, HDDEDATA, DWORD,
                                DWORD );
void HandleOutput ( char * );

