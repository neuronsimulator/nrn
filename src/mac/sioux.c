/* Metrowerks Standard Library
 * Copyright © 1995-2001 Metrowerks Corporation.  All rights reserved.
 *
 * $Date: 2003-02-11 19:36:05 +0100 (Tue, 11 Feb 2003) $
 * $Revision: 3 $
 */

/*******************************************************************************/
/*  Project...: C++ and ANSI-C Compiler Environment                     ********/
/*  Purpose...: Interface functions to SIOUX			                ********/
/*******************************************************************************/

#include <SIOUX.h> 

#include "SIOUXGlobals.h"
#include "SIOUXMenus.h"
#include "SIOUXWindows.h"

#if SIOUX_USE_WASTE
	#include "LongControls.h"		/* ¥¥¥LC  */
#endif /* SIOUX_USE_WASTE */

#include <console.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unix.h>

#include <AppleEvents.h>
#include <Dialogs.h>
#include <DiskInit.h>
#include <Fonts.h> 		
#include <Sound.h>      /*- mm 971006 -*/
#include <ToolUtils.h>
#include <Traps.h>


/*	defined in unix.c ...*/
#ifdef __cplusplus
	extern "C" {
#endif

#if ! __MSL__
	extern char	__system7present;
	extern void	__CheckForSystem7(void);
	extern void (*__RemoveConsoleHandler__)(void);
#else
	int  __system7present(void);
#endif /* ! __MSL__ */

/*extern long __myraise(long signal);   now an inline */
void hoc_sioux_show(int);

#ifdef __cplusplus
	}
#endif

/*	Local Defines*/
#define SIOUX_BUFSIZ 512U					/*	SIOUX's internal buffer size ...*/
#define TICK_DELTA 30UL						/*	Max ticks allowed between tests for UserBreak()*/
/*#define EOF (-1L)							/*	End Of File marker */

/*	Local Globals */
static Boolean toolBoxDone     = false;		/*	set to true when the TB is setup */
static Boolean appleEventsDone = false;		/*  set to true when the Apple Events are installed */
static unsigned long LastTick  = 0UL;		/*	The tickcount of the last UserBreak test ...*/
static short inputBuffersize   = 0;			/*	Used by GetCharsFromSIOUX and DoOneEvent ...*/
static CursHandle iBeamCursorH = NULL;		/*	Contains the iBeamCursor ... */
static Boolean atEOF 		   = false;		/*	Is the stream at EOF? */

/*	add by jijun 4/97 */
#define HistLen		20
static short		histTE[HistLen+1]={0};
static short		pcount=0;

typedef struct tSIOUXBuffer 
{
	char		*startpos,	/* pointer to a block of memory which will serve as the buffer */
				*curpos,	/* pointer to insertion point in the buffer */
				*endpos;	/* pointer to end of text in the buffer */
	long		tepos;		/* current offset in TEHandle (0 if at end) */
} tSIOUXBuffer;
static tSIOUXBuffer SIOUXBuffer;

#define ZEROSIOUXBUFFER()		{															\
									SIOUXBuffer.curpos =									\
										SIOUXBuffer.endpos =								\
											SIOUXBuffer.startpos;							\
								}
									/* SIOUXBuffer.tepos = -1;									/*- mm 971229 -*/
#define CURRENTBUFSIZE()		(SIOUXBuffer.endpos - SIOUXBuffer.startpos)
#define CHECKFOROVERFLOW(c)		{															\
									if (CURRENTBUFSIZE() + (c) >= SIOUX_BUFSIZ)				\
										InsertSIOUXBuffer();								\
								}
#define DELETEFROMBUFFER(num)	{															\
									if (SIOUXBuffer.curpos != SIOUXBuffer.endpos)			\
										BlockMoveData(SIOUXBuffer.curpos,					\
												SIOUXBuffer.curpos - (num),					\
												SIOUXBuffer.endpos - SIOUXBuffer.curpos);	\
									SIOUXBuffer.endpos -= (num);							\
									SIOUXBuffer.curpos -= (num);							\
								}
/*- mm 970626 -*/
#define ROLLBACKBUFFER(num)		{															\
									SIOUXBuffer.curpos = SIOUXBuffer.endpos - (num); 		\
									SIOUXBuffer.endpos = SIOUXBuffer.curpos;		 		\
								}
/*- mm 970626 -*/
#if SIOUX_USE_WASTE
#define INSERTCHAR(c)			{															\
									if (SIOUXBuffer.tepos == -1) {							\
										*SIOUXBuffer.curpos = (c);							\
										if (SIOUXBuffer.curpos == SIOUXBuffer.endpos)		\
											SIOUXBuffer.endpos++;							\
										SIOUXBuffer.curpos++;								\
									} else {												\
										WEReference theTEH = SIOUXTextWindow->edit;			\
										WESetSelection(SIOUXBuffer.tepos,					\
													SIOUXBuffer.tepos + 1,					\
													theTEH);								\
										WEKey(c, nil, theTEH);								\
										SIOUXBuffer.tepos++;								\
										if (SIOUXBuffer.tepos == WEGetTextLength(theTEH ))	\
											SIOUXBuffer.tepos = -1;							\
									}														\
								}
										/*if (SIOUXBuffer.tepos == WEGetTextLength( theTEH ) - 1)	*/ /*- mm 971229 -*/
#else
#define INSERTCHAR(c)			{															\
									if (SIOUXBuffer.tepos == -1) {							\
										*SIOUXBuffer.curpos = (c);							\
										if (SIOUXBuffer.curpos == SIOUXBuffer.endpos)		\
											SIOUXBuffer.endpos++;							\
										SIOUXBuffer.curpos++;								\
									} else {												\
										TEHandle theTEH = SIOUXTextWindow->edit;			\
										TESetSelect(SIOUXBuffer.tepos,						\
													SIOUXBuffer.tepos + 1,					\
													theTEH);								\
										TEKey(c, theTEH);									\
										SIOUXBuffer.tepos++;								\
										if (SIOUXBuffer.tepos == (*theTEH)->teLength)	    \
											SIOUXBuffer.tepos = -1;							\
									}														\
								}
										/* if (SIOUXBuffer.tepos == (*theTEH)->teLength - 1)	*/  /*- mm 971229 -*/
#endif /* SIOUX_USE_WASTE */
#define INSERTLINEFEED()		{															\
									*SIOUXBuffer.curpos = 0x0d;								\
									SIOUXBuffer.curpos++;									\
									SIOUXBuffer.endpos = SIOUXBuffer.curpos;				\
									SIOUXBuffer.tepos = -1;									\
								}


/* Globals for Interviews */
extern Boolean IVOCGoodLine;
static Boolean saw_line = 1; // for oc> prompt

/************************************************************************/
/* Purpose..: Setup the toolbox			  								*/
/* Input....: ---                       								*/
/* Returns..: ---                        								*/
/************************************************************************/
static void DoSetupToolbox(void)
{
#if TARGET_API_MAC_OS8
	/*	Initialize the ToolBox (not necessary with Carbon)...*/
	InitGraf(&qd.thePort);
	InitFonts();
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(0L);
	FlushEvents(everyEvent, 0L);
	
	MaxApplZone();
#endif /* TARGET_API_MAC_OS8 */
	MoreMasters();
	
	InitCursor();
	toolBoxDone = true;
}

/************************************************************************/
/*	Purpose..: 	Determines position on current line	(in chars)			*/
/*	Input....:	TextEdit Handle											*/
/*	Return...:	Position on current line								*/
/************************************************************************/
#if SIOUX_USE_WASTE
	static long OffsetOnCurrentLine(WEReference theTEH)
#else
	static long OffsetOnCurrentLine(TEHandle theTEH)
