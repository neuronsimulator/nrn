#include <../../nrnconf.h>
/* /local/src/master/nrn/src/memacs/random.c,v 1.1.1.1 1994/10/12 17:21:25 hines Exp */
/*
random.c,v
 * Revision 1.1.1.1  1994/10/12  17:21:25  hines
 * NEURON 3.0 distribution
 *
 * Revision 1.4  92/08/12  08:23:36  hines
 * saber free except for ARGSUSED which should be suppressed everywhere
 * 
 * Revision 1.3  89/07/10  10:25:55  mlh
 * LINT free
 * 
 * Revision 1.2  89/07/09  12:18:56  mlh
 * lint pass1 now ok
 * 
 * Revision 1.1  89/07/08  15:36:53  mlh
 * Initial revision
 * 
*/

/*
 * This file contains the command processing functions for a number of random
 * commands. There is no functional grouping here, for sure.
 */

#include        <stdio.h>
#include	"estruct.h"
#include        "edef.h"

/*
 * Set fill column to n.
 */
int setfillcol(f, n)
{
	LINTUSE(f)
        fillcol = n;
	mlwrite("[Fill column is %d]",n);
        return(TRUE);
}

/*
 * Display the current position of the cursor, in origin 1 X-Y coordinates,
 * the character that is under the cursor (in octal), and the fraction of the
 * text that is before the cursor. The displayed column is not the current
 * column, but the column that would be used on an infinite width display.
 * Normally this is bound to "C-X =".
 */
int showcpos(f, n)
{
        register LINE   *clp;
        register long   nch;
        register int    cbo;
        register long   nbc;
        register int    cac;
        register int    ratio;
        register int    col;
	register int	nline;
	char buf[200];

	LINTUSE(f) LINTUSE(n)
	nline = 0;
        clp = lforw(curbp->b_linep);            /* Grovel the data.     */
        cbo = 0;
        nch = 0;
        for (;;) {
                if (clp==curwp->w_dotp && cbo==curwp->w_doto) {
                        nbc = nch;
                        if (cbo == llength(clp))
                                cac = '\n';
                        else
                                cac = lgetc(clp, cbo);
                }
                if (cbo == llength(clp)) {
                        if (clp == curbp->b_linep)
                                break;
                        clp = lforw(clp);
                        cbo = 0;
                } else
                        ++cbo;
                ++nch;
        }
	for (clp = curbp->b_linep; clp != curwp->w_dotp; clp = lforw(clp)){
		++nline;
	}
        col = getccol(FALSE);                   /* Get real column.     */
        ratio = 0;                              /* Ratio before dot.    */
        if (nch != 0)
                ratio = (100L*nbc) / nch;
        sprintf(buf, "line=%d X=%d Y=%d CH=0x%x .=%ld (%d%% of %ld)",
                nline, col+1, currow+1, cac, nbc, ratio, nch);
        mlwrite(buf);
        return (TRUE);
}

/*
 * Return current column.  Stop at first non-blank given TRUE argument.
 */
int getccol(bflg)
int bflg;
{
        register int c, i, col;
        col = 0;
        for (i=0; i<curwp->w_doto; ++i) {
                c = lgetc(curwp->w_dotp, i);
                if (c!=' ' && c!='\t' && bflg)
                        break;
                if (c == '\t')
#if TRUETAB
			col += tabsize - col%tabsize - 1;
#else
                        col |= 0x07;
#endif
                else if (c<0x20 || c==0x7F)
                        ++col;
                ++col;
        }
        return(col);
}

/*
 * Twiddle the two characters on either side of dot. If dot is at the end of
 * the line twiddle the two characters before it. Return with an error if dot
 * is at the beginning of line; it seems to be a bit pointless to make this
 * work. This fixes up a very common typo with a single stroke. Normally bound
 * to "C-T". This always works within a line, so "WFEDIT" is good enough.
 */
