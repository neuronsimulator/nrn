#include <../../nrnconf.h>
/* /local/src/master/nrn/src/memacs/termio.c,v 1.1.1.1 1994/10/12 17:21:25 hines Exp */
/*
termio.c,v
 * Revision 1.1.1.1  1994/10/12  17:21:25  hines
 * NEURON 3.0 distribution
 *
 * Revision 1.7  1993/11/04  15:54:28  hines
 * port to solaris2 (no more warnings)
 *
 * Revision 1.6  1993/02/15  08:47:06  hines
 * linux
 *
 * Revision 1.5  92/08/12  08:16:51  hines
 * sejnowski changes to allow compiling on ncube using EXPRESS c libraries.
 * it allows compilation of neuron but doesn't actually work.
 * 
 * Revision 1.4  92/02/21  14:39:30  hines
 * Calls InterViews event loop before blocking on input when
 * termio.c compiled with -DINTERVIEWS=1
 * 
 * Revision 1.3  89/07/10  11:04:35  mlh
 * lint free
 * 
 * 
 * Revision 1.2  89/07/10  10:26:09  mlh
 * LINT free
 * 
 * Revision 1.1  89/07/08  15:37:16  mlh
 * Initial revision
 * 
*/

/*
 * The functions in this file negotiate with the operating system for
 * characters, and write characters in a barely buffered fashion on the display.
 * All operating systems.
 */
#include        <stdio.h>
#include	"estruct.h"
/* termios.h will sometimes define CTRL and we want to avoid macro redefined
   warning */
#undef CTRL
#include        "edef.h"

#if     AMIGA
#define NEW 1006
#define AMG_MAXBUF      1024
static long terminal;
static char     scrn_tmp[AMG_MAXBUF+1];
static int      scrn_tmp_p = 0;
#endif

#if     VMS
#include        <stsdef.h>
#include        <ssdef.h>
#include        <descrip.h>
#include        <iodef.h>
#include        <ttdef.h>

#define NIBUF   128                     /* Input buffer size            */
#define NOBUF   1024                    /* MM says bug buffers win!     */
#define EFN     0                       /* Event flag                   */

char    obuf[NOBUF];                    /* Output buffer                */
int     nobuf;                  /* # of bytes in above    */
char    ibuf[NIBUF];                    /* Input buffer          */
int     nibuf;                  /* # of bytes in above  */
int     ibufi;                  /* Read index                   */
int     oldmode[2];                     /* Old TTY mode bits            */
int     newmode[2];                     /* New TTY mode bits            */
short   iochan;                  /* TTY I/O channel             */
#endif

#if     CPM
#include        <bdos.h>
#endif

#if     MSDOS & (LATTICE | MSC)
#undef  LATTICE
#undef	CPM
#include        <dos.h>
#undef	CPM
union REGS rg;		/* cpu register for use of DOS calls */
int nxtchar = -1;	/* character held from type ahead    */
#endif

#if	MSDOS & AZTEC
struct regs {		/* cpu register for use of DOS calls */
	int ax, bx, cx, dx, si, di, ds, es; } rg;
int nxtchar = -1;	/* character held from type ahead    */
#endif

#if RAINBOW
#include "rainbow.h"
#endif

#if V7 && !defined(__MINGW32__) && !HAVE_TERMIO_H /* (SYSV == 0) & (LINUX == 0)*/
#undef	CTRL
#include        <sgtty.h>        /* for stty/gtty functions */
#include	<signal.h>
struct  sgttyb  ostate;          /* saved tty state */
struct  sgttyb  nstate;          /* values for editor mode */
struct tchars	otchars;	/* Saved terminal special character set */
struct tchars	ntchars = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
				/* A lot of nothing */
#ifndef HAVE_STTY
/* os x 4 and gcc 4.0.0 do define this */
#if !defined(stty)
#define stty(a,b) ioctl(a,TIOCSETP,b)
#define gtty(a,b) ioctl(a,TIOCGETP,b)
#endif
#endif

#if BSD
#include <sys/ioctl.h>		/* to get at the typeahead */
extern	int rtfrmshell();	/* return from suspended shell */
#define	TBUFSIZ	128
/*mlh char tobuf[TBUFSIZ];*/		/* terminal output buffer */
#endif
#endif

#ifdef HAVE_TERMIO_H /* #if SYSV | LINUX*/
#include <termio.h>
struct termio ostate;		/* saved tty state */
struct termio nstate;		/* new tty state */
#endif