#endif /* SIOUX_USE_WASTE */
{
	char *ptr, *start;
	long result;
#if SIOUX_USE_WASTE
	long i, lastCharIndex;
	SInt32 weSelStart, weSelEnd;
#endif /* SIOUX_USE_WASTE */
	
	/*	Check for a CR in the buffer ...*/
	if (SIOUXBuffer.endpos != SIOUXBuffer.startpos) 
	{
		for (start = SIOUXBuffer.startpos, ptr = SIOUXBuffer.endpos; ptr > start; ptr--)
			/*if (*ptr == 0x0d) return (SIOUXBuffer.endpos - ptr - 1);  /*- mm 970903 -*/
			if (ptr[-1] == 0x0d)                   					    /*- mm 980112 -*/
				return (SIOUXBuffer.endpos - ptr);  					/*- mm 980112 -*/
	}

#if SIOUX_USE_WASTE
	WEGetSelection( &weSelStart, &weSelEnd, theTEH );
	
	lastCharIndex = WEGetTextLength( theTEH ) - 1;
	i = 0;
	/*while ( ( lastCharIndex > weSelStart ) && WEGetChar( lastCharIndex - i, theTEH ) != 0x0d ) */ /*- mm 971230 -*/
	while ( ( lastCharIndex >= i ) && WEGetChar( lastCharIndex - i, theTEH ) != 0x0d )         /*- mm 971230 -*/
		/*i--;*/            /*- mm 971230 -*/
		i++;                /*- mm 971230 -*/
	
	/*result = weSelStart - i + ( SIOUXBuffer.endpos - SIOUXBuffer.startpos );*/  /*- mm 971230 -*/
	result = i + ( SIOUXBuffer.endpos - SIOUXBuffer.startpos);                    /*- mm 971230 -*/
#else
	HLock((Handle)theTEH);
	HLock((*theTEH)->hText);

	start = *(*theTEH)->hText;
	ptr = *(*theTEH)->hText + (*theTEH)->selStart;
	while (ptr > start && ptr[-1] != 0x0d)
		ptr--;
	
	result = *(*theTEH)->hText + (*theTEH)->selStart - ptr + SIOUXBuffer.endpos - SIOUXBuffer.startpos;
	
	HUnlock((*theTEH)->hText);
	HUnlock((Handle)theTEH);
#endif /* SIOUX_USE_WASTE */
	
	return result;
}

/************************************************************************/
/*	Purpose..:	Handle a mouseDown event								*/
/*	Input....:	pointer to an Event										*/
/*	Return...:	true/false												*/
/************************************************************************/
static Boolean HandleMouseDownEvent(EventRecord *theEvent)
{
	WindowPtr window;
	short part;
	Boolean isSIOUXwindow;
/*	Point theWhere; */
	
	part = FindWindow(theEvent->where, &window);
	isSIOUXwindow = SIOUXIsAppWindow(window);
	
	switch (part) 
	{
		case inMenuBar:
			if (SIOUXSettings.setupmenus) 
			{
				SIOUXUpdateMenuItems();
				/* jd 980923 - should update this to MenuEvent?? */
				SIOUXDoMenuChoice(MenuSelect(theEvent->where));
				return true;
			}
			break;
			
		#if TARGET_API_MAC_OS8     /*instead of !TARGET_API_MAC_CARBON*/  /*- cc 991111 -*/
		case inSysWindow:
			if (SIOUXSettings.standalone)
				SystemClick(theEvent, window);
			break;
		#endif /*- cc 991111 -*/
			
		case inContent:
			/*									mm 980605a 
			**	SIOUX should only select windows that it knows about so
			**	that the app embedding a SIOUx window can decide whether or
			**	not to select the window.
			*/
			if (isSIOUXwindow) 
			{

				if (window != FrontWindow())
					SelectWindow(window);
				else
				{
					if (SIOUXState == PRINTFING) 
					{
					if (StillDown())
							{
						while (WaitMouseUp())
							;	/*	Pause output while mouse is down ...*/
							}
				} else
					SIOUXDoContentClick(window, theEvent);
				}

				return true;
			}
			break;
			
		case inDrag:
			/*           mm 980605b
			**	SIOUX should select its window when the user clicks in
			**	the drag region if it's not the FrontWindow.
			*/
			if (isSIOUXwindow) 
			{
				if (window != FrontWindow())
					SelectWindow(window);
				else
				DragWindow(window, theEvent->where, &SIOUXBigRect);
				return true;
			}
			break;
			
		case inGrow:
			/*				mm 980605b
			**	SIOUX should select its window when the user clicks in
			**	the grow region if it's not the FrontWindow.
			*/
			if (isSIOUXwindow) 
			{
				if (window != FrontWindow())
					SelectWindow(window);
				else
				SIOUXMyGrowWindow(window, theEvent->where);
				return true;
			}
			break;
		}

	return false;
}

/************************************************************************/
/*	Purpose..:	Handle update and activate/deactivate events			*/
/*	Input....:	pointer to an Event										*/
/*	Return...:	true/false												*/
/************************************************************************/
static Boolean HandleUpdateActivateEvent(EventRecord *theEvent)
{
	if (SIOUXIsAppWindow((WindowPtr)theEvent->message)) 
	{
		if (theEvent->what == updateEvt)
			SIOUXUpdateWindow((WindowPtr)theEvent->message);
		else 
		{	/* must be an activate/deactivate event */
			if (theEvent->modifiers & activeFlag) 
			{
#if SIOUX_USE_WASTE
				WEActivate(SIOUXTextWindow->edit);
#else
				TEActivate(SIOUXTextWindow->edit);
#endif /* SIOUX_USE_WASTE */
				ShowControl(SIOUXTextWindow->vscroll);
			} 
			else 
			{
#if SIOUX_USE_WASTE
				WEDeactivate(SIOUXTextWindow->edit);
#else
				TEDeactivate(SIOUXTextWindow->edit);
#endif /* SIOUX_USE_WASTE */
				HideControl(SIOUXTextWindow->vscroll);
			}
			SIOUXDrawGrowBox(SIOUXTextWindow->window);	/*- ra 990612 -*/
			SIOUXUpdateStatusLine(SIOUXTextWindow->window);
		}
		return true;
	}

	return false;
}

/************************************************************************/
/*	Purpose..:	Handle update and activate/deactivate events			*/
/*	Input....:	pointer to an Event										*/
/*	Return...:	true/false												*/
/************************************************************************/
static Boolean HandleOSEvents(EventRecord *theEvent)
{
	switch ((theEvent->message >> 24) & 0xff) 
	{
		case resumeFlag:
			if (theEvent->message & suspendResumeMessage) 
			{
#if SIOUX_USE_WASTE
				WEActivate(SIOUXTextWindow->edit);
#else
				TEActivate(SIOUXTextWindow->edit);
#endif /* SIOUX_USE_WASTE */
				ShowControl(SIOUXTextWindow->vscroll);
			} 
			else 
			{
#if SIOUX_USE_WASTE
				WEDeactivate(SIOUXTextWindow->edit);
#else
				TEDeactivate(SIOUXTextWindow->edit);
#endif /* SIOUX_USE_WASTE */
				HideControl(SIOUXTextWindow->vscroll);
			}
			SIOUXDrawGrowBox(SIOUXTextWindow->window);	/*- ra 990612 -*/
			SIOUXUpdateStatusLine(SIOUXTextWindow->window);
			return(true);

		default:
			return(false);
	}
}

/************************************************************************/
/*	Purpose..:	Detect a user break (Command-'.')						*/
/*	Input....:	pointers an size of block								*/
/*	Return...:	true/false												*/
/************************************************************************/
static void UserBreak(void)
{
	EventRecord ev;

	if (TickCount() - LastTick < TICK_DELTA)
		return;

	LastTick = TickCount();

	if (SIOUXUseWaitNextEvent)
		WaitNextEvent(everyEvent, &ev, SIOUXSettings.sleep, NULL);
	else 
	{
	#if TARGET_API_MAC_OS8     /*instead of !TARGET_API_MAC_CARBON*/ /*- cc 991111 -*/
		/*	Keep the system happy ...*/
		SystemTask();
	#endif  /*- cc 991111 -*/

		GetNextEvent(everyEvent, &ev);
	}

	switch (ev.what) 
	{
		case nullEvent:		/*	ignore it ...*/
			break;

		case keyDown:
		case autoKey:		/*	check for break ...*/
			if (ev.modifiers & cmdKey) 
			{
				if ((ev.message & charCodeMask) == '.')
					__myraise(SIGINT);
				if ((ev.message & charCodeMask) == 'q' || (ev.message & charCodeMask) == 'Q') {
					SIOUXQuitting = true;
					__myraise(SIGINT);
				}
			}
			if ((ev.message & charCodeMask) == 0x03 && (ev.message & keyCodeMask) >> 8 != 0x4c)
				__myraise(SIGINT);			/*	enter and control c have same char code*/
			break;

		case mouseDown:
			HandleMouseDownEvent(&ev);
			break;

		case activateEvt:
		case updateEvt:
			HandleUpdateActivateEvent(&ev);
			break;

		case osEvt:
			HandleOSEvents(&ev);
			break;

		case kHighLevelEvent:
#if ! __MSL__
			if (__system7present == -1)
				__CheckForSystem7();
		
			if (__system7present)
#else
			if (__system7present())
#endif /* ! __MSL__ */
			{
				AEProcessAppleEvent(&ev);
			}
			break;

		#if TARGET_API_MAC_OS8     /*instead of !TARGET_API_MAC_CARBON*/  /*- cc 991111 -*/
		case diskEvt:
			if (HiWord(ev.message) != noErr) {
				Point pt = {100, 100};
				DIBadMount(pt, ev.message);
			}
			break;
		#endif  /*- cc 991111 -*/

		case mouseUp:
		case keyUp:
		default:
			break;
	}
}

