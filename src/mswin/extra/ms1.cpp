#include "../winio/debug.h"

#include <stdio.h>
#include <string.h>
#include <windows.h>
#pragma hdrstop
#include <ddeml.h>
#include <dde.h>
#include <windowsx.h>
extern "C" {
extern void HandleOutput(const char*);
extern DWORD          idInst;            /*  Instance of app for DDEML       */
 HCONV          hConvExtra;
}
static HSZ            hszService;
static HSZ            hszTopic;
static HSZ            hszItem;
static HDDEDATA       hData;
static DWORD          dwResult;
static WORD           wFmt = CF_TEXT;         /*  Clipboard format                */
static char           szDDEData[256];          /*  Local receive data buffer       */

int start() {
HandleOutput("hel2mos start-\n");
	if (!idInst) {
		MessageBox(NULL, "idInst is 0", "ochelp start", MB_OK);
		return 0;
	}
		hszService = DdeCreateStringHandle ( idInst, "NETSCAPE", CP_WINANSI );
		hszTopic = DdeCreateStringHandle ( idInst, "WWW_OpenURL", CP_WINANSI );
		hszItem = DdeCreateStringHandle ( idInst, "NetscapeData", CP_WINANSI );
		hConvExtra = DdeConnect ( idInst, hszService, hszTopic,
					(PCONVCONTEXT) NULL );
		if (!hConvExtra) {
			MessageBox(NULL, "Conversation not established with NETSCAPE",
				"ochelp start", MB_OK);
			return 0;
		}
HandleOutput("Conversation established with NETSCAPE\n");
		return 1;
}

void stop() {
HandleOutput("hel2mos stop-\n");
	if (hConvExtra) {
		DdeDisconnect ( hConvExtra );
	}
	if (idInst) {
		DdeFreeStringHandle ( idInst, hszService );
		DdeFreeStringHandle ( idInst, hszTopic );
		DdeFreeStringHandle ( idInst, hszItem );
	}
}


static void take(HCONV hc, const char* name) {
	if ( hc != (HCONV)NULL ){
		DdeFreeStringHandle(idInst, hszItem);
		sprintf(szDDEData, "%s,,0xFFFFFFFF,0x0,,", name);
		hszItem = DdeCreateStringHandle ( idInst, (LPTSTR)szDDEData, CP_WINANSI );

		hData = DdeClientTransaction ( NULL, 0, hc,
				 hszItem, wFmt, XTYP_REQUEST, 1000, &dwResult );
		if (!hData) {
			MessageBox(NULL, szDDEData, "WWW_OpenURL failed", MB_OK);
		}
	}else{
		HandleOutput( "A connection to Netscape via DDE is not established." );
	}
}

void send(const char* url) {
//	HandleOutput("hel2mos %s\n", url);
	take(hConvExtra, url);
}

