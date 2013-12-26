/* /local/src/master/nrn/src/memacs/estruct.h,v 1.4 1997/08/27 20:33:07 hines Exp */
/*
estruct.h,v
 * Revision 1.4  1997/08/27  20:33:07  hines
 * memacs code now compiled into memacs.dll for mswindows in order to
 * allow nrndisk2.zip to fit on one diskette.
 *
 * Revision 1.3  1997/03/13  14:17:41  hines
 * Merge Macintosh changes. NEURON sources compile with Codewarrior 11.
 * No readline or microemacs for mac.
 *
 * Revision 1.2  1996/10/17  16:31:44  hines
 * trivial #define change for memacs for compiling under solaris2.5 cc
 *
 * Revision 1.1.1.1  1994/10/12  17:21:23  hines
 * NEURON 3.0 distribution
 *
 * Revision 1.7  1993/11/09  13:43:48  hines
 * port to djg dos extender for running under go32
 *
 * Revision 1.6  1993/10/18  13:20:23  hines
 * some machines already define CTRL so we undef it first.
 *
 * Revision 1.5  92/10/24  11:25:08  hines
 * no such thing as #elif for some compilers
 * 
 * Revision 1.4  92/08/12  08:15:09  hines
 * sejnowski changes to allow compiling on ncube using EXPRESS c libraries.
 * it allows compilation of neuron but doesn't actually work.
 * 
 * Revision 1.3  92/01/20  13:49:21  hines
 * allow porting to ibmpc
 * 
 * Revision 1.2  91/08/13  11:19:23  hines
 * BSD and SYSV are defined in the makefile
 * 
 * Revision 1.1  89/07/08  15:37:52  mlh
 * Initial revision
 * 
*/

/*	ESTRUCT:	Structure and preprocesser defined for
			MicroEMACS 3.6

			written by Dave G. Conroy
			modified by Steve Wilhite, George Jones
			greatly modified by Daniel Lawrence
*/

#undef	LATTICE		/* don't use their definitions...use ours	*/
#undef	MSDOS
#undef	CPM

#define	MLH	1	/* personal functionality desired by michael hines */

/*	Machine/OS definitions			*/

#define AMIGA   0                       /* AmigaDOS			*/
#define ST520   0                       /* ST520, TOS                   */
#if defined(__MWERKS__) || defined(__TURBOC__) || __GO32__
#define MSDOS   1                       /* MS-DOS                       */
#else
#define V7      1                       /* V7 UN*X or Coherent or BSD4.2*/
#endif
#if !defined(BSD)
#define	BSD	0			/* also needed for BSD 4.2	*/
#endif
#if !defined(SYSV)
#define SYSV    0                       /* V7 and SYSV for System V     */
#else
#undef SYSV
#undef BSD
#define SYSV	1
#define BSD	0
#endif
#define VMS     0                       /* VAX/VMS                      */
#define CPM     0                       /* CP/M-86                      */

/*	Compiler definitions			*/
#define MWC86   0	/* marc williams compiler */
#define	LATTICE	0	/* either lattice compiler */
#define	LAT2	0	/* Lattice 2.15 */
#define	LAT3	0	/* Lattice 3.0 */
#define	AZTEC	0	/* Aztec C 3.20e */
#define MSC	0	/* Microsoft C V3 */

/*	Terminal Output definitions		*/

#if defined(__TURBOC__) || __GO32__
#define ANSI    0			/* ansi escape sequences	*/
#else
#if defined(EXPRESS)
#define ANSI	1
#define TERMCAP 0
#else
#define TERMCAP 1                       /* Use TERMCAP                  */
#endif
#endif
#define	HP150	0			/* HP150 screen driver		*/
#define	VMSVT	0			/* various VMS terminal entries	*/
#define VT52    0                       /* VT52 terminal (Zenith).      */
#define VT100   0                       /* Handle VT100 style keypad.   */
#define LK201   0                       /* Handle LK201 style keypad.   */
#define RAINBOW 0                       /* Use Rainbow fast video.      */

/*	Configuration options	*/

#define CVMVAS  1	/* arguments to page forward/back in pages	*/
#define	NFWORD	1	/* forward word jumps to begining of word	*/
#define	CLRMSG	0	/* space clears the message line with no insert	*/
#define	TYPEAH	0/*mlh*/	/* type ahead causes update to be skipped	*/
#define	FILOCK	0	/* file locking under unix BSD 4.2		*/
#define	REVSTA	1	/* Status line appears in reverse video		*/
#define BENTHAK	1 	/* Various Bennett preferences                  */
#define TRUETAB 1	/* Tabstops affect display, not insert spaces	*/