/************************************************************************/
/*	Purpose..: 	Insert the current SIOUX buffer into the TE Handle		*/
/*	Input....:	---														*/
/*	Return...:	---														*/
/************************************************************************/
static void InsertSIOUXBuffer(void)
{
#if SIOUX_USE_WASTE
	WEReference theTEH = SIOUXTextWindow->edit;
	long teLength;
	
/* the following variables were used for the aborted error-handling code found later in this function --pcg */
/*	long weToDelete;
	SInt16 autoScrollFlag;
*/
	teLength = WEGetTextLength( theTEH );
#else
	TEHandle theTEH = SIOUXTextWindow->edit;
	short teLength;
	
	HLock((Handle)theTEH);
	teLength = (*theTEH)->teLength;
#endif /* SIOUX_USE_WASTE */
	
#if ! SIOUX_USE_WASTE
	/* this isn't necessary w/ WASTE  -pcg */
	if ((teLength + CURRENTBUFSIZE()) > 32767) 
	{
		/*	Insert will grow TEHandle past 32K so we cut out excess from top ...*/
		char *ptr;
		short todelete = (short) ((teLength + CURRENTBUFSIZE()) - 32767) + 8*SIOUX_BUFSIZ;
		
		/*	Make sure that the text to be cut ends on a CR ...*/
		HLock((*theTEH)->hText);
		for (ptr = *(*theTEH)->hText + todelete; *ptr != 0x0d; ptr++) ;

		/*	We now point at the CR, increment ptr to point after it ...*/
		todelete += ++ptr - (*(*theTEH)->hText + todelete);
		HUnlock((*theTEH)->hText);
		
		/*	We hit the fields directly to keep TE from redrawing twice*/
		(*theTEH)->selStart = 0;
		(*theTEH)->selEnd   = todelete;
		TEDelete(theTEH);

		/*	Now fix things up...*/
		teLength = (*theTEH)->teLength;
	}
#endif /* ! SIOUX_USE_WASTE */

	/*	Now insert the new text ...*/
#if SIOUX_USE_WASTE
	/* aborted code to handle WASTE buffer size limiting. I'll leave it here
	for future generations to try their hand at.  --pcg */
/*	if ( SIOUXSettings.wastemaxbuffersize > 0 )
	{
		if ( teLength + CURRENTBUFSIZE() > SIOUXSettings.wastemaxbuffersize )
		{
			autoScrollFlag = WEFeatureFlag( weFAutoScroll, weBitTest, theTEH );
			weToDelete = CURRENTBUFSIZE();
			while ( ( weToDelete < teLength ) && ( WEGetChar( weToDelete, theTEH ) != 0x0d ) )
				weToDelete++;
			WEFeatureFlag( weFAutoScroll, weBitClear, theTEH );
			WESetSelection( 0, weToDelete + 1, theTEH );
			WEDelete( theTEH );
			SIOUXUpdateScrollbar();
			LCSetValue( SIOUXTextWindow->vscroll, */								/*¥¥¥LC */
/*									LCGetMax( SIOUXTextWindow->vscroll ) );	*/	/*¥¥¥LC */
/*			AdjustText();
			WEFeatureFlag( weFAutoScroll, autoScrollFlag, theTEH );
			
			teLength = WEGetTextLength( theTEH );
		}
	}
*/
	WESetSelection( teLength, teLength, theTEH );
	WEInsert(SIOUXBuffer.startpos, CURRENTBUFSIZE(), nil, nil, theTEH);
	
	teLength = WEGetTextLength( theTEH );
#else
	TESetSelect(teLength, teLength, theTEH);
	TEInsert(SIOUXBuffer.startpos, CURRENTBUFSIZE(), theTEH);
	teLength = (*theTEH)->teLength;
#endif /* SIOUX_USE_WASTE */
	
	SIOUXTextWindow->dirty = true;
	ZEROSIOUXBUFFER();

#if SIOUX_USE_WASTE
	WESetSelection(teLength, teLength, theTEH);
#else
	TESetSelect(teLength, teLength, theTEH);

	HUnlock((Handle)theTEH);
#endif /* SIOUX_USE_WASTE */

//	if (SIOUXSettings.standalone)
//		UserBreak();
//	else
		SIOUXUpdateScrollbar();
}

/************************************************************************/
/*	Purpose..:	Determine the user's theoretical menuchoice				*/
/*	Input....:	Character typed											*/
/*	Return...:	Menuchoice												*/
/************************************************************************/
static long myMenuKey(char key)
{
	short theMenu = 0;
	short theMenuItem = 0;

	switch (key) 
	{
		/*	File menu choices*/
		case 's': case 'S':
			theMenu = FILEID;
			theMenuItem = FILESAVE;
			break;
			
		case 'p': case 'P':
			theMenu = FILEID;
			theMenuItem = FILEPRINT;
			break;
			
		case 'q': case 'Q':
			theMenu = FILEID;
			theMenuItem = FILEQUIT;
			break;
			
		case 'x': case 'X':
			theMenu = EDITID;
			theMenuItem = EDITCUT;
			break;
			
		case 'c': case 'C':
			theMenu = EDITID;
			theMenuItem = EDITCOPY;
			break;
			
		case 'v': case 'V':
			theMenu = EDITID;
			theMenuItem = EDITPASTE;
			break;
			
		case 'a': case 'A':
			theMenu = EDITID;
			theMenuItem = EDITSELECTALL;
			break;
	}

	return (((long)theMenu << 16) | theMenuItem);
}

/************************************************************************/
/*	Purpose..: 	Check if insertion range is in edit range				*/
/*	Input....:	first character in edit range							*/
/*	Input....:	Handle to textedit										*/
/*	Return...:	true/false												*/
/************************************************************************/
#if SIOUX_USE_WASTE
	Boolean SIOUXisinrange(long first, WEReference te)
#else
	Boolean SIOUXisinrange(short first, TEHandle te)
#endif /* SIOUX_USE_WASTE */
{
#if SIOUX_USE_WASTE
	SInt32 weSelStart, weSelEnd;
	
	WEGetSelection( &weSelStart, &weSelEnd, te );
	if ((weSelStart < first) || (weSelEnd < first) )
#else
	if (((*te)->selStart < first) || (*te)->selEnd < first)
#endif /* SIOUX_USE_WASTE */
	{
		SysBeep(10);
		return false;
	} else
		return true;
}

/************************************************************************/
/*	Purpose..: 	add the current command line to the history	*/
/*	Input....:	the line value of the command in the TextEdit	*/
/*	Input....:	old history list				*/
/*	add by jijun 4/97						*/
/************************************************************************/
static void Addhistl (short value, short histTE[]) {
			short		i,j;
						
			if (histTE[0] < HistLen) {
				histTE[++histTE[0]] = value;
			}else{
				for (j=1;j<HistLen; j++) {
					histTE[j]=histTE[j+1];
				}
				histTE[HistLen]=value;
			}
}

/************************************************************************/
/*	Purpose..: 	get the previous command line							*/
/*	Input....:	the times you hit the ^p								*/
/*	Input....:	history list											*/
/*	Return...:	the value you wanted									*/
/*	add by jijun 4/97													*/
/************************************************************************/
static short GetHist (short n, short histTE[]) {
		if (n>=histTE[0])
			return histTE[1];
		return histTE[histTE[0]-n+1];
}		

