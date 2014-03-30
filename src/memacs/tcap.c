#include <../../nrnconf.h>
/*LINTLIBRARY*/
/* /local/src/master/nrn/src/memacs/tcap.c,v 1.1.1.1 1994/10/12 17:21:25 hines Exp */
/*
tcap.c,v
 * Revision 1.1.1.1  1994/10/12  17:21:25  hines
 * NEURON 3.0 distribution
 *
 * Revision 1.2  89/07/10  10:26:06  mlh
 * LINT free
 * 
 * Revision 1.1  89/07/08  15:37:09  mlh
 * Initial revision
 * 
*/

/*	tcap:	Unix V5, V7 and BS4.2 Termcap video driver
		for MicroEMACS
*/

#define	termdef	1			/* don't define "term" external */

#include <stdio.h>
#include	"estruct.h"
#include        "edef.h"

#if TERMCAP

#define NROW    24
#define NCOL    80
#define	MARGIN	8
#define	SCRSIZ	64
#define BEL     0x07
#define ESC     0x1B

extern int tgetnum();
extern int tgetent();
extern int tputs();

static int putpad();
static int putnpad();

extern int      ttopen();
extern int      ttgetc();
extern int      ttputc();
extern int      ttflush();
extern int      ttclose();
extern int      tcapmove();
extern int      tcapeeol();
extern int      tcapeeop();
extern int      tcapbeep();
extern int	tcaprev();
extern int      tcapopen();
extern int      tput();
extern char     *tgoto();

#define TCAPSLEN 315
char tcapbuf[TCAPSLEN];
static char *UP, PC, *CM, *CE, *CL, *SO, *SE;
static int LI, CO;
/*mlh short ospeed;*/

TERM term = {
        NROW-1,
        NCOL,
	MARGIN,
	SCRSIZ,
        tcapopen,
        ttclose,
        ttgetc,
        ttputc,
        ttflush,
        tcapmove,
        tcapeeol,
        tcapeeop,
        tcapbeep,
        tcaprev
};

int tcapopen()

{
        char *getenv();
        char *t, *p, *tgetstr();
        char tcbuf[1024];
        char *tv_stype;
        char err_str[72];

        if ((tv_stype = getenv("TERM")) == NULL)
        {
                puts("Environment variable TERM not defined!");
                exit(1);
        }

        if((tgetent(tcbuf, tv_stype)) != 1)
        {
                Sprintf(err_str, "Unknown terminal type %s!", tv_stype);
                puts(err_str);
                exit(1);
        }

        p = tcapbuf;
        t = tgetstr("pc", &p);
        if(t)
                PC = *t;

        CL = tgetstr("cl", &p);
        CM = tgetstr("cm", &p);
        CE = tgetstr("ce", &p);
        UP = tgetstr("up", &p);
	SE = tgetstr("se", &p);
	SO = tgetstr("so", &p);
	if ((CO=tgetnum("co")) != -1)
		term.t_ncol = CO;
	if ((LI=tgetnum("li")) != -1)
		term.t_nrow = LI-1;
	if (SO != NULL)
		revexist = TRUE;

        if(CL == NULL || CM == NULL || UP == NULL)
        {
                puts("Incomplete termcap entry\n");
                exit(1);
        }

	if (CE == NULL)		/* will we be able to use clear to EOL? */
		eolexist = FALSE;
		
        if (p >= &tcapbuf[TCAPSLEN])
        {
                puts("Terminal description too big!\n");
                exit(1);
        }
        ttopen();
	return 0;
}

int tcapmove(row, col)
register int row, col;
{
        putpad(tgoto(CM, col, row));
	return 0;
}

int tcapeeol()
{
        putpad(CE);
	return 0;
}

int tcapeeop()
{
        putpad(CL);
	return 0;
}

int tcaprev(state)		/* change reverse video status */

int state;	/* FALSE = normal video, TRUE = reverse video */

{
	if (state) {
		if (SO != NULL)
			putpad(SO);
	} else {
		if (SE != NULL)
			putpad(SE);
	}
	return 0;
}

int tcapbeep()
{
        ttputc(BEL);
	return 0;
}

static int putpad(str)
char    *str;
{
        tputs(str, 1, ttputc);
	return 0;
}

static int putnpad(str, n)
char    *str;
{
        tputs(str, n, ttputc);
	return 0;
}

#else

int hello()
{
	return 0;
}

#endif /*TERMCAP*/