#if EXPRESS 
/* ioctl erroneously defined in /express/include/sys/expname.h */
#undef ioctl   
#endif

/*
 * This function is called once to set up the terminal device streams.
 * On VMS, it translates TT until it finds the terminal, then assigns
 * a channel to it and sets it raw. On CPM it is a no-op.
 */
int ttopen()
{
#if     AMIGA
        terminal = Open("RAW:1/1/639/199/MicroEMACS 3.6/Amiga", NEW);
#endif
#if     VMS
        struct  dsc$descriptor  idsc;
        struct  dsc$descriptor  odsc;
        char    oname[40];
        int     iosb[2];
        int     status;

        odsc.dsc$a_pointer = "TT";
        odsc.dsc$w_length  = strlen(odsc.dsc$a_pointer);
        odsc.dsc$b_dtype        = DSC$K_DTYPE_T;
        odsc.dsc$b_class        = DSC$K_CLASS_S;
        idsc.dsc$b_dtype        = DSC$K_DTYPE_T;
        idsc.dsc$b_class        = DSC$K_CLASS_S;
        do {
                idsc.dsc$a_pointer = odsc.dsc$a_pointer;
                idsc.dsc$w_length  = odsc.dsc$w_length;
                odsc.dsc$a_pointer = &oname[0];
                odsc.dsc$w_length  = sizeof(oname);
                status = LIB$SYS_TRNLOG(&idsc, &odsc.dsc$w_length, &odsc);
                if (status!=SS$_NORMAL && status!=SS$_NOTRAN)
                        exit(status);
                if (oname[0] == 0x1B) {
                        odsc.dsc$a_pointer += 4;
                        odsc.dsc$w_length  -= 4;
                }
        } while (status == SS$_NORMAL);
        status = SYS$ASSIGN(&odsc, &iochan, 0, 0);
        if (status != SS$_NORMAL)
                exit(status);
        status = SYS$QIOW(EFN, iochan, IO$_SENSEMODE, iosb, 0, 0,
                          oldmode, sizeof(oldmode), 0, 0, 0, 0);
        if (status!=SS$_NORMAL || (iosb[0]&0xFFFF)!=SS$_NORMAL)
                exit(status);
        newmode[0] = oldmode[0];
        newmode[1] = oldmode[1] | TT$M_PASSALL | TT$M_NOECHO;
        status = SYS$QIOW(EFN, iochan, IO$_SETMODE, iosb, 0, 0,
                          newmode, sizeof(newmode), 0, 0, 0, 0);
        if (status!=SS$_NORMAL || (iosb[0]&0xFFFF)!=SS$_NORMAL)
                exit(status);
#endif
#if     CPM
#endif

#if     MSDOS & (HP150 == 0) & (LATTICE | MSC)
	/* kill the ctrl-break interupt */
	rg.h.ah = 0x33;		/* control-break check dos call */
	rg.h.al = 1;		/* set the current state */
	rg.h.dl = 0;		/* set it OFF */
	intdos(&rg, &rg);	/* go for it! */
#endif

#if     V7 & !defined(__MINGW32__) & !HAVE_TERMIO_H /* (SYSV == 0) & (LINUX == 0)*/
        IGNORE(gtty(0, &ostate));                       /* save old state */
        IGNORE(gtty(0, &nstate));                       /* get base of new state */
        nstate.sg_flags |= RAW;
        nstate.sg_flags &= ~(ECHO|CRMOD);       /* no echo for now... */
        IGNORE(stty(0, &nstate));                       /* set mode */
	IGNORE(ioctl(0, TIOCGETC, &otchars));		/* Save old characters */
	IGNORE(ioctl(0, TIOCSETC, &ntchars));		/* Place new character into K */
#if	BSD
	/* provide a smaller terminal output buffer so that
	   the type ahead detection works better (more often) */
/*	setbuffer(stdout, &tobuf[0], TBUFSIZ);*/
	IGNORE(signal(SIGTSTP,SIG_DFL));	/* set signals so that we can */
	IGNORE(signal(SIGCONT,rtfrmshell));	/* suspend & restart emacs */
#endif
#endif

#if HAVE_TERMIO_H /* SYSV | LINUX*/
	IGNORE(ioctl(0, TCGETA, &ostate));		/* snarf copy of old state */
	IGNORE(ioctl(0, TCGETA, &nstate));		/* and another to hack on */
	nstate.c_iflag &= ~(BRKINT | INPCK | IXON | IXOFF | IXANY);
	nstate.c_lflag &= ~(ISIG | ICANON | ECHO);
	nstate.c_cc[4]=1;
	nstate.c_cc[5]=1;
	IGNORE(ioctl(0, TCSETA, &nstate));
#endif
	return 0;
}