/************************************************************************/
/* Purpose..: Handles a single event ...  								*/
/* Input....: If non-zero then pointer to an event						*/
/* Returns..: ---           		             						*/
/************************************************************************/
short SIOUXHandleOneEvent(EventRecord *userevent)
{
	EventRecord theEvent;
	WindowPtr	window;
	char		aChar;
#if SIOUX_USE_WASTE
	LongRect weViewRect;
	long scrollAdjust, directionMultiplier;
#else
	short scrollAdjust, directionMultiplier;
#endif /* SIOUX_USE_WASTE */

	if (SIOUXState == OFF)
		return false;

	if (userevent)						/*	External call of the function ...*/
		theEvent = *userevent;
	else if (SIOUXUseWaitNextEvent)		/*	Internal with WNE allowed ...*/
		WaitNextEvent(everyEvent, &theEvent, SIOUXSettings.sleep, NULL);
	else 
	{								/*	Internal with no WNE allowed ...*/
		#if TARGET_API_MAC_OS8     /*instead of !TARGET_API_MAC_CARBON*/  /*cc 991111 */
		SystemTask();
		#endif   /*cc 991111 */
#if 1
/* share with interviews */
		EventAvail(everyEvent, &theEvent);
		FindWindow(theEvent.where, &window);
		if(((theEvent.what == updateEvt) || (theEvent.what == activateEvt)) &&
		   ((WindowPtr)(theEvent.message) != SIOUXTextWindow->window)){ 
		   	SIOUXState = IDLE;
		   	return true;

//Exact conditions need to be determined still ... we currently enter the 
//Interviews loop as soon as we leave the SIOUX window.  This can loose
//the current line.
		} else if(//((theEvent.what == mouseDown) || (theEvent.what == mouseUp)) &&
				  (window != SIOUXTextWindow->window)){
			SIOUXState = IDLE;
		   	return true;
		} else {
				GetNextEvent(everyEvent, &theEvent);
		}
#else				
		GetNextEvent(everyEvent, &theEvent);
#endif
	}

	window = FrontWindow();

	switch (theEvent.what) 
	{
		case nullEvent:
			/*	Maintain the cursor*/
			if (SIOUXIsAppWindow(window))
			{
				GrafPtr savePort;Point localMouse;		/* ¥¥¥MP  */
#if SIOUX_USE_WASTE
				Rect tempRect;
				LongRect weViewRect;
#endif /* SIOUX_USE_WASTE */

				GetPort(&savePort);
				SetPortWindowPort(window); /*- ra 990612 -*/
				localMouse = theEvent.where;GlobalToLocal(&localMouse);		/* ¥¥¥MP */
#if SIOUX_USE_WASTE
				WEGetViewRect( &weViewRect, SIOUXTextWindow->edit );
				WELongRectToRect( &weViewRect, &tempRect );
				if (PtInRect(localMouse, &tempRect) && iBeamCursorH )		/* ¥¥¥MP */
#else
				if (PtInRect(localMouse, &(*SIOUXTextWindow->edit)->viewRect) &&	/* ¥¥¥MP */
					iBeamCursorH)
#endif /* SIOUX_USE_WASTE */
				{
#if SIOUX_USE_WASTE
					WEAdjustCursor( theEvent.where, nil, SIOUXTextWindow->edit );
#else
					SetCursor(*iBeamCursorH);
#endif /* SIOUX_USE_WASTE */
				} 
				else 
				{
				#if TARGET_API_MAC_CARBON
					Cursor theArrow;
					SetCursor(GetQDGlobalsArrow(&theArrow));
				#else
					SetCursor(&qd.arrow);
				#endif /* TARGET_API_MAC_CARBON */
				}
				/*LocalToGlobal(&theEvent.where);	 */	/* ¥¥¥MP */
#if SIOUX_USE_WASTE
				WEAdjustCursor( theEvent.where, nil, SIOUXTextWindow->edit );
				WEIdle( nil, SIOUXTextWindow->edit );
#else
				TEIdle(SIOUXTextWindow->edit);
#endif /* SIOUX_USE_WASTE */
				SetPort(savePort);
				/*				mm 980605c
				**	SIOUX should return false so that an app embedding
				**	SIOUX that looks at the result of SIOUXHandleOneEvent()
				**	will be able to process the NULL event as well.
				*/
				return false;
			} 
			else 
			{
				if (SIOUXSettings.standalone)	/*- JWW 000531 -*/
				{
				#if TARGET_API_MAC_CARBON
					Cursor theArrow;
					SetCursor(GetQDGlobalsArrow(&theArrow));
				#else
					SetCursor(&qd.arrow);
				#endif /* TARGET_API_MAC_CARBON */
				
					if (SIOUXTextWindow != NULL)
#if SIOUX_USE_WASTE
						WEIdle( nil, SIOUXTextWindow->edit );
#else
						TEIdle(SIOUXTextWindow->edit);
#endif /* SIOUX_USE_WASTE */
				}
			}
			break;

		case mouseDown:
			if (HandleMouseDownEvent(&theEvent))
				return true;
			break;

		case keyDown:
		case autoKey:
			if (SIOUXIsAppWindow(window)) {
				aChar = (theEvent.message & charCodeMask);
				if ((theEvent.modifiers & cmdKey) && (aChar > 0x20)) 
				{	/*¥¥¥MP */
					/*	Check first for command - '.'*/
					if (SIOUXState != TERMINATED && aChar == '.')
						__myraise(SIGINT);
					if (SIOUXSettings.setupmenus) 
					{
						SIOUXUpdateMenuItems();
						SIOUXDoMenuChoice(MenuKey(aChar));
					} 
					else
						SIOUXDoMenuChoice(myMenuKey(aChar));
					return true;
				} else {
					if (((theEvent.message & keyCodeMask) >> 8 == 0x4c) ||                            /*- mm 980413 -*/
													((theEvent.message & keyCodeMask) >> 8 == 0x34))  /*- mm 980413 -*/
						aChar = 0x0d;				/*	map enter key to return key ...*/
					if (SIOUXState == SCANFING) {
						/*	If there are too many characters on the line already then just return ...*/
#if SIOUX_USE_WASTE
						if ((WEGetTextLength( SIOUXTextWindow->edit ) - SIOUXselstart + 1) >= inputBuffersize)
#else
						if (((*SIOUXTextWindow->edit)->teLength - SIOUXselstart + 1) >= inputBuffersize)
#endif /* SIOUX_USE_WASTE */
						{
							SysBeep(10);
							return false;
						}
#if 0 // the original
						switch (aChar) 
						{
							case 0x1a:				/*	Control - 'z'*/
							case 0x04:				/*	Control - 'd'*/
								/*	Place in the enter key char which will become the EOF*/
								aChar = 0x03;
							case 0x0d:				/*	Carriage Return*/
								SIOUXState = IDLE;
								break;
								
							case 0x03:				/*	Control - 'c'*/
								__myraise(SIGINT);
								break;
								
							case 0x08:				/*	Delete*/
								if (!SIOUXisinrange(SIOUXselstart + 1, SIOUXTextWindow->edit))
									return false;
								break;
								
							default:
								break;
						}
#else // emacs like line editing (from pro 5 working sioux.c
						switch (aChar) {
							case 0x1a:				//	Control - 'z'
								aChar = 0x03;
							case 0x03:				//	Control - 'c'
								__myraise(SIGINT);
								break;
							case 0x0d:				//	Carriage Return
								//jijun 4/4/97 put the cursor at the end of line whenever hit the return
								if ((*SIOUXTextWindow->edit)->selEnd!=(*SIOUXTextWindow->edit)->teLength)
								{	
									TESetSelect((*SIOUXTextWindow->edit)->teLength,
											(*SIOUXTextWindow->edit)->teLength,
											SIOUXTextWindow->edit);
								}
									// keep the track of the current line if it is not empty
								if ((*SIOUXTextWindow->edit)->selStart!=SIOUXselstart)
								{
									//histcount=Addhistl(ln,histTE);
									Addhistl((*SIOUXTextWindow->edit)->nLines,histTE);									
								}
								//sel = *(*psw->edit)->hText + SIOUXselstart; // for debugging
								pcount=0;
								TEKey(aChar, SIOUXTextWindow->edit);
								SIOUXUpdateScrollbar();
								SIOUXState = IDLE;
								//let Neuron know good line received.
								IVOCGoodLine = true;
								saw_line = true;
								aChar=0;								 
								break;
							case 0x08:				//	Delete Control - 'h'
								if (!SIOUXisinrange(SIOUXselstart + 1, SIOUXTextWindow->edit))
									return false;
								break;
								// add by jijun 4/2/97 - 4/4/97
							case 0x02:				// Backward a char Control-'b'
								if (!SIOUXisinrange(SIOUXselstart+1, SIOUXTextWindow->edit))
									return false;
								aChar = '\34';
								//teLength = (*SIOUXTextWindow->edit)->sel1; 
								//TESetSelect(teLength,teLength,SIOUXTextWindow->edit);
								break;
							case 0x06:				// Forward a char Control-'f'
								//if (!SIOUXisinrange(SIOUXselstart, SIOUXTextWindow->edit))
								//	return false;
								aChar = '\35';
								//teLength = (*SIOUXTextWindow->edit)->selEnd+1; 
								//TESetSelect(teLength,teLength,SIOUXTextWindow->edit);
								break;
							case 0x01:				// beginning of line Control-'a'
								TESetSelect(SIOUXselstart,SIOUXselstart,SIOUXTextWindow->edit);
								aChar=0;
								break;
							case 0x05:				// End of line Control-'e'
								TESetSelect((*SIOUXTextWindow->edit)->teLength,
											(*SIOUXTextWindow->edit)->teLength,
											SIOUXTextWindow->edit);
								aChar=0;
								break;
							case 0x04:				//	Control - 'd'
								//	delete a char after the cursor
								if (!SIOUXisinrange(SIOUXselstart, SIOUXTextWindow->edit))
									return false;
								TESetSelect((*SIOUXTextWindow->edit)->selEnd,
											(*SIOUXTextWindow->edit)->selEnd+1,
											SIOUXTextWindow->edit);
								TEDelete(SIOUXTextWindow->edit);
								aChar=0;
								break;
							case 0x0b:				//	Control - 'k'
								//	delete the line after the cursor
								if (!SIOUXisinrange(SIOUXselstart, SIOUXTextWindow->edit))
									return false;
								TESetSelect((*SIOUXTextWindow->edit)->selEnd,
											(*SIOUXTextWindow->edit)->teLength,
											SIOUXTextWindow->edit);
								TEDelete(SIOUXTextWindow->edit);
								aChar=0;
								break;
							case 0x10:				//	Control - 'p'								
								// select the previous command line in the textedit								
								pcount++;
								if (pcount>HistLen)
									pcount=HistLen;
								if (histTE[0])
								{ 
									TESetSelect((*SIOUXTextWindow->edit)->lineStarts[GetHist(pcount,histTE)-1]+3,
											(*SIOUXTextWindow->edit)->lineStarts[GetHist(pcount,histTE)]-1,
											SIOUXTextWindow->edit);
									TECopy(SIOUXTextWindow->edit);
									// move the cursor to the beginning of this line
									TESetSelect(SIOUXselstart,
											(*SIOUXTextWindow->edit)->teLength,
											SIOUXTextWindow->edit);
									TEPaste(SIOUXTextWindow->edit);
								}
								aChar=0;
								break;
							case 0x0e:				//	Control - 'n'								
								// select the next command line in the textedit								
								pcount--;
								if ((pcount>0) && (pcount<=HistLen))
								{ 
									TESetSelect((*SIOUXTextWindow->edit)->lineStarts[GetHist(pcount,histTE)-1]+3,
											(*SIOUXTextWindow->edit)->lineStarts[GetHist(pcount,histTE)]-1,
											SIOUXTextWindow->edit);
									TECopy(SIOUXTextWindow->edit);
									// move the cursor to the beginning of this line
									TESetSelect(SIOUXselstart,
											(*SIOUXTextWindow->edit)->teLength,
											SIOUXTextWindow->edit);
									TEPaste(SIOUXTextWindow->edit);
								}
								// if there is no next line, give the blank line
								else {
									pcount=0;
									TESetSelect(SIOUXselstart,
											(*SIOUXTextWindow->edit)->teLength,
											SIOUXTextWindow->edit);
									TEDelete(SIOUXTextWindow->edit);
								}
								aChar=0;
								break;
							default:
								break;
						}
#endif
						
						/*	if the cursor is currently outside the typeable region then move it ...*/
						if ((aChar >= ' ') && !SIOUXisinrange(SIOUXselstart, SIOUXTextWindow->edit))
#if SIOUX_USE_WASTE
							WESetSelection(WEGetTextLength( SIOUXTextWindow->edit ),
										WEGetTextLength( SIOUXTextWindow->edit ),
										SIOUXTextWindow->edit);
#else
							TESetSelect((*SIOUXTextWindow->edit)->teLength,
										(*SIOUXTextWindow->edit)->teLength,
										SIOUXTextWindow->edit);
#endif /* SIOUX_USE_WASTE */
					}
					
					directionMultiplier = 1;
					switch ( aChar )
					{
#if 0 //conflicts with our emacs editing					
						case 0x0C:	/* pageDown */
							directionMultiplier = -1;
						case 0x0B:	/* pageUp */
#if SIOUX_USE_WASTE
							WEGetViewRect( &weViewRect, SIOUXTextWindow->edit );
							scrollAdjust = (weViewRect.bottom - weViewRect.top) /
							   		 	   WEGetHeight( 0, 1, SIOUXTextWindow->edit );
#else
							scrollAdjust = ( (*SIOUXTextWindow->edit)->viewRect.bottom -
													(*SIOUXTextWindow->edit)->viewRect.top ) /
													(*SIOUXTextWindow->edit)->lineHeight;
#endif /* SIOUX_USE_WASTE */
							scrollAdjust *= directionMultiplier;
							MoveScrollBox(SIOUXTextWindow->vscroll, scrollAdjust);
							AdjustText();
							break;
							
						case 0x01:	/* home */
#if SIOUX_USE_WASTE
							LCSetValue( SIOUXTextWindow->vscroll,	/* ¥¥¥LC */
													LCGetMin( SIOUXTextWindow->vscroll ) );
#else
							SetControlValue( SIOUXTextWindow->vscroll,
													GetControlMinimum( SIOUXTextWindow->vscroll ) );
#endif /* SIOUX_USE_WASTE */
							AdjustText();
							break;
							
						case 0x04:	/* end */
#if SIOUX_USE_WASTE
							LCSetValue( SIOUXTextWindow->vscroll,	/* ¥¥¥LC */
													LCGetMax( SIOUXTextWindow->vscroll ) );
#else
							SetControlValue( SIOUXTextWindow->vscroll,
													GetControlMaximum( SIOUXTextWindow->vscroll ) );
#endif /* SIOUX_USE_WASTE */
							AdjustText();
							break;
#endif // above conflicts with our emacs editing
						case 0x0: //must have been handled earlier
							break;							
						default:
#if SIOUX_USE_WASTE
						WEKey(aChar, theEvent.modifiers, SIOUXTextWindow->edit);
#else
						TEKey(aChar, SIOUXTextWindow->edit);
#endif /* SIOUX_USE_WASTE */
						SIOUXUpdateScrollbar();
						if (aChar < 0x1c || aChar > 0x1f)
							SIOUXTextWindow->dirty = true;
					}
					return true;
				}
			}
			break;

		case activateEvt:
		case updateEvt:
			if (HandleUpdateActivateEvent(&theEvent))
				return true;
			break;

		case osEvt:
			if (HandleOSEvents(&theEvent))
				/*  					mm 980605c
				**	SIOUX should return false so that an app embedding
				**	SIOUX that looks at the result of SIOUXHandleOneEvent()
				**	will be able to process the OS event as well.
				*/
				return false;
			break;

		case kHighLevelEvent:
#if ! __MSL__
			if (__system7present == -1)
				__CheckForSystem7();
			
			if (__system7present)
#else
			if (__system7present())
#endif /* ! __MSL__ */
			{
				AEProcessAppleEvent(&theEvent);
			}
			break;

		#if TARGET_API_MAC_OS8     /*instead of !TARGET_API_MAC_CARBON*/ /*- cc 991111 -*/
		case diskEvt:
			if (HiWord(theEvent.message) != noErr) 
			{
				Point pt = {100, 100};
				DIBadMount(pt, theEvent.message);
			}
			break;
		#endif /*- cc 991111 -*/

		case mouseUp:
		case keyUp:
		default:
			break;
	}
	return false;
}