int twiddle(f, n)
{
        register LINE   *dotp;
        register int    doto;
        register int    cl;
        register int    cr;

	LINTUSE(f) LINTUSE(n)
	if (curbp->b_mode&MDVIEW)	/* don't allow this command if	*/
		return(rdonly());	/* we are in read only mode	*/
        dotp = curwp->w_dotp;
        doto = curwp->w_doto;
        if (doto==llength(dotp) && --doto<0)
                return (FALSE);
        cr = lgetc(dotp, doto);
        if (--doto < 0)
                return (FALSE);
        cl = lgetc(dotp, doto);
        lputc(dotp, doto+0, cr);
        lputc(dotp, doto+1, cl);
        lchange(WFEDIT);
        return (TRUE);
}

/*
 * Quote the next character, and insert it into the buffer. All the characters
 * are taken literally, with the exception of the newline, which always has
 * its line splitting meaning. The character is always read, even if it is
 * inserted 0 times, for regularity. Bound to "C-Q"
 */
int quote(f, n)
{
        register int    s;
        register int    c;

	LINTUSE(f)
	if (curbp->b_mode&MDVIEW)	/* don't allow this command if	*/
		return(rdonly());	/* we are in read only mode	*/
        c = (*term.t_getchar)();
        if (n < 0)
                return (FALSE);
        if (n == 0)
                return (TRUE);
        if (c == '\n') {
                do {
                        s = lnewline();
                } while (s==TRUE && --n);
                return (s);
        }
        return (linsert(n, c));
}

#if TRUETAB
/*
 * Set tab size if given non-default argument (n <> 1).  Otherwise, insert a
 * tab into file.
 */
int tab(f, n)
{
	LINTUSE(f)
        if (n < 0)
                return (FALSE);
        if (n == 0 || n > 1) {
                tabsize = n;
                return(TRUE);
        }
        return(linsert(1, '\t'));
}
#else
/*
 * Set tab size if given non-default argument (n <> 1).  Otherwise, insert a
 * tab into file.  If given argument, n, of zero, change to true tabs.
 * If n > 1, simulate tab stop every n-characters using spaces. This has to be
 * done in this slightly funny way because the tab (in ASCII) has been turned
 * into "C-I" (in 10 bit code) already. Bound to "C-I".
 */
int tab(f, n)
{
        if (n < 0)
                return (FALSE);
        if (n == 0 || n > 1) {
                tabsize = n;
                return(TRUE);
        }
        if (! tabsize)
                return(linsert(1, '\t'));
        return(linsert(tabsize - (getccol(FALSE) % tabsize), ' '));
}
#endif

/*
 * Open up some blank space. The basic plan is to insert a bunch of newlines,
 * and then back up over them. Everything is done by the subcommand
 * procerssors. They even handle the looping. Normally this is bound to "C-O".
 */
int openline(f, n)
{
        register int    i;
        register int    s;

	if (curbp->b_mode&MDVIEW)	/* don't allow this command if	*/
		return(rdonly());	/* we are in read only mode	*/
        if (n < 0)
                return (FALSE);
        if (n == 0)
                return (TRUE);
        i = n;                                  /* Insert newlines.     */
        do {
                s = lnewline();
        } while (s==TRUE && --i);
        if (s == TRUE)                          /* Then back up overtop */
                s = backchar(f, n);             /* of them all.         */
        return (s);
}

/*
 * Insert a newline. Bound to "C-M". If we are in CMODE, do automatic
 * indentation as specified.
 */
int newline(f, n)
{
	register int    s;

	LINTUSE(f)
	if (curbp->b_mode&MDVIEW)	/* don't allow this command if	*/
		return(rdonly());	/* we are in read only mode	*/
	if (n < 0)
		return (FALSE);

	/* if we are in C mode and this is a default <NL> */
	if (n == 1 && (curbp->b_mode & MDCMOD) &&
	    curwp->w_dotp != curbp->b_linep)
		return(cinsert());

	/* insert some lines */
	while (n--) {
		if ((s=lnewline()) != TRUE)
			return (s);
	}
	return (TRUE);
}