/*	System dependant library redefinitions	*/

#if	MSDOS & AZTEC
#undef	fputc
#undef	fgetc
#define	fputc	aputc
#define	fgetc	agetc
#endif

/*	internal constants	*/

#define	NBINDS	200			/* max # of bound keys		*/
#define NFILEN  1024                      /* # of bytes, file name        */
#define NBUFN   16                      /* # of bytes, buffer name      */
#if 0
#define NLINE   256                     /* # of bytes, line             */
#else
#define NLINE   16384                     /* # of bytes, line             */
#endif
#define	NSTRING	256			/* # of bytes, string buffers	*/
#define NKBDM   256                     /* # of strokes, keyboard macro */
#define NPAT    80                      /* # of bytes, pattern          */
#define HUGE    1000                    /* Huge number                  */
#define	NLOCKS	100			/* max # of file locks active	*/

#define AGRAVE  0x60                    /* M- prefix,   Grave (LK201)   */
#define METACH  0x1B                    /* M- prefix,   Control-[, ESC  */
#define CTMECH  0x1C                    /* C-M- prefix, Control-\       */
#define EXITCH  0x1D                    /* Exit level,  Control-]       */
#define CTRLCH  0x1E                    /* C- prefix,   Control-^       */
#define HELPCH  0x1F                    /* Help key,    Control-_       */

#undef CTRL
#define CTRL    0x0100                  /* Control flag, or'ed in       */
#define META    0x0200                  /* Meta flag, or'ed in          */
#define CTLX    0x0400                  /* ^X flag, or'ed in            */
#define	SPEC	0x0800			/* special key (function keys)	*/

#define FALSE   0                       /* False, no, bad, etc.         */
#define TRUE    1                       /* True, yes, good, etc.        */
#define ABORT   2                       /* Death, ^G, abort, etc.       */

#define FIOSUC  0                       /* File I/O, success.           */
#define FIOFNF  1                       /* File I/O, file not found.    */
#define FIOEOF  2                       /* File I/O, end of file.       */
#define FIOERR  3                       /* File I/O, error.             */
#define	FIOLNG	4			/*line longer than allowed len	*/

#define CFCPCN  0x0001                  /* Last command was C-P, C-N    */
#define CFKILL  0x0002                  /* Last command was a kill      */

#define	BELL	0x07			/* a bell character		*/
#define	TAB	0x09			/* a tab character		*/

/*
 * There is a window structure allocated for every active display window. The
 * windows are kept in a big list, in top to bottom screen order, with the
 * listhead at "wheadp". Each window contains its own values of dot and mark.
 * The flag field contains some bits that are set by commands to guide
 * redisplay; although this is a bit of a compromise in terms of decoupling,
 * the full blown redisplay is just too expensive to run for every input
 * character.
 */
typedef struct  WINDOW {
        struct  WINDOW *w_wndp;         /* Next window                  */
        struct  BUFFER *w_bufp;         /* Buffer displayed in window   */
        struct  LINE *w_linep;          /* Top line in the window       */
        struct  LINE *w_dotp;           /* Line containing "."          */
        struct  LINE *w_markp;          /* Line containing "mark"       */
        int   w_doto;                 /* Byte offset for "."          */
        int   w_marko;                /* Byte offset for "mark"       */
        char    w_toprow;               /* Origin 0 top row of window   */
        char    w_ntrows;               /* # of rows of text in window  */
        char    w_force;                /* If NZ, forcing row.          */
        char    w_flag;                 /* Flags.                       */
}       WINDOW;

#define WFFORCE 0x01                    /* Window needs forced reframe  */
#define WFMOVE  0x02                    /* Movement from line to line   */
#define WFEDIT  0x04                    /* Editing within a line        */
#define WFHARD  0x08                    /* Better to a full display     */
#define WFMODE  0x10                    /* Update mode line.            */

/*
 * Text is kept in buffers. A buffer header, described below, exists for every
 * buffer in the system. The buffers are kept in a big list, so that commands
 * that search for a buffer by name can find the buffer header. There is a
 * safe store for the dot and mark in the header, but this is only valid if
 * the buffer is not being displayed (that is, if "b_nwnd" is 0). The text for
 * the buffer is kept in a circularly linked list of lines, with a pointer to
 * the header line in "b_linep".
 * 	Buffers may be "Inactive" which means the files accosiated with them
 * have not been read in yet. These get read in at "use buffer" time.
 */