/************************************************************************/
/* Purpose..: Cleans up the data for a quit ...							*/
/* Input....: ---                       								*/
/* Returns..: true killed everything/false user cancelled ...			*/
/************************************************************************/
static Boolean SIOUXCleanUp(void)
{
	short item;
	Str255 aString;

	if (SIOUXTextWindow) 
	{
		if (SIOUXTextWindow->dirty && SIOUXSettings.asktosaveonclose) 
		{
			GetWTitle(SIOUXTextWindow->window, aString); /*- ra 990612 -*/
		
		#if TARGET_API_MAC_CARBON
			{
				Cursor arrowCursor;
				SetCursor(GetQDGlobalsArrow(&arrowCursor));
			}
		#else
			SetCursor(&qd.arrow);
		#endif /* TARGET_API_MAC_CARBON */
		
			item = SIOUXYesNoCancelAlert(aString);
	
			switch (item) 
			{
				case 1:		/*	Yes*/
					if (SIOUXDoSaveText() != noErr && SIOUXSettings.standalone) 
					{	/*	Save the textWindow ...*/
						SIOUXQuitting = false;
						return (false);
					}
					break;
					
				case 3:		/*	Cancel*/
					SIOUXQuitting = false;
					return (false);
					
				case 2:		/*	No*/
				default:	/*	error*/
					break;
			}
		}

#if SIOUX_USE_WASTE		/* ¥¥¥LC */
		/*	before killing the scrollbar, be sure to dispose of the LongControls record */
		LCDetach(SIOUXTextWindow->vscroll);
#endif /* SIOUX_USE_WASTE */


		/*	Kill the textWindow ...*/
	    KillControls(SIOUXTextWindow->window);
#if SIOUX_USE_WASTE
	    WEDispose(SIOUXTextWindow->edit);
#else
	    TEDispose(SIOUXTextWindow->edit);
#endif /* SIOUX_USE_WASTE */

		DisposeWindow(SIOUXTextWindow->window);	
		
		DisposePtr((Ptr)SIOUXTextWindow);
		SIOUXTextWindow = 0L;
		DisposePtr(SIOUXBuffer.startpos);
		
		SIOUXBuffer.startpos = NULL;
		ZEROSIOUXBUFFER();
	}

	return (true);
}

