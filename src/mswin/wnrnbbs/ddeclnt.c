
#define STRICT

#include <windows.h>
#pragma hdrstop
#include <ddeml.h>
#include <dde.h>
#include <windowsx.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ddeclnt.h"
#include "nrnbbs.h"

/*
         The DDE variables
*/

DWORD          idInst = 0L;            /*  Instance of app for DDEML       */
FARPROC        lpDdeProc;              /*  DDE callback function           */
HSZ            hszService;
HSZ            hszAdmin;
HSZ            hszUser;
HSZ            hszItem;
HCONV          hConv = (HCONV)NULL;    /*Handle of established conversation*/
HCONV				hConvAdmin = (HCONV)NULL;
HDDEDATA       hData;
DWORD          dwResult;
WORD           wFmt = CF_TEXT;         /*  Clipboard format                */
char           szDDEString[256];        /*  Local allocation of data buffer */
char           szDDEData[256];          /*  Local receive data buffer       */
int            iClientCount = 0;       /*  Client to Server message counter*/
char           tbuf[5];					 /*  Temporary, to hold count        */

/***********/
static void post(HCONV hc, const char* name);

void nrnbbs_post(const char* name) {
	sprintf(szDDEString, "");
	post(hConv, name);
}
void nrnbbs_post_int(const char* name, int i) {
	sprintf(szDDEString, "%d", i);
	post(hConv, name);
}
void nrnbbs_post_string(const char* name, const char* data) {
	sprintf(szDDEString, "%s", data);
	post(hConv, name);
}

static BOOL take(HCONV hc, const char* name, BOOL look);

BOOL nrnbbs_take(const char* name){
	return take(hConv, name, 0);
}
BOOL nrnbbs_look(const char* name){
	return take(hConv, name, 1);
}
BOOL nrnbbs_take_int(const char* name, int* pi) {
	int ok;
	ok = nrnbbs_take(name);
	if (ok) {
		sscanf(szDDEData+1, "%d", pi);
	}
	return ok;
}

BOOL nrnbbs_take_string(const char* name, char* val) {
	int ok;
	ok = nrnbbs_take(name);
	if (ok) {
		strcpy(val, szDDEData+1);
	}
	return ok;
}

void nrnbbs_exec(const char* cmd) {
	WinExec(cmd, SW_SHOW);
}

//void nrnbbs_callback(NrnBBSCallback f, const char* trigger) {

/***************************************************************************/
static NrnBBSCallback cbfunc_;

void nrnbbs_notify(const char* name, NrnBBSCallback f) {
	cbfunc_ = f;
	DdeFreeStringHandle(idInst, hszItem);
	hszItem = DdeCreateStringHandle ( idInst, (LPTSTR)name, CP_WINANSI );
	hData = DdeClientTransaction ( NULL, 0, hConv,
		hszItem, wFmt, XTYP_ADVSTART, 1000, &dwResult );
	if (!hData) {
		HandleOutput("notify failed\n");
	}
}

static int started;
BOOL nrnbbs_connected() {
	return (idInst && hConv) ? 1 : 0;
}

BOOL nrnbbs_connect() {
	if (!idInst) {
		lpDdeProc = MakeProcInstance ( (FARPROC) DDECallback, hInst );
		if ( DdeInitialize ( (LPDWORD)&idInst, (PFNCALLBACK)lpDdeProc,
										APPCMD_CLIENTONLY, 0L ) ){
			HandleOutput ( "Client DDE initialization failure.\n" );
			return ( FALSE );
		}

		hszService = DdeCreateStringHandle ( idInst, "NrnBBS", CP_WINANSI );
		hszAdmin = DdeCreateStringHandle ( idInst, "Admin", CP_WINANSI );
		hszUser = DdeCreateStringHandle ( idInst, "User", CP_WINANSI );
		hszItem = DdeCreateStringHandle ( idInst, "DDEData", CP_WINANSI );
	}
	while ( hConv == (HCONV)NULL ){
		hConv = DdeConnect ( idInst, hszService, hszUser,
					(PCONVCONTEXT) NULL );
		if ( hConv == (HCONV)NULL ){
			HandleError ( DdeGetLastError ( idInst ) );
			HandleOutput ( "Unsuccessful connection.\n" );
			if (started++ == 0) {
				char buf[256];
				HandleOutput("Starting NrnBBS server.\n");
//            WinExec("nrnbbs", SW_HIDE);
				sprintf(buf, "%s\\bin\\nrnbbs", getenv("NEURONHOME"));
				WinExec(buf, SW_MINIMIZE);
			}
			if (started < 5) {
         	Sleep(500);
				HandleOutput("Retrying connection.\n");
			}else{
				started = 0;
				return FALSE;
			}
		}else{
			HandleOutput ( "Successful connection.\n" );
		}
	}
	return TRUE;
}

