/* /local/src/master/nrn/src/memacs/edef.h,v 1.2 1997/08/27 20:33:06 hines Exp */
/*
edef.h,v
 * Revision 1.2  1997/08/27  20:33:06  hines
 * memacs code now compiled into memacs.dll for mswindows in order to
 * allow nrndisk2.zip to fit on one diskette.
 *
 * Revision 1.1.1.1  1994/10/12  17:21:23  hines
 * NEURON 3.0 distribution
 *
 * Revision 1.7  93/02/11  17:07:18  hines
 * to work on NeXT
 * 
 * Revision 1.6  93/02/03  09:18:26  hines
 * make slightly more generic
 * 
 * Revision 1.5  91/08/22  16:58:57  hines
 * for distribution to work on RS6000
 * 
 * Revision 1.4  91/08/22  12:33:43  hines
 * works with turboc
 * 
 * Revision 1.3  89/07/10  10:25:29  mlh
 * LINT free
 * 
 * Revision 1.2  89/07/09  12:18:42  mlh
 * lint pass1 now ok
 * 
 * Revision 1.1  89/07/08  15:37:41  mlh
 * Initial revision
 * 
*/

/*	EDEF:		Global variable definitions for
			MicroEMACS 3.2

			written by Dave G. Conroy
			modified by Steve Wilhite, George Jones
			greatly modified by Daniel Lawrence
*/

#include	"redef.h"	/*mlh 2/29/87 add emacs_ prefix to all public variables*/
#include	<string.h>

#include	<stdlib.h>
#if 0
#ifndef NeXT
#if __TURBOC__
#include <alloc.h>
#else
#include <malloc.h>
#endif
#endif
#endif

#ifdef	maindef

/* for MAIN.C */

/* initialized global definitions */

int     fillcol = 72;                   /* Current fill column          */
int     tabsize = 8;                    /* Tab size */
int   kbdm[NKBDM] = {CTLX|')'};       /* Macro                        */
char    pat[NPAT];                      /* Search pattern		*/
char	rpat[NPAT];			/* replacement pattern		*/
char	sarg[NSTRING] = "";		/* string argument for line exec*/
int	eolexist = TRUE;		/* does clear to EOL exist	*/
int	revexist = FALSE;		/* does reverse video exist?	*/
char	*modename[] = {			/* name of modes		*/
	"WRAP", "CMODE", "SPELL", "EXACT", "VIEW", "OVER"};
char	modecode[] = "WCSEVO";		/* letters to represent modes	*/
int	gmode = 0;			/* global editor mode		*/
int     sgarbf  = TRUE;                 /* TRUE if screen is garbage	*/
int     mpresf  = FALSE;                /* TRUE if message in last line */
int	clexec	= FALSE;		/* command line execution flag	*/

/* uninitialized global definitions */

int     currow;                 /* Cursor row                   */
int     curcol;                 /* Cursor column                */
int     thisflag;               /* Flags, this command          */
int     lastflag;               /* Flags, last command          */
int     curgoal;                /* Goal for C-P, C-N            */
#ifdef MEMACS_DLL
extern WINDOW  *curwp;                 /* Current window               */
extern BUFFER  *curbp;                 /* Current buffer               */
#else
WINDOW  *curwp;                 /* Current window               */
BUFFER  *curbp;                 /* Current buffer               */
#endif
WINDOW  *wheadp;                /* Head of list of windows      */
BUFFER  *bheadp;                /* Head of list of buffers      */
BUFFER  *blistp;                /* Buffer for C-X C-B           */
int   *kbdmip;                /* Input pointer for above      */
int   *kbdmop;                /* Output pointer for above     */

BUFFER  *bfind();               /* Lookup a buffer by name      */
WINDOW  *wpopup();              /* Pop up window creation       */
LINE    *lalloc();              /* Allocate a line              */

#else

/* for all the other .C files */

/* initialized global external declarations */

extern  int     fillcol;                /* Fill column                  */
extern	int     tabsize;                /* Tab size                     */
extern  int   kbdm[];                 /* Holds kayboard macro data    */
extern  char    pat[];                  /* Search pattern               */
extern	char	rpat[];			/* Replacement pattern		*/
extern	char	sarg[];			/* string argument for line exec*/
extern	int	eolexist;		/* does clear to EOL exist?	*/
extern	int	revexist;		/* does reverse video exist?	*/
extern	char *modename[];		/* text names of modes		*/
extern	char	modecode[];		/* letters to represent modes	*/
extern	KEYTAB keytab[];		/* key bind to functions table	*/
extern	NBIND names[];			/* name to function table	*/
extern	int	gmode;			/* global editor mode		*/
extern  int     sgarbf;                 /* State of screen unknown      */
extern  int     mpresf;                 /* Stuff in message line        */
extern	int	clexec;			/* command line execution flag	*/

/* initialized global external declarations */

extern  int     currow;                 /* Cursor row                   */
extern  int     curcol;                 /* Cursor column                */
extern  int     thisflag;               /* Flags, this command          */
extern  int     lastflag;               /* Flags, last command          */
extern  int     curgoal;                /* Goal for C-P, C-N            */
extern  WINDOW  *curwp;                 /* Current window               */
extern  BUFFER  *curbp;                 /* Current buffer               */
extern  WINDOW  *wheadp;                /* Head of list of windows      */
extern  BUFFER  *bheadp;                /* Head of list of buffers      */
extern  BUFFER  *blistp;                /* Buffer for C-X C-B           */
extern  int   *kbdmip;                /* Input pointer for above      */
extern  int   *kbdmop;                /* Output pointer for above     */

extern  BUFFER  *bfind();               /* Lookup a buffer by name      */
extern  WINDOW  *wpopup();              /* Pop up window creation       */
extern  LINE    *lalloc();              /* Allocate a line              */

#endif

/* terminal table defined only in TERM.C */

#ifndef	termdef
extern  TERM    term;                   /* Terminal information.        */
#endif

#include "intfunc.h"

#if LINT
#define IGNORE(arg)	{if (arg);}
#define LINTUSE(arg)	{if (arg);}
char *cplint;
#define	Strcat		cplint = strcat
#define Strncat		cplint = strncat
#define Strcpy		cplint = strcpy
#define Strncpy		cplint = strncpy
#define Sprintf		cplint = sprintf
#else
#define IGNORE(arg)	arg
#define LINTUSE(arg)
#define Strcat		strcat
#define Strncat		strncat
#define Strcpy		strcpy
#define Strncpy		strncpy
#define Sprintf		sprintf
#endif
