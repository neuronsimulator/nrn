/* Borland C++ - (C) Copyright 1994 by Borland International               */

/***************************************************************************

      Program Name      DDESrvr.c

      Purpose           A simple DDE client application, which communicates
                        to a DDE server using the new 3.1 api DDEML calls.

                        To use this program, build DDEClnt and DDESrvr. There
                        are project files for this.

****************************************************************************/

#define STRICT

#include <windows.h>
#pragma hdrstop
#include <ddeml.h>
#include <dde.h>
#include <windowsx.h>

#include <stdio.h>
#include <string.h>

#include "ddesrvr.h"

#include "nrnbbs.h"
void nrnbbs_server_post(const char* name, const char* value);
void nrnbbs_server_take(const char* name, char* value);
void nrnbbs_server_admin_post(const char* name, const char* value);
void nrnbbs_server_admin_request(const char* name, char* value);
void nrnbbs_show_postings();

HANDLE         hInst;                  /*  Current instance of application */
HWND           hWnd;                   /*  Handle of Main window           */

int            xScreen;                /* Screen metrics                   */
int            yScreen;                /*  ...                             */
int            yFullScreen;            /*  ...                             */
int            xFrame;                 /*  ...                             */
int            yMenu;                  /*  ...                             */
TEXTMETRIC     tm;                     /* Text metrics                     */
int            cxChar;                 /* Character metrics                */
int            cyChar;                 /*  ...                             */

char           szScreenText[10][80];   /* Contains 10 lines of display data*/
int            cCurrentLine;           /* Index into szScreenText          */
int            cTotalLines;            /* Total lines in szScreenText      */

/*
         The DDE variables
*/

DWORD          idInst = 0L;            /*  Instance of app for DDEML       */
FARPROC        lpDdeProc;              /*  DDE callback function           */
HSZ            hszService;
HSZ            hszAdmin;
HSZ            hszUser;
HCONV          hConvApp = (HCONV)NULL; /*Handle of established conversation*/
char           szDDEData[256];          /*  Local receive buffer            */
char           szDDEString[256];        /*  Local send buffer               */
int            iServerCount = 0;       /*  Send message counter            */
char           tbuf[100];                /*  Temporary buffer for count      */

char szAppName[] = "DDEServerApplication";


/***************************************************************************/

#pragma argsused
int PASCAL WinMain ( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpszCmdLine, int nCmdShow )
{
  MSG         msg;


	if ( !hPrevInstance ){               /* Other instances of app running?  */
		if ( !InitApplication ( hInstance ) ){ /* Initialize shared things    */
         return ( FALSE );             /* Exits if unable to initialize    */
		}
	}
	if ( !InitInstance ( hInstance, nCmdShow ) )
      return ( FALSE );
  nrnbbs_connect();
  while ( GetMessage ( &msg, NULL, 0, 0 ) )
  {
    TranslateMessage ( &msg );
    DispatchMessage ( &msg );
  }
	DdeUninitialize ( idInst );
  nrnbbs_disconnect();
  return ( msg.wParam );
}

/***************************************************************************/

BOOL FAR PASCAL InitApplication ( HANDLE hInstance )
{
  WNDCLASS    wc;

   wc.style         = CS_HREDRAW | CS_VREDRAW;
   wc.lpfnWndProc   = MainWndProc;
   wc.cbClsExtra    = 0;
   wc.cbWndExtra    = 0;
   wc.hInstance     = hInstance;
   wc.hIcon         = LoadIcon ( hInstance, "DDEServerIcon" );
   wc.hCursor       = LoadCursor ( NULL, IDC_ARROW );
   wc.hbrBackground = GetStockObject ( WHITE_BRUSH );
   wc.lpszMenuName  = "DDEServerMenu";
   wc.lpszClassName = szAppName;

	if ( !RegisterClass ( &wc ) )
      return ( FALSE );

   return ( TRUE );
}

/***************************************************************************/