void nrnbbs_disconnect() {
	if ( hConv != (HCONV)NULL ){
		DdeDisconnect ( hConv );
		hConv = (HCONV)NULL;
		if (hConvAdmin != (HCONV)NULL) {
			DdeDisconnect(hConvAdmin);
			hConvAdmin = (HCONV)NULL;
		}
		HandleOutput ( "Disconnected from server.\n" );
	}
	if (idInst) {
		DdeFreeStringHandle ( idInst, hszService );
		DdeFreeStringHandle ( idInst, hszAdmin );
		DdeFreeStringHandle ( idInst, hszUser );
		DdeFreeStringHandle ( idInst, hszItem );
		FreeProcInstance ( lpDdeProc );
		HandleOutput("DdeUninitialize\n");
		DdeUninitialize ( idInst );
	}
}

static void post(HCONV hc, const char* name) {
	if ( hc != (HCONV)NULL ){
		DdeFreeStringHandle(idInst, hszItem);
		hszItem = DdeCreateStringHandle ( idInst, (LPTSTR)name, CP_WINANSI );

		hData = DdeCreateDataHandle ( idInst, (LPBYTE) szDDEString,
			sizeof ( szDDEString ), 0L, hszItem, wFmt, 0 );

		if ( hData != (HDDEDATA)NULL ) {
		  hData = DdeClientTransaction ( (LPBYTE)hData, -1, hc,
				 hszItem, wFmt, XTYP_POKE, 1000, &dwResult );
		}else{
         HandleOutput( "Could not create data handle.\n" );
		}
	}else{
      HandleOutput ( "A connection to a DDE Server has not been established.\n" );
	}
}

static BOOL take(HCONV hc, const char* name, BOOL look) {
	if ( hc != (HCONV)NULL ){
		DdeFreeStringHandle(idInst, hszItem);
		sprintf(szDDEData, "%d%s", look, name);
		hszItem = DdeCreateStringHandle ( idInst, (LPTSTR)szDDEData, CP_WINANSI );

		hData = DdeClientTransaction ( NULL, 0, hc,
             hszItem, wFmt, XTYP_REQUEST, 1000, &dwResult );

		if ( !hData ){
			HandleOutput ( "Data not available from server.\n" );
		}else{
         DdeGetData ( hData, (LPBYTE) szDDEData, 80L, 0L );

			if ( szDDEData != NULL ){
				int ok;
				ok = szDDEData[0] - '0';
				return ok;
			}else{
				HandleOutput ( "Message from server is null.\n" );
			}
      }
	}else{
		HandleOutput ( "A connection to a DDE Server has not been established.\n" );
	}
	return 0;
}

void nrnbbs_wait(BOOL* pflag) {
	MSG msg;
	BOOL f = FALSE;
	BOOL* pf;
	pf = (pflag) ? pflag : &f;
	while (!(*pf) && nrnbbs_connected()) {
		f = TRUE; // once only if no arg
		while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}
		
/***************************************************************************/
#if defined(__MWERKS__)
#undef _export
#define _export /**/
#endif

#pragma argsused
HDDEDATA EXPENTRY _export DDECallback ( WORD wType, WORD wFmt, HCONV hConvX, HSZ hsz1,
		  HSZ hsz2, HDDEDATA hData, DWORD dwData1,
		  DWORD dwData2 )
{
   switch ( wType )
   {
      case XTYP_DISCONNECT:
         hConv = (HCONV)NULL;
         HandleOutput ( "The server forced a disconnect.\n" );
         return ( (HDDEDATA) NULL );

      case XTYP_ERROR:
         break;

		case XTYP_XACT_COMPLETE:
         // compare transaction identifier, indicate transaction complete
         break;
		case XTYP_ADVDATA:
			if (cbfunc_){
				char buf[100];
				DdeQueryString(idInst, hsz2, buf, 100, CP_WINANSI);
				(*cbfunc_)(buf);
			}
	}

   return ( (HDDEDATA) NULL );
}

/***************************************************************************/

void HandleError ( DWORD DdeError )
{
   switch ( DdeError )
   {
      case DMLERR_DLL_NOT_INITIALIZED:
			HandleOutput ( "DLL not initialized.\n" );
         break;

      case DMLERR_INVALIDPARAMETER:
			HandleOutput ( "Invalid parameter.\n" );
         break;

      case DMLERR_NO_CONV_ESTABLISHED:
			HandleOutput( "No conversation established.\n" );
         break;

      case DMLERR_NO_ERROR:
			HandleOutput ( "No error.\n" );
         break;
   }
}