int cinsert()	/* insert a newline and indentation for C */

{
	register char *cptr;	/* string pointer into text to copy */
	register int tptr;	/* index to scan into line */
	register int bracef;	/* was there a brace at the end of line? */
	register int i;
	char ichar[NSTRING];	/* buffer to hold indent of last line */

	/* grab a pointer to text to copy indentation from */
	cptr = &curwp->w_dotp->l_text[0];

	/* check for a brace */
	tptr = curwp->w_doto - 1;
	bracef = (cptr[tptr] == '{');

	/* save the indent of the previous line */
	i = 0;
	while ((i < tptr) && (cptr[i] == ' ' || cptr[i] == '\t')
		&& (i < NSTRING - 1)) {
		ichar[i] = cptr[i];
		++i;
	}
	ichar[i] = 0;		/* terminate it */

	/* put in the newline */
	if (lnewline() == FALSE)
		return(FALSE);

	/* and the saved indentation */
	i = 0;
	while (ichar[i])
		IGNORE(linsert(1, ichar[i++]));

	/* and one more tab for a brace */
	if (bracef)
		{IGNORE(tab(FALSE, 1));}

	return(TRUE);
}

int insbrace(n, c)	/* insert a brace into the text here...we are in CMODE */

int n;	/* repeat count */
int c;	/* brace to insert (always { for now) */

{
	register int ch;	/* last character before input */
	register int i;
	register int target;	/* column brace should go after */

	/* if we are at the begining of the line, no go */
	if (curwp->w_doto == 0)
		return(linsert(n,c));
		
	/* scan to see if all space before this is white space */
	for (i = curwp->w_doto - 1; i >= 0; --i) {
		ch = lgetc(curwp->w_dotp, i);
		if (ch != ' ' && ch != '\t')
			return(linsert(n, c));
	}

	/* delete back first */
	target = getccol(FALSE);	/* calc where we will delete to */
	target -= 1;
#if TRUETAB
	target -= target % tabsize;
#else
	target -= target % (tabsize == 0 ? 8 : tabsize);
#endif
	while (getccol(FALSE) > target)
		{IGNORE(backdel(FALSE, 1));}
	/* and insert the required brace(s) */
	return(linsert(n, c));
}

int inspound()	/* insert a # into the text here...we are in CMODE */

{
	register int ch;	/* last character before input */
	register int i;

	/* if we are at the begining of the line, no go */
	if (curwp->w_doto == 0)
		return(linsert(1,'#'));
		
	/* scan to see if all space before this is white space */
	for (i = curwp->w_doto - 1; i >= 0; --i) {
		ch = lgetc(curwp->w_dotp, i);
		if (ch != ' ' && ch != '\t')
			return(linsert(1, '#'));
	}

	/* delete back first */
	while (getccol(FALSE) > 1)
		{IGNORE(backdel(FALSE, 1));}

	/* and insert the required pound */
	return(linsert(1, '#'));
}

/*
 * Delete blank lines around dot. What this command does depends if dot is
 * sitting on a blank line. If dot is sitting on a blank line, this command
 * deletes all the blank lines above and below the current line. If it is
 * sitting on a non blank line then it deletes all of the blank lines after
 * the line. Normally this command is bound to "C-X C-O". Any argument is
 * ignored.
 */
int deblank(f, n)
{
        register LINE   *lp1;
        register LINE   *lp2;
        register int    nld;

	LINTUSE(f) LINTUSE(n)
	if (curbp->b_mode&MDVIEW)	/* don't allow this command if	*/
		return(rdonly());	/* we are in read only mode	*/
        lp1 = curwp->w_dotp;
        while (llength(lp1)==0 && (lp2=lback(lp1))!=curbp->b_linep)
                lp1 = lp2;
        lp2 = lp1;
        nld = 0;
        while ((lp2=lforw(lp2))!=curbp->b_linep && llength(lp2)==0)
                ++nld;
        if (nld == 0)
                return (TRUE);
        curwp->w_dotp = lforw(lp1);
        curwp->w_doto = 0;
        return (ldelete(nld, FALSE));
}