typedef struct  BUFFER {
        struct  BUFFER *b_bufp;         /* Link to next BUFFER          */
        struct  LINE *b_dotp;           /* Link to "." LINE structure   */
        struct  LINE *b_markp;          /* The same as the above two,   */
        struct  LINE *b_linep;          /* Link to the header LINE      */
        int   b_doto;                 /* Offset of "." in above LINE  */
        int   b_marko;                /* but for the "mark"           */
	char	b_active;		/* window activated flag	*/
        char    b_nwnd;                 /* Count of windows on buffer   */
        char    b_flag;                 /* Flags                        */
	char	b_mode;			/* editor mode of this buffer	*/
        char    b_fname[NFILEN];        /* File name                    */
        char    b_bname[NBUFN];         /* Buffer name                  */
}       BUFFER;

#define BFTEMP  0x01                    /* Internal temporary buffer    */
#define BFCHG   0x02                    /* Changed since last write     */

/*	mode flags	*/
#define	NUMMODES	6		/* # of defined modes		*/

#define	MDWRAP	0x0001			/* word wrap			*/
#define	MDCMOD	0x0002			/* C indentation and fence match*/
#define	MDSPELL	0x0004			/* spell error parcing		*/
#define	MDEXACT	0x0008			/* Exact matching for searches	*/
#define	MDVIEW	0x0010			/* read-only buffer		*/
#define MDOVER	0x0020			/* overwrite mode		*/

/*
 * The starting position of a region, and the size of the region in
 * characters, is kept in a region structure.  Used by the region commands.
 */
typedef struct  {
        struct  LINE *r_linep;          /* Origin LINE address.         */
        int   r_offset;               /* Origin LINE offset.          */
        int   r_size;                 /* Length in characters.        */
}       REGION;

/*
 * All text is kept in circularly linked lists of "LINE" structures. These
 * begin at the header line (which is the blank line beyond the end of the
 * buffer). This line is pointed to by the "BUFFER". Each line contains a the
 * number of bytes in the line (the "used" size), the size of the text array,
 * and the text. The end of line is not stored as a byte; it's implied. Future
 * additions will include update hints, and a list of marks into the line.
 */
typedef struct  LINE {
        struct  LINE *l_fp;             /* Link to the next line        */
        struct  LINE *l_bp;             /* Link to the previous line    */
        int   l_size;                 /* Allocated size               */
        int   l_used;                 /* Used size                    */
        char    l_text[1];              /* A bunch of characters.       */
}       LINE;

#define lforw(lp)       ((lp)->l_fp)
#define lback(lp)       ((lp)->l_bp)
#define lgetc(lp, n)    ((lp)->l_text[(n)]&0xFF)
#define lputc(lp, n, c) ((lp)->l_text[(n)]=(c))
#define llength(lp)     ((lp)->l_used)

/*
 * The editor communicates with the display using a high level interface. A
 * "TERM" structure holds useful variables, and indirect pointers to routines
 * that do useful operations. The low level get and put routines are here too.
 * This lets a terminal, in addition to having non standard commands, have
 * funny get and put character code too. The calls might get changed to
 * "termp->t_field" style in the future, to make it possible to run more than
 * one terminal type.
 */
typedef struct  {
        int   t_nrow;                 /* Number of rows.              */
        int   t_ncol;                 /* Number of columns.           */
	int	t_margin;		/* min margin for extended lines*/
	int	t_scrsiz;		/* size of scroll region "	*/
        int     (*t_open)();            /* Open terminal at the start.  */
        int     (*t_close)();           /* Close terminal at end.       */
        int     (*t_getchar)();         /* Get character from keyboard. */
        int     (*t_putchar)();         /* Put character to display.    */
        int     (*t_flush)();           /* Flush output buffers.        */
        int     (*t_move)();            /* Move the cursor, origin 0.   */
        int     (*t_eeol)();            /* Erase to end of line.        */
        int     (*t_eeop)();            /* Erase to end of page.        */
        int     (*t_beep)();            /* Beep.                        */
	int	(*t_rev)();		/* set reverse video state	*/
}       TERM;

/*	structure for the table of initial key bindings		*/

typedef struct  {
        int   k_code;                 /* Key code                     */
        int     (*k_fp)();              /* Routine to handle it         */
}       KEYTAB;

/*	structure for the name binding table		*/

typedef struct {
	char *n_name;		/* name of function key */
	int (*n_func)();	/* function name is bound to */
}	NBIND;