/************************************************************************/
/*	Purpose..: 	Make sure all required Apple Event parameters are used	*/
/*	Input....:	---														*/
/*	Return...:	---														*/
/************************************************************************/
static OSErr GotRequiredParams(const AppleEvent *theEvent)
{
	OSErr theErr;
	DescType theType;
	Size theSize;
	
	theErr = AEGetAttributePtr(theEvent, keyMissedKeywordAttr, typeWildCard, &theType,
		NULL, 0, &theSize);
	
	if (theErr == errAEDescNotFound)
		theErr = noErr;
	else
		theErr = errAEParamMissed;
	
	return theErr;
}

/************************************************************************/
/*	Purpose..: 	Handle Apple Event for when application is launched		*/
/*	Input....:	---														*/
/*	Return...:	---														*/
/************************************************************************/
static pascal OSErr DoHandleOpenApplication(const AppleEvent *theEvent, AppleEvent *theReply,
	long refCon)
{
#pragma unused(theReply, refCon)
	OSErr theErr;
	
	theErr = GotRequiredParams(theEvent);
	
	return theErr;
}

/************************************************************************/
/*	Purpose..: 	Process open and print Apple Event						*/
/*	Input....:	---														*/
/*	Return...:	---														*/
/************************************************************************/
static OSErr DoHandleOpenPrint(const AppleEvent *theEvent, Boolean isOpen)
{
#pragma unused(isOpen)
	OSErr theErr;
	AEDescList theDocuments;
	
	theErr = AEGetParamDesc(theEvent, keyDirectObject, typeAEList, &theDocuments);
	
	if (theErr == noErr)
	{
		theErr = GotRequiredParams(theEvent);
		
		AEDisposeDesc(&theDocuments);
	}
	
	return theErr;
}

/************************************************************************/
/*	Purpose..: 	Handler for Open Document Apple Event					*/
/*	Input....:	---														*/
/*	Return...:	---														*/
/************************************************************************/
static pascal OSErr DoHandleOpenDocuments(const AppleEvent *theEvent, AppleEvent *theReply,
	long refCon)
{
#pragma unused(theReply, refCon)
	OSErr theErr;
	
	theErr = DoHandleOpenPrint(theEvent, true);
	
	return theErr;
}

/************************************************************************/
/*	Purpose..: 	Handler for Print Document Apple Event					*/
/*	Input....:	---														*/
/*	Return...:	---														*/
/************************************************************************/
static pascal OSErr DoHandlePrintDocuments(const AppleEvent *theEvent, AppleEvent *theReply,
	long refCon)
{
#pragma unused(theReply, refCon)
	OSErr theErr;
	
	theErr = DoHandleOpenPrint(theEvent, false);
	
	return theErr;
}

/************************************************************************/
/*	Purpose..: 	Handle Apple Event for quit application requests		*/
/*	Input....:	---														*/
/*	Return...:	---														*/
/************************************************************************/
static pascal OSErr DoHandleQuit(const AppleEvent *theEvent, AppleEvent *theReply,
	long refCon)
{
#pragma unused(theReply, refCon)
	OSErr theErr;
	
	theErr = GotRequiredParams(theEvent);
	
	if (theErr == noErr)
	{
		SIOUXQuitting = true;
		
		if (SIOUXCleanUp() == false)
			theErr = userCanceledErr;
	}	
	return theErr;
}

/************************************************************************/
/* Purpose..: Install the console package								*/
/* Input....: The stream to install (ignored)							*/
/* Returns..: 0 no error / -1 error occurred							*/
/************************************************************************/
short InstallConsole(short fd)
{
#pragma unused (fd)
	
	if (SIOUXQuitting || SIOUXState != OFF || SIOUXSettings.stubmode) return 0;
	//this allows hoc_quit() to work. Ie call to RemoveConsole succeeds withou
	SIOUXSettings.autocloseonquit = true;
	//SIOUXSettings.standalone = false;
	if (SIOUXSettings.initializeTB && !toolBoxDone)
		DoSetupToolbox();

	/*	Initialize Space for the SIOUX buffer ...*/
	if ((SIOUXBuffer.startpos = (char *)NewPtr(SIOUX_BUFSIZ)) == NULL)
		return -1;
	ZEROSIOUXBUFFER();
	SIOUXBuffer.tepos = -1;    /*- mm 980108 -*/
	/*	Setup the menus ...*/
	if (SIOUXSettings.setupmenus)
		SIOUXSetupMenus();

	/* JWW - Install Apple Event handlers for standalone SIOUX appliation */
	if ((SIOUXSettings.standalone == true) && (!appleEventsDone))
	{
#if ! __MSL__
		if (__system7present == -1)
			__CheckForSystem7();
	
		if (__system7present)
#else
		if (__system7present())
#endif /* ! __MSL__ */
		{
// done in session.cpp
//			AEInstallEventHandler(kCoreEventClass, kAEOpenApplication,
//				NewAEEventHandlerUPP(&DoHandleOpenApplication), 0, false);
			
//			AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments,
//				NewAEEventHandlerUPP(&DoHandleOpenDocuments), 0, false);
			
			AEInstallEventHandler(kCoreEventClass, kAEPrintDocuments,
				NewAEEventHandlerUPP(&DoHandlePrintDocuments), 0, false);
			
			AEInstallEventHandler(kCoreEventClass, kAEQuitApplication,
				NewAEEventHandlerUPP(&DoHandleQuit), 0, false);
			
			appleEventsDone = true;
		}
	}
	
	/*	Setup the textWindow ...*/
	if (SIOUXSetupTextWindow()) 
	{
		if (SIOUXSettings.standalone == false) 
		{
			SIOUXSettings.autocloseonquit = true;
		}

		SIOUXState = IDLE;

		/*	Test for WaitNextEvent ...*/
	
	#if !TARGET_API_MAC_OS8
		SIOUXUseWaitNextEvent = true;
	#else
		if (GetToolTrapAddress(_WaitNextEvent) != GetToolTrapAddress(_Unimplemented))
			SIOUXUseWaitNextEvent = true;
	#endif /* !TARGET_API_MAC_OS8 */

		iBeamCursorH = GetCursor(iBeamCursor);

		return 0;
	}

#if ! __MSL__
		__RemoveConsoleHandler__ = RemoveConsole;
#endif /* ! __MSL__ */
			
	return(-1);
}

/************************************************************************/
/* Purpose..: Remove the console package								*/
/* Input....: ---														*/
/* Returns..: ---														*/
/************************************************************************/
void RemoveConsole(void)
{
	extern int __aborting;
	
	if (SIOUXState == OFF || !SIOUXTextWindow || SIOUXSettings.stubmode)
		return;

	if (__aborting)
		SIOUXState = ABORTED;
	else
		SIOUXState = TERMINATED;
	SIOUXUpdateStatusLine(SIOUXTextWindow->window);	/*- ra 990612 -*/

	SIOUXselstart = 0;
#if SIOUX_USE_WASTE
	WEActivate(SIOUXTextWindow->edit);
#else
	TEActivate(SIOUXTextWindow->edit);
#endif /* SIOUX_USE_WASTE */
	SIOUXUpdateScrollbar();

	if (SIOUXSettings.autocloseonquit)
		SIOUXQuitting = true;

	while (!SIOUXQuitting) 
	{
BackInTheLoop:
		SIOUXHandleOneEvent(NULL);
	}

	if (!SIOUXCleanUp())
		goto BackInTheLoop;

#if ! __MSL__
	__RemoveConsoleHandler__ = NULL;
#endif /* ! __MSL__ */

	SIOUXState = OFF;
}