BOOL InitInstance ( HANDLE hInstance, int nCmdShow )
{
   hInst = hInstance;

   xScreen     = GetSystemMetrics ( SM_CXSCREEN );
   yScreen     = GetSystemMetrics ( SM_CYSCREEN );

  hWnd = CreateWindow ( szAppName,
								 "NrnBBS Server Window",
                         WS_OVERLAPPEDWINDOW,
                         190,                   /* These co-ordinates look */
                         yScreen / 2 - 20,      /* good on a VGA monitor   */
                         xScreen - 200,         /* running in 640x480.  No */
                         yScreen / 2 - 50,      /* combination was tried.  */
                         NULL,
                         NULL,
                         hInstance,
                         NULL );

/*
      If window could not be created, return "failure"
*/

   if ( !hWnd )
      return ( FALSE );

/*
      Make the window visible; update its client area; and return "success"
*/
#if 1
	ShowWindow ( hWnd, nCmdShow );      /* Show the window */
#else
	ShowWindow ( hWnd, SW_HIDE );
#endif
	UpdateWindow ( hWnd );              /* Sends WM_PAINT message           */
   return ( TRUE );              /* Returns the value from PostQuitMessage */

}

/***************************************************************************/

#pragma warn -eff
LRESULT CALLBACK _export MainWndProc ( HWND hWnd, UINT message,
                               WPARAM wParam, LPARAM lParam )
{
   HDC            hDC;
   PAINTSTRUCT    ps;
   DLGPROC        dlgProcAbout;
   int            i;
   int            j;
   int            y;


   switch ( message )
   {
      case WM_CREATE:
         hDC = GetDC ( hWnd );

         GetTextMetrics ( hDC, &tm );
         cxChar = tm.tmAveCharWidth;
         cyChar = tm.tmHeight + tm.tmExternalLeading;

         ReleaseDC ( hWnd, hDC );

         lpDdeProc = MakeProcInstance ( (FARPROC) DDECallback, hInst );
         if ( DdeInitialize ( (LPDWORD)&idInst, (PFNCALLBACK)lpDdeProc,
                              APPCLASS_STANDARD, 0L ) )
         {
            HandleOutput ( "DDE initialization failure." );
            return ( FALSE );
         }
         else
         {
				hszService = DdeCreateStringHandle ( idInst, "NrnBBS", CP_WINANSI );
				hszAdmin = DdeCreateStringHandle ( idInst, "Admin", CP_WINANSI );
				hszUser = DdeCreateStringHandle ( idInst, "User", CP_WINANSI );
				hConvApp = DdeConnect ( idInst, hszService, 0L,
					(PCONVCONTEXT) NULL );
				if ( hConvApp ){
					MessageBox(NULL, "NrnBBS server already running", "NrnBBS", MB_OK);
					DdeDisconnect(hConvApp);
					hConvApp = 0;
					DestroyWindow(hWnd);
					break;
				}

				DdeNameService ( idInst, hszService, (HSZ) NULL, DNS_REGISTER );
         }

         cCurrentLine = 0;
         cTotalLines = 0;
			break;

      case WM_COMMAND:
   switch ( GET_WM_COMMAND_ID(wParam, lParam) )
   {
		case IDM_EXIT:
         DestroyWindow ( hWnd );
			break;
      case IDM_SHOW_CONNECTIONS:
			nrnbbs_show_postings();
			break;
		default:
			return ( DefWindowProc ( hWnd, message, wParam, lParam ) );
	}
	break;

		case WM_PAINT:
	hDC = BeginPaint ( hWnd, &ps );

	y = 0;

	for ( i = 0; i < cTotalLines; i ++ )
	{
		if ( cTotalLines == 8 )
			j = ( (cCurrentLine + 1 + i) % 9 );
		else
					j = i;

				TextOut ( hDC, 0, y, (LPSTR)(szScreenText[j]),
											lstrlen ( szScreenText[j] ) );
				y = y + cyChar;
			}

			EndPaint ( hWnd, &ps );
			break;

		case WM_DESTROY:
			if ( hConvApp != (HCONV)NULL )
	{
				DdeDisconnect ( hConvApp );
				hConvApp = (HCONV)NULL;
			}
			DdeFreeStringHandle ( idInst, hszService );
			DdeFreeStringHandle ( idInst, hszAdmin );
			DdeFreeStringHandle ( idInst, hszUser );

			FreeProcInstance ( lpDdeProc );

			PostQuitMessage ( 0 );
			break;

		default:
			return ( DefWindowProc ( hWnd, message, wParam, lParam ) );
	}

	return ( FALSE );
}
#pragma warn .eff