/*
 * Insert a newline, then enough tabs and spaces to duplicate the indentation
 * of the previous line. Assumes tabs are every eight characters. Quite simple.
 * Figure out the indentation of the current line. Insert a newline by calling
 * the standard routine. Insert the indentation by inserting the right number
 * of tabs and spaces. Return TRUE if all ok. Return FALSE if one of the
 * subcomands failed. Normally bound to "C-J".
 */
int indent(f, n)
{
        register int    nicol;
        register int    c;
        register int    i;

	LINTUSE(f)
	if (curbp->b_mode&MDVIEW)	/* don't allow this command if	*/
		return(rdonly());	/* we are in read only mode	*/
        if (n < 0)
                return (FALSE);
        while (n--) {
                nicol = 0;
                for (i=0; i<llength(curwp->w_dotp); ++i) {
                        c = lgetc(curwp->w_dotp, i);
                        if (c!=' ' && c!='\t')
                                break;
                        if (c == '\t')
#if TRUETAB
				nicol += tabsize - nicol%tabsize - 1;
#else
                                nicol |= 0x07;
#endif
                        ++nicol;
                }
                if (lnewline() == FALSE
                || ((i=nicol/tabsize)!=0 && linsert(i, '\t')==FALSE)
                || ((i=nicol%tabsize)!=0 && linsert(i,  ' ')==FALSE))
                        return (FALSE);
        }
        return (TRUE);
}

/*
 * Delete forward. This is real easy, because the basic delete routine does
 * all of the work. Watches for negative arguments, and does the right thing.
 * If any argument is present, it kills rather than deletes, to prevent loss
 * of text if typed with a big argument. Normally bound to "C-D".
 */
int forwdel(f, n)
{
	if (curbp->b_mode&MDVIEW)	/* don't allow this command if	*/
		return(rdonly());	/* we are in read only mode	*/
        if (n < 0)
                return (backdel(f, -n));
        if (f != FALSE) {                       /* Really a kill.       */
                if ((lastflag&CFKILL) == 0)
                        kdelete();
                thisflag |= CFKILL;
        }
        return (ldelete(n, f));
}

/*
 * Delete backwards. This is quite easy too, because it's all done with other
 * functions. Just move the cursor back, and delete forwards. Like delete
 * forward, this actually does a kill if presented with an argument. Bound to
 * both "RUBOUT" and "C-H".
 */
int backdel(f, n)
{
        register int    s;

	if (curbp->b_mode&MDVIEW)	/* don't allow this command if	*/
		return(rdonly());	/* we are in read only mode	*/
        if (n < 0)
                return (forwdel(f, -n));
        if (f != FALSE) {                       /* Really a kill.       */
                if ((lastflag&CFKILL) == 0)
                        kdelete();
                thisflag |= CFKILL;
        }
        if ((s=backchar(f, n)) == TRUE)
                s = ldelete(n, f);
        return (s);
}

/*
 * Kill text. If called without an argument, it kills from dot to the end of
 * the line, unless it is at the end of the line, when it kills the newline.
 * If called with an argument of 0, it kills from the start of the line to dot.
 * If called with a positive argument, it kills from dot forward over that
 * number of newlines. If called with a negative argument it kills backwards
 * that number of newlines. Normally bound to "C-K".
 */
int killtext(f, n)
{
        register int    chunk;
        register LINE   *nextp;

	if (curbp->b_mode&MDVIEW)	/* don't allow this command if	*/
		return(rdonly());	/* we are in read only mode	*/
        if ((lastflag&CFKILL) == 0)             /* Clear kill buffer if */
                kdelete();                      /* last wasn't a kill.  */
        thisflag |= CFKILL;
        if (f == FALSE) {
                chunk = llength(curwp->w_dotp)-curwp->w_doto;
                if (chunk == 0)
                        chunk = 1;
        } else if (n == 0) {
                chunk = curwp->w_doto;
                curwp->w_doto = 0;
        } else if (n > 0) {
                chunk = llength(curwp->w_dotp)-curwp->w_doto+1;
                nextp = lforw(curwp->w_dotp);
                while (--n) {
                        if (nextp == curbp->b_linep)
                                return (FALSE);
                        chunk += llength(nextp)+1;
                        nextp = lforw(nextp);
                }
        } else {
                mlwrite("neg kill");
                return (FALSE);
        }
        return (ldelete(chunk, TRUE));
}