/************************************************************************/
/*	Purpose..: 	Write a string to the console							*/
/*	Input....:	pointer to buffer										*/
/*	Input....:	number of chars in buffer								*/
/*	Return...:	0 no error / -1 error occurred							*/
/************************************************************************/
long WriteCharsToConsole(char *buffer, long n)
{
	long counter, i, spacestoinsert;
	char aChar;
	GrafPtr saveport;

	if (SIOUXQuitting || SIOUXSettings.stubmode)
		return 0;

	GetPort(&saveport);
	SetPortWindowPort(SIOUXTextWindow->window);		/*- ra 990612 -*/

	SIOUXState = PRINTFING;
	SIOUXUpdateStatusLine(SIOUXTextWindow->window); /*- ra 990612 -*/

	for(counter = n; counter > 0; counter--)
	{
		aChar = *buffer++;
		switch(aChar) 
		{
			case 0x0d:	
				#if !__option(mpwc_newline)		/*	Line Feed (Mac newline)*/
					INSERTLINEFEED();
					break;
					
				#else	/*	Carriage Return (move to start of line)*/
					i = OffsetOnCurrentLine(SIOUXTextWindow->edit);	
					if (i <= CURRENTBUFSIZE()) 
					{
						ROLLBACKBUFFER(i);
					} 
					else 
					{
						SIOUXBuffer.tepos = (*SIOUXTextWindow->edit)->teLength - (i - CURRENTBUFSIZE());
					}
					break;
				#endif
				
			case 0x0a:	
				#if !__option(mpwc_newline)	/*	Carriage Return (move to start of line)*/
					i = OffsetOnCurrentLine(SIOUXTextWindow->edit);
					if (i <= CURRENTBUFSIZE()) 
					{
						ROLLBACKBUFFER(i);
					} 
					else 
					{
#if SIOUX_USE_WASTE
						SIOUXBuffer.tepos = WEGetTextLength( SIOUXTextWindow->edit ) - (i - CURRENTBUFSIZE());
#else
						SIOUXBuffer.tepos = (*SIOUXTextWindow->edit)->teLength - (i - CURRENTBUFSIZE());
#endif /* SIOUX_USE_WASTE */
                    ROLLBACKBUFFER(CURRENTBUFSIZE());  /*- mm 980109 -*/					
                    }
					break;
				#else		/*	Line Feed (Mac newline)*/
					INSERTLINEFEED();
					break;			
				#endif
				
			case '\t':	/*	Tab character*/
				if (SIOUXSettings.tabspaces) 
				{
					/*	insert spaces for tabs*/
					CHECKFOROVERFLOW(SIOUXSettings.tabspaces);

					i = OffsetOnCurrentLine(SIOUXTextWindow->edit);

					spacestoinsert = SIOUXSettings.tabspaces -
									 (i % SIOUXSettings.tabspaces);
					for (i = 0; i < spacestoinsert; i++) INSERTCHAR(' ');
				} 
				else
					INSERTCHAR('\t');
				break;
				
			case '\f':	/*	Form Feed*/
				CHECKFOROVERFLOW(SIOUXTextWindow->linesInFolder);
				for (i = SIOUXTextWindow->linesInFolder; i > 0; i--) INSERTLINEFEED();
				break;
				
			case '\a':	/*	Audible Alert*/
				SysBeep(1);
				break;
				
			case '\b':	/*	Backspace*/
				if (CURRENTBUFSIZE() != 0) 
				{
					/*DELETEFROMBUFFER(1);   */   /*- mm 970212 -*/
				    SIOUXBuffer.curpos -= 1;      /*- mm 970212 -*/
				    SIOUXBuffer.endpos -= 1;      /*- mm 981210 -*/
				} 
				else 
				{				/*	Need to delete the last character from the TextEdit Handle*/
#if SIOUX_USE_WASTE
					long teLength = WEGetTextLength( SIOUXTextWindow->edit );
#else
					short teLength = (*SIOUXTextWindow->edit)->teLength;
#endif /* SIOUX_USE_WASTE */
					if (teLength > 0) 
					{
#if SIOUX_USE_WASTE
						OSErr err;
						WESetSelection(teLength-1, teLength, SIOUXTextWindow->edit);
						err = WEDelete(SIOUXTextWindow->edit);
#else
						TESetSelect(teLength-1, teLength, SIOUXTextWindow->edit);
						TEDelete(SIOUXTextWindow->edit);
#endif /* SIOUX_USE_WASTE */
					}
				}
				break;
				
			case '\v':	/*	Vertical Tab*/
				break;
				
			default:	/*	just add it to SIOUX ...*/
				INSERTCHAR(aChar);
				break;
		}
		CHECKFOROVERFLOW(0);
	}
	InsertSIOUXBuffer();

	if (!SIOUXQuitting)
	{
		SIOUXState = IDLE;
		SIOUXUpdateStatusLine(SIOUXTextWindow->window);	 /*- ra 990612 -*/
	}

	SetPort(saveport);

	return n;
}

/************************************************************************/
/*	Purpose..: 	Read characters into the buffer							*/
/*	Input....:	pointer to buffer										*/
/*	Input....:	max length of buffer									*/
/*	Return...:	number of characters read / -1 error occurred			*/
/************************************************************************/
long ReadCharsFromConsole(char *buffer, long n)
{
	long charsread;
	GrafPtr saveport;
#if SIOUX_USE_WASTE
	Handle textHandle;
#endif /* SIOUX_USE_WASTE */

	IVOCGoodLine = false;

	if (SIOUXQuitting || SIOUXSettings.stubmode)
		return 0;

	if (atEOF) 
	{
		buffer[0] = EOF;
		return -1;								/*- mm 010314 -*/
	}
	//jijun 4/16/97 for "OC>" prompt
	if (saw_line) {
			saw_line = false;
			WriteCharsToConsole("oc>",3);		
	}
	
	GetPort(&saveport);
	SetPortWindowPort(SIOUXTextWindow->window);	/*- ra 990612 -*/

	SIOUXState = SCANFING;
	inputBuffersize = n;
#if SIOUX_USE_WASTE
	SIOUXselstart = WEGetTextLength( SIOUXTextWindow->edit );
#else
	SIOUXselstart = (*SIOUXTextWindow->edit)->teLength;
#endif /* SIOUX_USE_WASTE */

	SelectWindow(SIOUXTextWindow->window);		/*- ra 990612 -*/
	SIOUXUpdateStatusLine(SIOUXTextWindow->window);
	
#if SIOUX_USE_WASTE
	WESetSelection(SIOUXselstart, SIOUXselstart, SIOUXTextWindow->edit);
	WEActivate(SIOUXTextWindow->edit);
#else
	TESetSelect(SIOUXselstart, SIOUXselstart, SIOUXTextWindow->edit);
	TEActivate(SIOUXTextWindow->edit);
#endif /* SIOUX_USE_WASTE */
	
	SIOUXUpdateScrollbar();

	while (SIOUXState == SCANFING && !SIOUXQuitting) 
	{
BackInTheLoop:
		SIOUXHandleOneEvent(NULL);
	}
		
	if (SIOUXQuitting) 
	{
		if (!SIOUXCleanUp()) 
		{
			SIOUXQuitting = false;
			goto BackInTheLoop;
		}
		SetPort(saveport);
		exit(0);
	}

	/*	put the string into the buffer ...*/
#if SIOUX_USE_WASTE
	charsread = WEGetTextLength(SIOUXTextWindow->edit ) - SIOUXselstart;
	textHandle = WEGetText(SIOUXTextWindow->edit);
	HLock( textHandle );
	BlockMoveData( *textHandle + SIOUXselstart, buffer, charsread);
	HUnlock( textHandle );

#else
	if ((*SIOUXTextWindow->edit)->teLength > 0)
	charsread = (*SIOUXTextWindow->edit)->teLength - SIOUXselstart;
	else
		charsread = ((unsigned short) (*SIOUXTextWindow->edit)->teLength) - SIOUXselstart;
#ifndef __NO_WIDE_CHAR														/*- mm 981020 -*/
	if (fwide(stdin, 0) <= 0)                                        		/*- mm 980208 -*/
#endif /* #ifndef __NO_WIDE_CHAR */											/*- mm 981020 -*/
	BlockMoveData((*(*SIOUXTextWindow->edit)->hText) + SIOUXselstart,
			  buffer,
			  charsread);
#ifndef __NO_WIDE_CHAR														/*- mm 981020 -*/
	else																	/*- mm 980208 -*/
	{																		/*- mm 980208 -*/
		char tempbuf[256];
		long count;													        /*- mm 980208 -*/
		BlockMoveData((*(*SIOUXTextWindow->edit)->hText) + SIOUXselstart,	/*- mm 980208 -*/
			  tempbuf,														/*- mm 980208 -*/
			  charsread);													/*- mm 980208 -*/
		for (count = 0; count <= charsread; count++)						/*- mm 980208 -*/
		{																	/*- mm 980208 -*/
			buffer[2*count] = '\0';											/*- mm 980208 -*/
			buffer[2*count+1] = tempbuf[count];								/*- mm 980208 -*/
		}																	/*- mm 980208 -*/
		
		charsread *= 2;														/*- mm 980208 -*/
	}																		/*- mm 980208 -*/
#endif /* #ifndef __NO_WIDE_CHAR */											/*- mm 981020 -*/
		
#endif /* SIOUX_USE_WASTE */
	/*	if no error occurred continue else return 0 characters read ...*/
	if (MemError() == noErr) 
	{
		if (buffer[charsread - 1L] == 0x03)	/* The user did a Control - Z or control - D (ie an EOF) */
			charsread--, atEOF = 1;
		else
			buffer[charsread - 1L] = 0x0d;
	} 
	else 
	{
		charsread = 0;
	}

	SIOUXUpdateStatusLine(SIOUXTextWindow->window);	/*- ra 990612 -*/

	SetPort(saveport);

	return charsread;
}

