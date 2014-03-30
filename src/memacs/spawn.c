#include <../../nrnconf.h>
/* /local/src/master/nrn/src/memacs/spawn.c,v 1.2 1995/04/03 13:56:50 hines Exp */
/*
spawn.c,v
 * Revision 1.2  1995/04/03  13:56:50  hines
 * Port to MSWindows
 *
 * Revision 1.1.1.1  1994/10/12  17:21:25  hines
 * NEURON 3.0 distribution
 *
 * Revision 1.6  92/08/12  08:24:13  hines
 * saber free except for ARGSUSED which should be suppressed everywhere
 * 
 * Revision 1.5  92/08/12  08:16:49  hines
 * sejnowski changes to allow compiling on ncube using EXPRESS c libraries.
 * it allows compilation of neuron but doesn't actually work.
 * 
 * Revision 1.4  89/07/10  11:04:20  mlh
 * lint free
 * 
 * 
 * Revision 1.3  89/07/10  10:26:03  mlh
 * LINT free
 * 
 * Revision 1.2  89/07/09  12:19:09  mlh
 * lint pass1 now ok
 * 
 * Revision 1.1  89/07/08  15:37:05  mlh
 * Initial revision
 * 
*/

/*
 * The routines in this file are called to create a subjob running a command
 * interpreter. This code is a big fat nothing on CP/M-86. You lose.
 */

#include        <stdio.h>
#include	<unistd.h>
#include	"estruct.h"
#include        "edef.h"

#undef IGNORE
#define IGNORE(arg) if (arg) {;}

#if     AMIGA
#define  NEW   1006
#endif

#if     VMS
#define EFN     0                               /* Event flag.          */

#include        <ssdef.h>                       /* Random headers.      */
#include        <stsdef.h>
#include        <descrip.h>
#include        <iodef.h>

extern  int     oldmode[];                      /* In "termio.c"        */
extern  int     newmode[];                      /* In "termio.c"        */
extern  short   iochan;                         /* In "termio.c"        */
#endif

#if     MSDOS & LATTICE
#include        <dos.h>
#undef	CPM
#endif

#if MSDOS & MSC
#include <process.h>
#include <dos.h>
#endif

#if     V7
#if EXPRESS
#define SIGTSTP 18
#else
#include        <signal.h>
#endif
extern int vttidy();
#endif

/*
 * Create a subjob with a copy of the command intrepreter in it. When the
 * command interpreter exits, mark the screen as garbage so that you do a full
 * repaint. Bound to "^X C". The message at the start in VMS puts out a newline.
 * Under some (unknown) condition, you don't get one free when DCL starts up.
 */
int spawncli(f, n)
{
#if     AMIGA
        long newcli;

        newcli = Open("CON:1/1/639/199/MicroEmacs Subprocess", NEW);
        mlwrite("[Starting new CLI]");
        sgarbf = TRUE;
        Execute("", newcli, 0);
        Close(newcli);
        return(TRUE);
#endif

#if     V7
        register char *cp;
        char    *getenv();
#endif
#if     VMS
        movecursor(term.t_nrow, 0);             /* In last line.        */
        mlputs("[Starting DCL]\r\n");
        (*term.t_flush)();                      /* Ignore "ttcol".      */
        sgarbf = TRUE;
        return (sys(NULL));                     /* NULL => DCL.         */
#endif
#if     CPM
        mlwrite("Not in CP/M-86");
#endif

#if     MSDOS & MSC
        movecursor(term.t_nrow, 0);             /* Seek to last line.   */
        (*term.t_flush)();
		spawnlp(P_WAIT,"command",NULL);
        sgarbf = TRUE;
        return(TRUE);
#endif

#if     MSDOS & AZTEC
        movecursor(term.t_nrow, 0);             /* Seek to last line.   */
		  (*term.t_flush)();
#if defined(_Windows) && !defined(WIN32)
#else
	IGNORE(system("command.com"));
#endif
        sgarbf = TRUE;
        return(TRUE);
#endif
#if     MSDOS & LATTICE
        movecursor(term.t_nrow, 0);             /* Seek to last line.   */
        (*term.t_flush)();
        sys("\\command.com", "");               /* Run CLI.             */
        sgarbf = TRUE;
        return(TRUE);
#endif
	LINTUSE(f) LINTUSE(n)
#if     V7
        movecursor(term.t_nrow, 0);             /* Seek to last line.   */
        (*term.t_flush)();
        ttclose();                              /* stty to old settings */
        if ((cp = getenv("SHELL")) != NULL && *cp != '\0')
                {IGNORE(system(cp));}
        else
#if	BSD
                {IGNORE(system("exec /bin/csh"));}
#else
                {IGNORE(system("exec /bin/sh"));}
#endif
        sgarbf = TRUE;
#if !defined(__MINGW32__)
        sleep(2);
#endif
        ttopen();
        return(TRUE);
#endif
}

#if	V7 & BSD

int bktoshell()		/* suspend MicroEMACS and wait to wake up */
{
	int pid;

	vttidy();
	pid = getpid();
	IGNORE(kill(pid,SIGTSTP));
	return 0;
}

int rtfrmshell()
{
	ttopen();
	curwp->w_flag = WFHARD;
	IGNORE(refresh(TRUE, 0));
	return 0;
}
#endif