/***************************************************************************/


#pragma argsused
HDDEDATA EXPENTRY _export DDECallback ( WORD wType, WORD wFmt, HCONV hConv, HSZ hsz1,
        HSZ hsz2, HDDEDATA hData, DWORD dwData1,
        DWORD dwData2 )
{
   switch ( wType )
   {
      case XTYP_CONNECT:
			if ( hsz2 == hszService ){
				return ( (HDDEDATA) TRUE );
			}else{
				HandleOutput ( "XTYP_CONNECT: hsz2 != hszService" );
            return ( (HDDEDATA) FALSE );
         }

		case XTYP_REQUEST:   /* hsz1 topic ; hsz2 item */
			DdeQueryString(idInst, hsz2, szDDEData, 40, CP_WINANSI);
			if (hsz1 == hszUser) {
				nrnbbs_server_take(szDDEData, szDDEString);
			}else if (hsz1 == hszAdmin) {
				nrnbbs_server_admin_request(szDDEData, szDDEString);
			}else{
				HandleOutput("Request for invalid Topic.");
				return ( (HDDEDATA)NULL );
  			}
			hData = DdeCreateDataHandle ( idInst, (LPBYTE) szDDEString,
						sizeof ( szDDEString ), 0L, hsz1, wFmt, 0 );

			if ( hData != (HDDEDATA)NULL ){
				return ( hData );
			}else{
				HandleOutput ( "Could not create data handle." );
				return ( (HDDEDATA)NULL );
			}

		case XTYP_EXECUTE:
			break;

		case XTYP_POKE: /* hsz1 topic; hsz2 item */
				DdeGetData ( hData, (LPBYTE) szDDEData, 80L, 0L );

				if ( szDDEData != NULL ){
					DdeQueryString(idInst, hsz2, szDDEString, 40, CP_WINANSI);
					if (hsz1 == hszUser) {
						nrnbbs_server_post(szDDEString, szDDEData);
					}else if (hsz1 == hszAdmin) {
						nrnbbs_server_admin_post(szDDEString, szDDEData);
					}else{
						return ( (HDDEDATA) NULL );
					}
					DdePostAdvise(idInst, hsz1, hsz2);
					return ( (HDDEDATA) DDE_FACK );
				}
				return ( (HDDEDATA) NULL );
		case XTYP_CONNECT_CONFIRM:
			HandleOutput ( "DDE connection confirmed." );
			hConvApp = hConv;
			break;

		case XTYP_DISCONNECT:
			hConvApp = (HCONV)NULL;
			HandleOutput ( "A client has disconnected." );
			break;

		case XTYP_ADVREQ:
			hData = DdeCreateDataHandle ( idInst, (LPBYTE) szDDEString,
//						sizeof ( szDDEString ), hsz1, hsz2, wFmt, 0 );
						sizeof ( szDDEString ), 0L, hsz2, wFmt, 0 );
			if ( hData != (HDDEDATA)NULL ){
				return ( hData );
			}else{
				HandleOutput ( "Could not create data handle." );
				return ( (HDDEDATA)NULL );
			}

		case XTYP_ADVSTART:
  			 return TRUE;

		case XTYP_ERROR:
			break;
	}

	return ( (HDDEDATA) NULL );
}

/***************************************************************************/

void HandleOutput ( char *szOutputString )
{
//DebugMessage("%s\n", szOutputString);
	strcpy ( szScreenText[cCurrentLine], szOutputString );
   cCurrentLine = ( cCurrentLine + 1 ) % 9;
   if ( cTotalLines < 8 )
      cTotalLines++;

   InvalidateRect ( hWnd, NULL, TRUE );
   UpdateWindow ( hWnd );
}