/*
 * Yank text back from the kill buffer. This is really easy. All of the work
 * ms done by the standard insert routines. All you do is run the loop, and
 * check for errors. Bound to "C-Y".
 */
int yank(f, n)
{
        register int    c;
        register int    i;
        extern   unsigned    kused;

	LINTUSE(f)
	if (curbp->b_mode&MDVIEW)	/* don't allow this command if	*/
		return(rdonly());	/* we are in read only mode	*/
        if (n < 0)
                return (FALSE);
        while (n--) {
                i = 0;
                while ((c=kremove(i)) >= 0) {
                        if (c == '\n') {
                                if (lnewline() == FALSE)
                                        return (FALSE);
                        } else {
                                if (linsert(1, c) == FALSE)
                                        return (FALSE);
                        }
                        ++i;
                }
        }
        return (TRUE);
}

int setmode(f, n)	/* prompt and set an editor mode */

int f, n;	/* default and argument */

{
	LINTUSE(f) LINTUSE(n)
	IGNORE(adjustmode(TRUE, FALSE));
        return (TRUE);
}

int delmode(f, n)	/* prompt and delete an editor mode */

int f, n;	/* default and argument */

{
	LINTUSE(f) LINTUSE(n)
	IGNORE(adjustmode(FALSE, FALSE));
        return (TRUE);
}

int setgmode(f, n)	/* prompt and set a global editor mode */

int f, n;	/* default and argument */

{
	LINTUSE(f) LINTUSE(n)
	IGNORE(adjustmode(TRUE, TRUE));
        return (TRUE);
}

int delgmode(f, n)	/* prompt and delete a global editor mode */

int f, n;	/* default and argument */

{
	LINTUSE(f) LINTUSE(n)
	IGNORE(adjustmode(FALSE, TRUE));
        return (TRUE);
}

int adjustmode(kind, global)	/* change the editor mode status */

int kind;	/* true = set,		false = delete */
int global;	/* true = global flag,	false = current buffer flag */
{
	register char *scan;		/* scanning pointer to convert prompt */
	register int i;			/* loop index */
	char prompt[50];		/* string to prompt user with */
	char cbuf[NPAT];		/* buffer to recieve mode name into */

	/* build the proper prompt string */
	if (global)
		Strcpy(prompt,"Global mode to ");
	else
		Strcpy(prompt,"Mode to ");

	if (kind == TRUE)
		Strcat(prompt, "add: ");
	else
		Strcat(prompt, "delete: ");

	/* prompt the user and get an answer */

	IGNORE(mlreply(prompt, cbuf, NPAT - 1)); /*mlh really IGNORE */

	/* make it uppercase */

	scan = cbuf;
	while (*scan != 0) {
		if (*scan >= 'a' && *scan <= 'z')
			*scan = *scan - 32;
		scan++;
	}

	/* test it against the modes we know */

	for (i=0; i < NUMMODES; i++) {
		if (strcmp(cbuf, modename[i]) == 0) {
			/* finding a match, we process it */
			if (kind == TRUE)
				if (global)
					gmode |= (1 << i);
				else
					curwp->w_bufp->b_mode |= (1 << i);
			else
				if (global)
					gmode &= ~(1 << i);
				else
					curwp->w_bufp->b_mode &= ~(1 << i);
			/* display new mode line */
			if (global == 0)
				upmode();
			mlerase();	/* erase the junk */
			return(TRUE);
		}
	}

	mlwrite("No such mode!");
	return(FALSE);
}