/*
 * Run a one-liner in a subjob. When the command returns, wait for a single
 * character to be typed, then mark the screen as garbage so a full repaint is
 * done. Bound to "C-X !".
 */
int spawn(f, n)
{
        register int    s;
        char            line[NLINE];

	LINTUSE(f) LINTUSE(n)
#if     AMIGA
        long newcli;

        newcli = Open("CON:1/1/639/199/MicroEmacs Subprocess", NEW);
        if ((s=mlreply("CLI command: ", line, NLINE)) != TRUE)
                return (s);
        Execute(line,0,newcli);
        Close(newcli);
        (*term.t_getchar)();     /* Pause.               */
        sgarbf = TRUE;
        return(TRUE);
#endif
#if     VMS
        if ((s=mlreply("DCL command: ", line, NLINE)) != TRUE)
                return (s);
        (*term.t_putchar)('\n');                /* Already have '\r'    */
        (*term.t_flush)();
        s = sys(line);                          /* Run the command.     */
        mlputs("\r\n\n[End]");                  /* Pause.               */
        (*term.t_flush)();
        (*term.t_getchar)();
        sgarbf = TRUE;
        return (s);
#endif
#if     CPM
        mlwrite("Not in CP/M-86");
        return (FALSE);
#endif
#if     MSDOS
        if ((s=mlreply("MS-DOS command: ", line, NLINE)) != TRUE)
                return (s);
#if	MSC
		spawnlp(P_WAIT,line,NULL);
#else
#if defined(_Windows) && !defined(WIN32)
#else
		  {IGNORE(system(line));}
#endif
#endif
        mlputs("\r\n\n[End]");                  /* Pause.               */
        (*term.t_getchar)();     /* Pause.               */
        sgarbf = TRUE;
        return (TRUE);
#endif
#if     V7
        if ((s=mlreply("! ", line, NLINE)) != TRUE)
                return (s);
        (*term.t_putchar)('\n');                /* Already have '\r'    */
        (*term.t_flush)();
        ttclose();                              /* stty to old modes    */
        IGNORE(system(line));
#if !defined(__MINGW32__)
        sleep(2);
#endif
        ttopen();
        mlputs("[End]");                        /* Pause.               */
        (*term.t_flush)();
        while ((s = (*term.t_getchar)()) != '\r' && s != ' ')
		/*EMPTY*/
                ;
        sgarbf = TRUE;
        return (TRUE);
#endif
}

#if     VMS
/*
 * Run a command. The "cmd" is a pointer to a command string, or NULL if you
 * want to run a copy of DCL in the subjob (this is how the standard routine
 * LIB$SPAWN works. You have to do wierd stuff with the terminal on the way in
 * and the way out, because DCL does not want the channel to be in raw mode.
 */
int sys(cmd)
register char   *cmd;
{
        struct  dsc$descriptor  cdsc;
        struct  dsc$descriptor  *cdscp;
        long    status;
        long    substatus;
        long    iosb[2];

        status = SYS$QIOW(EFN, iochan, IO$_SETMODE, iosb, 0, 0,
                          oldmode, sizeof(oldmode), 0, 0, 0, 0);
        if (status!=SS$_NORMAL || (iosb[0]&0xFFFF)!=SS$_NORMAL)
                return (FALSE);
        cdscp = NULL;                           /* Assume DCL.          */
        if (cmd != NULL) {                      /* Build descriptor.    */
                cdsc.dsc$a_pointer = cmd;
                cdsc.dsc$w_length  = strlen(cmd);
                cdsc.dsc$b_dtype   = DSC$K_DTYPE_T;
                cdsc.dsc$b_class   = DSC$K_CLASS_S;
                cdscp = &cdsc;
        }
        status = LIB$SPAWN(cdscp, 0, 0, 0, 0, 0, &substatus, 0, 0, 0);
        if (status != SS$_NORMAL)
                substatus = status;
        status = SYS$QIOW(EFN, iochan, IO$_SETMODE, iosb, 0, 0,
                          newmode, sizeof(newmode), 0, 0, 0, 0);
        if (status!=SS$_NORMAL || (iosb[0]&0xFFFF)!=SS$_NORMAL)
                return (FALSE);
        if ((substatus&STS$M_SUCCESS) == 0)     /* Command failed.      */
                return (FALSE);
        return (TRUE);
}
#endif

#if	~AZTEC & ~MSC & MSDOS

/*
 * This routine, once again by Bob McNamara, is a C translation of the "system"
 * routine in the MWC-86 run time library. It differs from the "system" routine
 * in that it does not unconditionally append the string ".exe" to the end of
 * the command name. We needed to do this because we want to be able to spawn
 * off "command.com". We really do not understand what it does, but if you don't
 * do it exactly "malloc" starts doing very very strange things.
 */
int sys(cmd, tail)
char    *cmd;
char    *tail;
{
#if MWC_86
        register unsigned n;
        extern   char     *__end;

        n = __end + 15;
        n >>= 4;
        n = ((n + dsreg() + 16) & 0xFFF0) + 16;
        return(execall(cmd, tail, n));
#endif

#if LATTICE
        return forklp(cmd, tail, NULL);
#endif
#if MSC
        return spawnlp(P_WAIT, cmd, tail, NULL);
#endif
}
#endif
