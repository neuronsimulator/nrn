#include <../../nrnconf.h>
/*LINTLIBRARY*/
/* /local/src/master/nrn/src/memacs/ansi.c,v 1.1.1.1 1994/10/12 17:21:23 hines Exp */
/*
ansi.c,v
 * Revision 1.1.1.1  1994/10/12  17:21:23  hines
 * NEURON 3.0 distribution
 *
 * Revision 1.2  89/07/10  10:24:51  mlh
 * LINT free
 * 
 * Revision 1.1  89/07/08  15:35:54  mlh
 * Initial revision
 * 
*/

/*
 * The routines in this file provide support for ANSI style terminals
 * over a serial line. The serial I/O services are provided by routines in
 * "termio.c". It compiles into nothing if not an ANSI device.
 */

#define	termdef	1			/* don't define "term" external */

#include        <stdio.h>
#include	"estruct.h"
#include        "edef.h"

#if     ANSI

#if	AMIGA
#define NROW    23                      /* Screen size.                 */
#define NCOL    77                      /* Edit if you want to.         */
#else
#define NROW    25                      /* Screen size.                 */
#define NCOL    80                      /* Edit if you want to.         */
#endif
#define	MARGIN	8			/* size of minimim margin and	*/
#define	SCRSIZ	64			/* scroll size for extended lines */
#define BEL     0x07                    /* BEL character.               */
#define ESC     0x1B                    /* ESC character.               */

extern  int     ttopen();               /* Forward references.          */
extern  int     ttgetc();
extern  int     ttputc();
extern  int     ttflush();
extern  int     ttclose();
extern  int     ansimove();
extern  int     ansieeol();
extern  int     ansieeop();
extern  int     ansibeep();
extern  int     ansiopen();
extern	int	ansirev();
/*
 * Standard terminal interface dispatch table. Most of the fields point into
 * "termio" code.
 */
TERM    term    = {
        NROW-1,
        NCOL,
	MARGIN,
	SCRSIZ,
        ansiopen,
        ttclose,
        ttgetc,
        ttputc,
        ttflush,
        ansimove,
        ansieeol,
        ansieeop,
        ansibeep,
	ansirev
};

int ansimove(row, col)
{
        ttputc(ESC);
        ttputc('[');
        ansiparm(row+1);
        ttputc(';');
        ansiparm(col+1);
        ttputc('H');
	return 0;
}

int ansieeol()
{
        ttputc(ESC);
        ttputc('[');
        ttputc('K');
	return 0;
}

int ansieeop()
{
        ttputc(ESC);
        ttputc('[');
        ttputc('J');
	return 0;
}

int ansirev(state)		/* change reverse video state */

int state;	/* TRUE = reverse, FALSE = normal */

{
	ttputc(ESC);
	ttputc('[');
	ttputc(state ? '7': '0');
	ttputc('m');
	return 0;
}

int ansibeep()
{
        ttputc(BEL);
        ttflush();
	return 0;
}

int ansiparm(n)
register int    n;
{
        register int    q;

        q = n/10;
        if (q != 0)
                ansiparm(q);
        ttputc((n%10) + '0');
	return 0;
}

int ansiopen()
{
#if     V7
        register char *cp;
        char *getenv();

        if ((cp = getenv("TERM")) == NULL) {
                puts("Shell variable TERM not defined!");
                exit(1);
        }
        if (strcmp(cp, "vt100") != 0) {
                puts("Terminal type not 'vt100'!");
                exit(1);
        }
#endif
	revexist = TRUE;
        ttopen();
	return 0;
}

#endif