/*
 * This function gets called just before we go back home to the command
 * interpreter. On VMS it puts the terminal back in a reasonable state.
 * Another no-operation on CPM.
 */
int ttclose(void)
{
#if     AMIGA
        amg_flush();
        Close(terminal);
#endif
#if     VMS
        int     status;
        int     iosb[1];

        ttflush();
        status = SYS$QIOW(EFN, iochan, IO$_SETMODE, iosb, 0, 0,
                 oldmode, sizeof(oldmode), 0, 0, 0, 0);
        if (status!=SS$_NORMAL || (iosb[0]&0xFFFF)!=SS$_NORMAL)
                exit(status);
        status = SYS$DASSGN(iochan);
        if (status != SS$_NORMAL)
                exit(status);
#endif
#if     CPM
#endif
#if     MSDOS & (HP150 == 0) & (LATTICE | MSC)
	/* restore the ctrl-break interupt */
	rg.h.ah = 0x33;		/* control-break check dos call */
	rg.h.al = 1;		/* set the current state */
	rg.h.dl = 1;		/* set it ON */
	intdos(&rg, &rg);	/* go for it! */
#endif

#if     V7 && !defined(__MINGW32__) && !HAVE_TERMIO_H /* (SYSV == 0) & (LINUX == 0)*/
        IGNORE(stty(0, &ostate));
	IGNORE(ioctl(0, TIOCSETC, &otchars));	/* Place old character into K */
#endif

#if HAVE_TERMIO_H /* SYSV | LINUX */
	IGNORE(ioctl(0, TCSETA, &ostate));
#endif
	return 0;
}

/*
 * Write a character to the display. On VMS, terminal output is buffered, and
 * we just put the characters in the big array, after checking for overflow.
 * On CPM terminal I/O unbuffered, so we just write the byte out. Ditto on
 * MS-DOS (use the very very raw console output routine).
 */
int  ttputc(char c)
{
#if     AMIGA
        scrn_tmp[scrn_tmp_p++] = c;
        if(scrn_tmp_p>=AMG_MAXBUF)
                amg_flush();
#endif
#if     VMS
        if (nobuf >= NOBUF)
                ttflush();
        obuf[nobuf++] = c;
#endif

#if     CPM
        bios(BCONOUT, c, 0);
#endif

#if     MSDOS & MWC86
        dosb(CONDIO, c, 0);
#endif

#if	MSDOS & (LATTICE | MSC)
	bdos(6, c, 0);
#endif

#if	MSDOS & AZTEC
	bdos(6, c, 0);
#endif

#if RAINBOW
        Put_Char(c);                    /* fast video */
#endif

#if     V7
        IGNORE(fputc(c, stdout));
#endif
	return 0;
}

#if	AMIGA
int amg_flush(void)
{
        if(scrn_tmp_p)
                Write(terminal,scrn_tmp,scrn_tmp_p);
        scrn_tmp_p = 0;
	return 0;
}
#endif

/*
 * Flush terminal buffer. Does real work where the terminal output is buffered
 * up. A no-operation on systems where byte at a time terminal I/O is done.
 */
int  ttflush(void)
{
#if     AMIGA
        amg_flush();
#endif
#if     VMS
        int     status;
        int     iosb[2];

        status = SS$_NORMAL;
        if (nobuf != 0) {
                status = SYS$QIOW(EFN, iochan, IO$_WRITELBLK|IO$M_NOFORMAT,
                         iosb, 0, 0, obuf, nobuf, 0, 0, 0, 0);
                if (status == SS$_NORMAL)
                        status = iosb[0] & 0xFFFF;
                nobuf = 0;
        }
        return (status);
#endif
#if     CPM
#endif
#if     MSDOS
#endif
#if     V7
        IGNORE(fflush(stdout));
#endif
	return 0;
}

/*
 * Read a character from the terminal, performing no editing and doing no echo
 * at all. More complex in VMS that almost anyplace else, which figures. Very
 * simple on CPM, because the system can do exactly what you want.
 */