/*
 *	return the name of the current terminal ...
 */
char *__ttyname(long fildes)
{
	/*	all streams have the same name ...*/
	static char *__SIOUXDeviceName = "SIOUX";
	
	if (fildes >= 0 && fildes <= 2)
		return (__SIOUXDeviceName);

	return (NULL);
}

void hoc_sioux_show(int b) {
    if (SIOUXTextWindow != NULL) {
        if (b) {
            ShowWindow(SIOUXTextWindow->window);
        }else{
            HideWindow(SIOUXTextWindow->window);
        }
    }
}

/*
 *	Set SIOUX's window title ...
 */
void SIOUXSetTitle(unsigned char title[256])
{

	if (SIOUXTextWindow != NULL)
		SetWTitle(SIOUXTextWindow->window, title);	/*- ra 990612 -*/
	else   /*- mm 980609 -*/
	{
		SIOUXSettings.userwindowtitle = (unsigned char*)malloc(title[0]+1);
		if (SIOUXSettings.userwindowtitle != NULL)
			BlockMoveData(title, SIOUXSettings.userwindowtitle, (long)title[0] + 1);
	}
}

/*
*
*    int kbhit()
*
*    returns true if any keyboard key is pressed without retrieving the key
*    used for stopping a loop by pressing any key
*/
int kbhit(void)
{
  EventRecord event;
  
  if (SIOUXSettings.stubmode) return 0;
  
  return EventAvail(keyDownMask,&event); 
}

/*
*
*    int getch()
*
*    returns the keyboard character pressed when an ascii key is pressed  
*    used for console style menu selections for immediate actions.
*/
int getch(void)
{
   int c;
   EventRecord event;
   
   if (SIOUXSettings.stubmode) return 0;
   
   fflush(stdout);
         /* Allow SIOUX response for the mouse, drag, zoom, or resize. */
	while(!GetNextEvent(keyDownMask,&event)) 
	{
		if (GetNextEvent(updateMask | osMask | mDownMask | mUpMask, &event)) /*- mm 980506 -*/
			SIOUXHandleOneEvent(&event);
	}
	c = event.message&charCodeMask;
	if (c == '.' && (event.modifiers&cmdKey))
		exit(1);
  
   return c;
}

/*
*     void clrscr()
*
*     clears screen  and empties buffer.
*/
void clrscr(void)                   /*- mm 980427 -*/  /*- mm 981218 -*/
{
	if (SIOUXSettings.stubmode) return;
	
#if SIOUX_USE_WASTE
	WESetSelection(0, WEGetTextLength(SIOUXTextWindow->edit), SIOUXTextWindow->edit);
	WEDelete(SIOUXTextWindow->edit);
#else
	TESetText(NULL, 0, SIOUXTextWindow->edit);
#endif
/*	EventRecord rEvent;
 
	fflush(stdout);
	rEvent.what 	 = keyDown;
	rEvent.when 	 = TickCount( );
	rEvent.message 	 = 'a';
	rEvent.modifiers = cmdKey;	
	SIOUXHandleOneEvent(&rEvent);

	rEvent.what 	 = keyDown;
	rEvent.when 	 = TickCount( );
	rEvent.message 	 = 0x7F;	
	rEvent.modifiers = 0;
	SIOUXHandleOneEvent(&rEvent);*/
}



/* Change record:
 * BB  930110 removed diskEvt from switch statement since this called
 *			  DIBadMount which is not glue code and hence required importing
 *			  MacOS.lib ...
 * BB  940121 removed the direct call to GrowDrawIcon and replaced it
 *			  with a call to a clipping function which doesn't draw the
 *			  lines.
 * BB  940125 Added support for command - '.', also changed calls to
 *			  ExitToShell() to exit() which allows the ANSI libs to close
 *			  any open file streams.
 * BB  940125 Added support for tab characters.
 * BB  940722 Added support for EOF, through control - z
 * BB  940911 Added support for control - c, also both control-c and command-.
 *			  call raise SIGABRT rather than calling exit.
 *			  Added support for mouse down to pause output.
 *			  Added support for setting tab behaviour in SIOUX.
 *			  Fixed support for characters >128
 * BB  941020 Fixed behaviour of replacing tabs for spaces ...
 * BB  941025 Added support for '\r' to move cursor to beginning of line
 * BB  941026 Changed command-'.' and control - c to raise SIGINT instead of
 *			  SIGABRT.
 * BB  940112 Extended EOF support to include control - d
 * JH  950901 Modified to run with new ANSI C library
 * JH  951210 Moved __system7present() back to unix.c
 * JH  960129 Added missing return in SIOUXHandleOneEvent's keyDown/autoKey
 *			  Cmd-key handling
 * JH  960219 Eliminated local definition of EOF.
 * bk  960219 added Universal Headers incase macheaders not the prefix
 * bk  961228 line 752-772 switched LF/CR if __option(mpwc_newline)
 * mm  970212 Changed backspace to just move cursor when text still in buffer.
 * mm  970626 Modification to allow correct function of '\r'
 * mm  970903 Correction to calculation of OffsetOnCurrentLine  MW00396 
 * mm  971006 Added #include of Sound.h because of change in universal headers
 * mm  971229 Corrected semantics of \r in errors revealed by MW03003
 * mm  971230 Changes to OffsetOnCurrentLine for WASTE to correct semantics of \r in errors revealed by MW03003
 * mm  980108 Change to InstallConsole to correct behaviour of \r on first line of output
 * mm  980109 A further fix to \r
 * mm  980112 Yet another fix to \r
 * mm  980208 Support for reading wide characters from console for wscanf.  Not yet done for WASTE
 * mm  980331 Fix to insert linefeed to make the sequence \b\n work correctly.
 * mm  980413 Fix to make the enter key work as return on input on a PowerBook where its virtual key code is 0x34.
 * mm  980427 Added SIOUXclrscr   MW06847
 * mm  980506 Modified getch() so that the window is repainted if necessary. MW03278
 * mm  980605 Change from Michael Bishop to make SIOUX only select windows it knows about MW06855
 * mm  980605 Change from Michael Bishop to make SIOUX select windows when user clicks in drag or grow region 
 *            if it's not the FrontWindow.   MW06856
 * mm  980605 Change from Michael Bishop to return false from SIOUXHandleOneEvent() so that an app that embeds
 *            SIOUX will be able to process a NULL event as well.  MM06857
 * mm  980609 Changes that allow user to specify window title before the SIOUX window is created.
 * vss 980629 moved variable into code block that uses it to remove warning
 * mm  981020 Added  #ifndef __NO_WIDE_CHAR wrappers
 * mm  981210 Corrected backspace behaviour for stderr (non-buffered) MW08661
 * mm  981218 Changed name SIOUXclrscr to clrscr MW08237
 * cc  991108 added ra Carbon Changes done 990611
 * cc  991109 changed TARGET_CARBON to TARGET_API_MAC_CARBON
 * cc  991111 added ra & JWW suggestions for functions no longer in carbon
 * cc  991115 updated and deleted carbon outdated comments
 * JWW 000413 Call DisposeWindow or CloseWindow -- never both
 * cc  000516 __myraise is now an inline in unix.h
 * JWW  Added Apple Event handlers for standalone application (necessary to catch Quit on OS X, but also nice to have overall)
 * JWW 000922 Added new user changable sleep value setting for WaitNextEvent
 * JWW 000531 Changed clrscr to directly clear the output window contents instead of sending events
 * JWW 010124 Fixed case where when 32767 items in the TextEdit window could cause a crash during input operations
 * mm  010314 Pass the detection of EOF (ctrl-z or ctrl-d) back to caller as an error detected
 * JWW 010321 Set a flag so that the Apple Event handlers are installed only once no matter what
 * JWW 010321 RemoveConsole now does DisposeWindow() in all cases and disposes of the buffer memory too
 * JWW 010726 Check for SIOUX quitting inside WriteCharsToConsole before updating the status line of a possibly nonexistant window
 * JWW 010807 Added stub mode setting
 */
 