int ttgetc(void)
{
#if     AMIGA
        char ch;
        amg_flush();
        Read(terminal, &ch, 1);
        return(255 & (int)ch);
#endif
#if     VMS
        int     status;
        int     iosb[2];
        int     term[2];

        while (ibufi >= nibuf) {
                ibufi = 0;
                term[0] = 0;
                term[1] = 0;
                status = SYS$QIOW(EFN, iochan, IO$_READLBLK|IO$M_TIMED,
                         iosb, 0, 0, ibuf, NIBUF, 0, term, 0, 0);
                if (status != SS$_NORMAL)
                        exit(status);
                status = iosb[0] & 0xFFFF;
                if (status!=SS$_NORMAL && status!=SS$_TIMEOUT)
                        exit(status);
                nibuf = (iosb[0]>>16) + (iosb[1]>>16);
                if (nibuf == 0) {
                        status = SYS$QIOW(EFN, iochan, IO$_READLBLK,
                                 iosb, 0, 0, ibuf, 1, 0, term, 0, 0);
                        if (status != SS$_NORMAL
                        || (status = (iosb[0]&0xFFFF)) != SS$_NORMAL)
                                exit(status);
                        nibuf = (iosb[0]>>16) + (iosb[1]>>16);
                }
        }
        return (ibuf[ibufi++] & 0xFF);    /* Allow multinational  */
#endif

#if     CPM
        return (biosb(BCONIN, 0, 0));
#endif

#if RAINBOW
        int Ch;

        while ((Ch = Read_Keyboard()) < 0);

        if ((Ch & Function_Key) == 0)
                if (!((Ch & 0xFF) == 015 || (Ch & 0xFF) == 0177))
                        Ch &= 0xFF;

        return Ch;
#endif

#if     MSDOS & MWC86
        return (dosb(CONRAW, 0, 0));
#endif

#if	MSDOS & (LATTICE | MSC)
	int c;		/* character read */
	int flags;	/* cpu flags after dos call */

	/* if a char already is ready, return it */
	if (nxtchar >= 0) {
		c = nxtchar;
		nxtchar = -1;
		return(c);
	}

	/* call the dos to get a char */
	rg.h.ah = 7;		/* dos Direct Console I/O call */
	intdos(&rg, &rg);
	c = rg.h.al;		/* grab the char */

	return(c & 255);
#endif

#if	MSDOS & AZTEC
	int c;		/* character read */
	int flags;	/* cpu flags after dos call */

	/* if a char already is ready, return it */
	if (nxtchar >= 0) {
		c = nxtchar;
		nxtchar = -1;
		return(c);
	}

	/* call the dos to get a char until one is there */
	flags = 255;
	while ((flags & 64) != 0) {	/* while we don't yet have a char */
		rg.ax = 1536;		/* dos Direct Console I/O call */
		rg.dx = 255;		/*     console input */
		flags = sysint(33, &rg, &rg);
		c = rg.ax & 255;	/* grab the char */
	}

	return(c);
#endif

#if     V7
#if INTERVIEWS
	{extern int hoc_interviews; int run_til_stdin();
		if (hoc_interviews) {
			run_til_stdin();
		}
	}
#endif /*INTERVIEWS*/
#if 0 && defined(__MINGW32__)
	return getch();
#else
        return(127 & fgetc(stdin));
#endif
#endif
}

#if	TYPEAH
/* typahead:	Check to see if any characters are already in the
		keyboard buffer
*/

int typahead(void)

{
#if	MSDOS
	int c;		/* character read */
	int flags;	/* cpu flags from dos call */

#if MSC
	if (kbhit() != 0)
		return(TRUE);
	else
		return(FALSE);
#endif

#if  LATTICE

	if (nxtchar >= 0)
		return(TRUE);

	rg.h.ah = 6;	/* Direct Console I/O call */
	rg.h.dl = 255;	/*         does console input */
	flags = intdos(&rg, &rg);
	c = rg.h.al;	/* grab the character */

	/* no character pending */
	if ((flags & 64) != 0)
		return(FALSE);

	/* save the character and return true */
	nxtchar = c;
	return(TRUE);
#endif

#if	AZTEC
	if (nxtchar >= 0)
		return(TRUE);

	rg.ax = 1536;	/* Direct Console I/O call */
	rg.dx = 255;	/*         does console input */
	flags = sysint(33, &rg, &rg);
	c = rg.ax & 255; /* grab the character */

	/* no character pending */
	if ((flags & 64) != 0)
		return(FALSE);

	/* save the character and return true */
	nxtchar = c;
	return(TRUE);
#endif
#endif

#if	V7 & BSD
	int x;	/* holds # of pending chars */

	return((ioctl(0,FIONREAD,&x) < 0) ? 0 : x);
#endif
	return(FALSE);
}
#endif

