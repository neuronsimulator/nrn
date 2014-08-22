#include <../../nrnconf.h>
/* /local/src/master/nrn/src/memacs/display.c,v 1.2 1995/04/03 13:56:38 hines Exp */
/*
display.c,v
 * Revision 1.2  1995/04/03  13:56:38  hines
 * Port to MSWindows
 *
 * Revision 1.1.1.1  1994/10/12  17:21:24  hines
 * NEURON 3.0 distribution
 *
 * Revision 1.5  91/08/22  12:33:38  hines
 * works with turboc
 * 
 * Revision 1.4  91/08/13  16:33:50  hines
 * vtinit can be called more than once.
 * 
 * Revision 1.3  89/07/10  10:25:19  mlh
 * LINT free
 * 
 * Revision 1.2  89/07/09  12:18:35  mlh
 * lint pass1 now ok
 * 
 * Revision 1.1  89/07/08  15:36:17  mlh
 * Initial revision
 * 
*/

/*
 * The functions in this file handle redisplay. There are two halves, the
 * ones that update the virtual display screen, and the ones that make the
 * physical display screen the same as the virtual display screen. These
 * functions use hints that are left in the windows by the commands.
 *
 */

#include        <stdio.h>
#include	"estruct.h"
#include        "edef.h"

#define WFDEBUG 0                       /* Window flag debug. */

typedef struct  VIDEO {
        short   v_flag;                 /* Flags */
        char    v_text[1];              /* Screen data. */
}       VIDEO;

#define VFCHG   0x0001                  /* Changed flag			*/
#define	VFEXT	0x0002			/* extended (beyond column 80)	*/
#define	VFREV	0x0004			/* reverse video status		*/
#define	VFREQ	0x0008			/* reverse video request	*/

int     vtrow   = 0;                    /* Row location of SW cursor */
int     vtcol   = 0;                    /* Column location of SW cursor */
int     ttrow   = HUGE;                 /* Row location of HW cursor */
int     ttcol   = HUGE;                 /* Column location of HW cursor */
int	lbound	= 0;			/* leftmost column of current line
					   being displayed */

VIDEO   **vscreen;                      /* Virtual screen. */
VIDEO   **pscreen;                      /* Physical screen. */

/*
 * Initialize the data structures used by the display code. The edge vectors
 * used to access the screens are set up. The operating system's terminal I/O
 * channel is set up. All the other things get initialized at compile time.
 * The original window has "WFCHG" set, so that it will get completely
 * redrawn on the first call to "update".
 */
int vtinit()
{
    register int i;
    register VIDEO *vp;
	static int nrow, ncol;

    (*term.t_open)();
	if (nrow == term.t_nrow && ncol == term.t_ncol) {
		return TRUE;
	}
    (*term.t_rev)(FALSE);
	if (vscreen) {
		for (i=0; i<nrow; ++i) {
			free((char *)vscreen[i]);
		}
		free((char *)vscreen);
	}
    vscreen = (VIDEO **) malloc((unsigned)(term.t_nrow*sizeof(VIDEO *)));

    if (vscreen == NULL)
        exit(1);

	if (pscreen) {
		for (i=0; i<nrow; ++i) {
			free((char *)pscreen[i]);
		}
		free((char *)pscreen);
	}
    pscreen = (VIDEO **) malloc((unsigned)(term.t_nrow*sizeof(VIDEO *)));

    if (pscreen == NULL)
        exit(1);

    for (i = 0; i < term.t_nrow; ++i)
        {
        vp = (VIDEO *) malloc((unsigned)(sizeof(VIDEO)+term.t_ncol));

        if (vp == NULL)
            exit(1);

	vp->v_flag = VFCHG;
        vscreen[i] = vp;
        vp = (VIDEO *) malloc((unsigned)(sizeof(VIDEO)+term.t_ncol));

        if (vp == NULL)
            exit(1);

	vp->v_flag = VFCHG;
        pscreen[i] = vp;
        }

	if (nrow) {
		WINDOW *wp;
		for (wp = wheadp; wp; wp = wp->w_wndp) {
			wp->w_ntrows = term.t_nrow - 1;
			wp->w_toprow = 0;
			wp->w_flag |= WFFORCE|WFMODE|WFHARD;
		}
	}
	nrow = term.t_nrow;
	ncol = term.t_ncol;
	return TRUE;
}

/*
 * Clean up the virtual terminal system, in anticipation for a return to the
 * operating system. Move down to the last line and clear it out (the next
 * system prompt will be written in the line). Shut down the channel to the
 * terminal.
 */
int vttidy()
{
    mlerase();
    movecursor(term.t_nrow, 0);
    (*term.t_close)();
    return TRUE;
}

/*
 * Set the virtual cursor to the specified row and column on the virtual
 * screen. There is no checking for nonsense values; this might be a good
 * idea during the early stages.
 */
int vtmove(row, col)
{
    vtrow = row;
    vtcol = col;
    return TRUE;
}

/*
 * Write a character to the virtual screen. The virtual row and column are
 * updated. If the line is too long put a "$" in the last column. This routine
 * only puts printing characters into the virtual terminal buffers. Only
 * column overflow is checked.
 */
int vtputc(c)
    int c;
{
    register VIDEO      *vp;

    vp = vscreen[vtrow];

    if (vtcol >= term.t_ncol) {
        vtcol = (vtcol + 0x07) & ~0x07;
        vp->v_text[term.t_ncol - 1] = '$';
    } else if (c == '\t')
        {
        do
            {
            vtputc(' ');
            }
#if TRUETAB
	while ((vtcol%tabsize) != 0);
#else
        while ((vtcol&0x07) != 0);
#endif
        }
    else if (c < 0x20 || c == 0x7F)
        {
        vtputc('^');
        vtputc(c ^ 0x40);
        }
    else
	vp->v_text[vtcol++] = c;
    return TRUE;
}

/*	put a character to the virtual screen in an extended line. If we are
	not yet on left edge, don't print it yet. check for overflow on
	the right margin						*/

int vtpute(c)

int c;

{
    register VIDEO      *vp;

    vp = vscreen[vtrow];

    if (vtcol >= term.t_ncol) {
        vtcol = (vtcol + 0x07) & ~0x07;
        vp->v_text[term.t_ncol - 1] = '$';
    } else if (c == '\t')
        {
        do
            {
            vtpute(' ');
            }
#if TRUETAB
	while (((vtcol + lbound)%tabsize) != 0);
#else
        while (((vtcol + lbound)&0x07) != 0);
#endif
        }
    else if (c < 0x20 || c == 0x7F)
        {
        vtpute('^');
        vtpute(c ^ 0x40);
        }
    else {
	if (vtcol >= 0)
		vp->v_text[vtcol] = c;
	++vtcol;
    }
    return TRUE;
}

/*
 * Erase from the end of the software cursor to the end of the line on which
 * the software cursor is located.
 */
int vteeol()
{
    register VIDEO      *vp;

    vp = vscreen[vtrow];
    while (vtcol < term.t_ncol)
        vp->v_text[vtcol++] = ' ';
    return TRUE;
}

/*
 * Make sure that the display is right. This is a three part process. First,
 * scan through all of the windows looking for dirty ones. Check the framing,
 * and refresh the screen. Second, make sure that "currow" and "curcol" are
 * correct for the current window. Third, make the virtual and physical
 * screens the same.
 */
int update()
{
    register LINE *lp;
    register WINDOW *wp;
    register VIDEO *vp1;
    register VIDEO *vp2;
    register int i;
    register int j;
    register int c;

#if	TYPEAH
    if (typahead())
	return(TRUE);
#endif

	/* update the reverse video flags for any mode lines out there */
	for (i = 0; i < term.t_nrow; ++i)
		vscreen[i]->v_flag &= ~VFREQ;

#if	REVSTA
	wp = wheadp;
	while (wp != NULL) {
		vscreen[wp->w_toprow+wp->w_ntrows]->v_flag |= VFREQ;
		wp = wp->w_wndp;
	}
#endif

    wp = wheadp;

    while (wp != NULL)
        {
        /* Look at any window with update flags set on. */

        if (wp->w_flag != 0)
            {
            /* If not force reframe, check the framing. */

            if ((wp->w_flag & WFFORCE) == 0)
                {
                lp = wp->w_linep;

                for (i = 0; i < wp->w_ntrows; ++i)
                    {
                    if (lp == wp->w_dotp)
                        goto out;

                    if (lp == wp->w_bufp->b_linep)
                        break;

                    lp = lforw(lp);
                    }
                }

            /* Not acceptable, better compute a new value for the line at the
             * top of the window. Then set the "WFHARD" flag to force full
             * redraw.
             */
            i = wp->w_force;

            if (i > 0)
                {
                --i;

                if (i >= wp->w_ntrows)
                  i = wp->w_ntrows-1;
                }
            else if (i < 0)
                {
                i += wp->w_ntrows;

                if (i < 0)
                    i = 0;
                }
            else
                i = wp->w_ntrows/2;

            lp = wp->w_dotp;

            while (i != 0 && lback(lp) != wp->w_bufp->b_linep)
                {
                --i;
                lp = lback(lp);
                }

            wp->w_linep = lp;
            wp->w_flag |= WFHARD;       /* Force full. */

out:
            /* Try to use reduced update. Mode line update has its own special
             * flag. The fast update is used if the only thing to do is within
             * the line editing.
             */
            lp = wp->w_linep;
            i = wp->w_toprow;

            if ((wp->w_flag & ~WFMODE) == WFEDIT)
                {
                while (lp != wp->w_dotp)
                    {
                    ++i;
                    lp = lforw(lp);
                    }

                vscreen[i]->v_flag |= VFCHG;
                vtmove(i, 0);

                for (j = 0; j < llength(lp); ++j)
                    vtputc(lgetc(lp, j));

                vteeol();
                }
             else if ((wp->w_flag & (WFEDIT | WFHARD)) != 0)
                {
                while (i < wp->w_toprow+wp->w_ntrows)
                    {
                    vscreen[i]->v_flag |= VFCHG;
                    vtmove(i, 0);

		    /* if line has been changed */
                    if (lp != wp->w_bufp->b_linep)
                        {
                        for (j = 0; j < llength(lp); ++j)
                            vtputc(lgetc(lp, j));

                        lp = lforw(lp);
                        }

                    vteeol();
                    ++i;
                    }
                }
#if ~WFDEBUG
            if ((wp->w_flag&WFMODE) != 0)
                modeline(wp);

            wp->w_flag  = 0;
            wp->w_force = 0;
#endif
            }
#if WFDEBUG
        modeline(wp);
        wp->w_flag =  0;
        wp->w_force = 0;
#endif

	/* and onward to the next window */
        wp = wp->w_wndp;
        }

    /* Always recompute the row and column number of the hardware cursor. This
     * is the only update for simple moves.
     */
    lp = curwp->w_linep;
    currow = curwp->w_toprow;

    while (lp != curwp->w_dotp)
        {
        ++currow;
        lp = lforw(lp);
        }

    curcol = 0;
    i = 0;

    while (i < curwp->w_doto)
        {
        c = lgetc(lp, i++);

        if (c == '\t')
#if TRUETAB
		curcol += tabsize - curcol%tabsize - 1;
#else
            curcol |= 0x07;
#endif
        else if (c < 0x20 || c == 0x7F)
            ++curcol;

        ++curcol;
        }

    if (curcol >= term.t_ncol - 1) {          /* extended line. */
	/* flag we are extended and changed */
	vscreen[currow]->v_flag |= VFEXT | VFCHG;
	updext();				/* and output extended line */
    } else
	lbound = 0;				/* not extended line */

/* make sure no lines need to be de-extended because the cursor is
   no longer on them */

    wp = wheadp;

    while (wp != NULL) {
	lp = wp->w_linep;
	i = wp->w_toprow;

	while (i < wp->w_toprow + wp->w_ntrows) {
		if (vscreen[i]->v_flag & VFEXT) {
			/* always flag extended lines as changed */
			vscreen[i]->v_flag |= VFCHG;
			if ((wp != curwp) || (lp != wp->w_dotp) ||
			   (curcol < term.t_ncol - 1)) {
				vtmove(i, 0);
        	                for (j = 0; j < llength(lp); ++j)
                	            vtputc(lgetc(lp, j));
				vteeol();

				/* this line no longer is extended */
				vscreen[i]->v_flag &= ~VFEXT;
			}
		}
		lp = lforw(lp);
		++i;
	}
	/* and onward to the next window */
        wp = wp->w_wndp;
    }
    /* Special hacking if the screen is garbage. Clear the hardware screen,
     * and update your copy to agree with it. Set all the virtual screen
     * change bits, to force a full update.
     */
    if (sgarbf != FALSE)
        {
        for (i = 0; i < term.t_nrow; ++i)
            {
            vscreen[i]->v_flag |= VFCHG;
            vp1 = pscreen[i];
            for (j = 0; j < term.t_ncol; ++j)
                vp1->v_text[j] = ' ';
            }

        movecursor(0, 0);               /* Erase the screen. */
        (*term.t_eeop)();
        sgarbf = FALSE;                 /* Erase-page clears */
        mpresf = FALSE;                 /* the message area. */
        }

    /* Make sure that the physical and virtual displays agree. Unlike before,
     * the "updateline" code is only called with a line that has been updated
     * for sure.
     */
    for (i = 0; i < term.t_nrow; ++i)
        {
        vp1 = vscreen[i];

	/* for each line that needs to be updated, or that needs its
	   reverse video status changed, call the line updater	*/
	j = vp1->v_flag;
        if (((j & VFCHG) != 0) || (((j & VFREV) == 0) != ((j & VFREQ) == 0)))
            {
#if	TYPEAH
	    if (typahead())
		return(TRUE);
#endif
            vp2 = pscreen[i];
            IGNORE(updateline(i, &vp1->v_text[0], &vp2->v_text[0], &vp1->v_flag));
            }
        }

    /* Finally, update the hardware cursor and flush out buffers. */

    movecursor(currow, curcol - lbound);
    (*term.t_flush)();
    return TRUE;
}

/*	updext: update the extended line which the cursor is currently
		on at a column greater than the terminal width. The line
		will be scrolled right or left to let the user see where
		the cursor is
								*/

int updext()

{
	register int rcursor;	/* real cursor location */
	register LINE *lp;	/* pointer to current line */
	register int j;		/* index into line */

	/* calculate what column the real cursor will end up in */
	rcursor = ((curcol - term.t_ncol) % term.t_scrsiz) + term.t_margin;
	lbound = curcol - rcursor + 1;

	/* scan through the line outputing characters to the virtual screen */
	/* once we reach the left edge					*/
	vtmove(currow, -lbound);	/* start scanning offscreen */
	lp = curwp->w_dotp;		/* line to output */
	for (j=0; j<llength(lp); ++j)	/* until the end-of-line */
		vtpute(lgetc(lp, j));

	/* truncate the virtual line */
	vteeol();

	/* and put a '$' in column 1 */
	vscreen[currow]->v_text[0] = '$';
    return TRUE;
}

/*
 * Update a single line. This does not know how to use insert or delete
 * character sequences; we are using VT52 functionality. Update the physical
 * row and column variables. It does try an exploit erase to end of line. The
 * RAINBOW version of this routine uses fast video.
 */
int updateline(row, vline, pline, flags)
    char vline[];	/* what we want it to end up as */
    char pline[];	/* what it looks like now       */
    short *flags;	/* and how we want it that way  */
{
#if RAINBOW
    register char *cp1;
    register char *cp2;
    register int nch;

    /* since we don't know how to make the rainbow do this, turn it off */
    flags &= (~VFREV & ~VFREQ);

    cp1 = &vline[0];                    /* Use fast video. */
    cp2 = &pline[0];
    putline(row+1, 1, cp1);
    nch = term.t_ncol;

    do
        {
        *cp2 = *cp1;
        ++cp2;
        ++cp1;
        }
    while (--nch);
    *flags &= ~VFCHG;
#else
	register char *cp1;
	register char *cp2;
	register char *cp3;
	register char *cp4;
	register char *cp5;
	register int nbflag;	/* non-blanks to the right flag? */
	int rev;		/* reverse video flag */
	int req;		/* reverse video request flag */


	/* set up pointers to virtual and physical lines */
	cp1 = &vline[0];
	cp2 = &pline[0];

#if	REVSTA
	/* if we need to change the reverse video status of the
	   current line, we need to re-write the entire line     */
	rev = *flags & VFREV;
	req = *flags & VFREQ;
	if (rev != req) {
		movecursor(row, 0);	/* Go to start of line. */
		(*term.t_rev)(req != FALSE);	/* set rev video if needed */

		/* scan through the line and dump it to the screen and
		   the virtual screen array				*/
		cp3 = &vline[term.t_ncol];
		while (cp1 < cp3) {
			(*term.t_putchar)(*cp1);
			++ttcol;
			*cp2++ = *cp1++;
		}
		(*term.t_rev)(FALSE);		/* turn rev video off */

		/* update the needed flags */
		*flags &= ~VFCHG;
		if (req)
			*flags |= VFREV;
		else
			*flags &= ~VFREV;
		return(TRUE);
	}
#endif

	/* advance past any common chars at the left */
	while (cp1 != &vline[term.t_ncol] && cp1[0] == cp2[0]) {
		++cp1;
		++cp2;
	}

/* This can still happen, even though we only call this routine on changed
 * lines. A hard update is always done when a line splits, a massive
 * change is done, or a buffer is displayed twice. This optimizes out most
 * of the excess updating. A lot of computes are used, but these tend to
 * be hard operations that do a lot of update, so I don't really care.
 */
	/* if both lines are the same, no update needs to be done */
	if (cp1 == &vline[term.t_ncol]) {
		*flags &= ~VFCHG;		/* flag this line is changed */
		return(TRUE);
	}

	/* find out if there is a match on the right */
	nbflag = FALSE;
	cp3 = &vline[term.t_ncol];
	cp4 = &pline[term.t_ncol];

	while (cp3[-1] == cp4[-1]) {
		--cp3;
		--cp4;
		if (cp3[0] != ' ')		/* Note if any nonblank */
			nbflag = TRUE;		/* in right match. */
	}

	cp5 = cp3;

	if (nbflag == FALSE && eolexist == TRUE) {	/* Erase to EOL ? */
		while (cp5!=cp1 && cp5[-1]==' ')
			--cp5;

		if (cp3-cp5 <= 3)		/* Use only if erase is */
			cp5 = cp3;		/* fewer characters. */
	}

	movecursor(row, cp1-&vline[0]);	/* Go to start of line. */

	while (cp1 != cp5) {		/* Ordinary. */
		(*term.t_putchar)(*cp1);
		++ttcol;
		*cp2++ = *cp1++;
	}

	if (cp5 != cp3) {		/* Erase. */
		(*term.t_eeol)();
		while (cp1 != cp3)
			*cp2++ = *cp1++;
	}
	*flags &= ~VFCHG;		/* flag this line is changed */
#endif
	return TRUE;
}

/*
 * Redisplay the mode line for the window pointed to by the "wp". This is the
 * only routine that has any idea of how the modeline is formatted. You can
 * change the modeline format by hacking at this routine. Called by "update"
 * any time there is a dirty window.
 */
int modeline(wp)
    WINDOW *wp;
{
    register char *cp;
    register int c;
    register int n;		/* cursor position count */
    register BUFFER *bp;
    register int i;			/* loop index */
    register int lchar;		/* character to draw line in buffer with */
    register int firstm;		/* is this the first mode? */
    char tline[NLINE];		/* buffer for part of mode line */

    n = wp->w_toprow+wp->w_ntrows;      /* Location. */
    vscreen[n]->v_flag |= VFCHG;        /* Redraw next time. */
    vtmove(n, 0);                       /* Seek to right line. */
    if (wp == curwp)			/* mark the current buffer */
	lchar = '=';
    else
#if	REVSTA & (BENTHAK == 0)
	if (revexist)
		lchar = ' ';
	else
#endif
		lchar = '-';

    vtputc(lchar);
    bp = wp->w_bufp;

    if ((bp->b_flag&BFCHG) != 0)                /* "*" if changed. */
        vtputc('*');
    else
        vtputc(lchar);

    n  = 2;
#if (MLH == 1)
    Strcpy(tline, " MicroEMACS 3.6M1.1 (");		/* Buffer name. */
#else
    Strcpy(tline, " MicroEMACS 3.6B1.1 (");		/* Buffer name. */
#endif /*MLH*/
    /* display the modes */

	firstm = TRUE;
	for (i = 0; i < NUMMODES; i++)	/* add in the mode flags */
		if (wp->w_bufp->b_mode & (1 << i)) {
			if (firstm != TRUE)
				Strcat(tline, " ");
			firstm = FALSE;
			Strcat(tline, modename[i]);
		}
	Strcat(tline,") ");

    cp = &tline[0];
    while ((c = *cp++) != 0)
        {
        vtputc(c);
        ++n;
        }

    vtputc(lchar);
    vtputc(lchar);
    vtputc(' ');
    n += 3;
    cp = &bp->b_bname[0];

    while ((c = *cp++) != 0)
        {
        vtputc(c);
        ++n;
        }

    vtputc(' ');
    vtputc(lchar);
    vtputc(lchar);
    n += 3;

    if (bp->b_fname[0] != 0)            /* File name. */
        {
	vtputc(' ');
	++n;
        cp = "File: ";

        while ((c = *cp++) != 0)
            {
            vtputc(c);
            ++n;
            }

        cp = &bp->b_fname[0];

        while ((c = *cp++) != 0)
            {
            vtputc(c);
            ++n;
            }

        vtputc(' ');
        ++n;
        }

#if WFDEBUG
    vtputc(lchar);
    vtputc((wp->w_flag&WFMODE)!=0  ? 'M' : lchar);
    vtputc((wp->w_flag&WFHARD)!=0  ? 'H' : lchar);
    vtputc((wp->w_flag&WFEDIT)!=0  ? 'E' : lchar);
    vtputc((wp->w_flag&WFMOVE)!=0  ? 'V' : lchar);
    vtputc((wp->w_flag&WFFORCE)!=0 ? 'F' : lchar);
    n += 6;
#endif

    while (n < term.t_ncol)             /* Pad to full width. */
        {
        vtputc(lchar);
        ++n;
        }
    return TRUE;
}

int upmode()	/* update all the mode lines */

{
	register WINDOW *wp;

	wp = wheadp;
	while (wp != NULL) {
		wp->w_flag |= WFMODE;
		wp = wp->w_wndp;
	}
    return TRUE;
}

/*
 * Send a command to the terminal to move the hardware cursor to row "row"
 * and column "col". The row and column arguments are origin 0. Optimize out
 * random calls. Update "ttrow" and "ttcol".
 */
int movecursor(row, col)
    {
    if (row!=ttrow || col!=ttcol)
        {
        ttrow = row;
        ttcol = col;
        (*term.t_move)(row, col);
        }
    return TRUE;
    }

/*
 * Erase the message line. This is a special routine because the message line
 * is not considered to be part of the virtual screen. It always works
 * immediately; the terminal buffer is flushed via a call to the flusher.
 */
int mlerase()
    {
    int i;
    
    movecursor(term.t_nrow, 0);
    if (eolexist == TRUE)
	    (*term.t_eeol)();
    else {
        for (i = 0; i < term.t_ncol - 1; i++)
            (*term.t_putchar)(' ');
        movecursor(term.t_nrow, 1);	/* force the move! */
        movecursor(term.t_nrow, 0);
    }
    (*term.t_flush)();
    mpresf = FALSE;
    return TRUE;
    }

/*
 * Ask a yes or no question in the message line. Return either TRUE, FALSE, or
 * ABORT. The ABORT status is returned if the user bumps out of the question
 * with a ^G. Used any time a confirmation is required.
 */

int mlyesno(prompt)

char *prompt;

{
	char c;			/* input character */
	char buf[NPAT];		/* prompt to user */

	for (;;) {
		/* build and prompt the user */
		Strcpy(buf, prompt);
		Strcat(buf, " [y/n]? ");
		mlwrite(buf);

		/* get the responce */
		c = (*term.t_getchar)();

		if (c == BELL)		/* Bail out! */
			return(ABORT);

		if (c=='y' || c=='Y')
			return(TRUE);

		if (c=='n' || c=='N')
			return(FALSE);
	}
}

/*
 * Write a prompt into the message line, then read back a response. Keep
 * track of the physical position of the cursor. If we are in a keyboard
 * macro throw the prompt away, and return the remembered response. This
 * lets macros run at full speed. The reply is always terminated by a carriage
 * return. Handle erase, kill, and abort keys.
 */

int mlreply(prompt, buf, nbuf)
    char *prompt;
    char *buf;
{
	return(mlreplyt(prompt,buf,nbuf,'\n'));
}

/*	A more generalized prompt/reply function allowing the caller
	to specify the proper terminator. If the terminator is not
	a return ('\n') it will echo as "<NL>"
							*/
int mlreplyt(prompt, buf, nbuf, eolchar)

char *prompt;
char *buf;
char eolchar;

{
	register int cpos;
	register int i;
	register int c;

	cpos = 0;

	if (kbdmop != NULL) {
		while ((c = *kbdmop++) != '\0')
			buf[cpos++] = c;

		buf[cpos] = 0;

		if (buf[0] == 0)
			return(FALSE);

		return(TRUE);
	}

	/* check to see if we are executing a command line */
	if (clexec) {
		IGNORE(nxtarg(buf));
		return(TRUE);
	}

	mlwrite(prompt);

	for (;;) {
	/* get a character from the user. if it is a <ret>, change it
	   to a <NL>							*/
		c = (*term.t_getchar)();
		if (c == 0x0d)
			c = '\n';

		if (c == eolchar) {
			buf[cpos++] = 0;

			if (kbdmip != NULL) {
				if (kbdmip+cpos > &kbdm[NKBDM-3]) {
					IGNORE(ctrlg(FALSE, 0));
					(*term.t_flush)();
					return(ABORT);
				}

				for (i=0; i<cpos; ++i)
					*kbdmip++ = buf[i];
				}

#if _Windows
					(*term.t_move)(ttrow, 0);
#else
				(*term.t_putchar)('\r');
#endif
				ttcol = 0;
				(*term.t_flush)();

				if (buf[0] == 0)
					return(FALSE);

				return(TRUE);

			} else if (c == 0x07) {	/* Bell, abort */
				(*term.t_putchar)('^');
				(*term.t_putchar)('G');
				ttcol += 2;
				IGNORE(ctrlg(FALSE, 0));
				(*term.t_flush)();
				return(ABORT);

			} else if (c == 0x7F || c == 0x08) {	/* rubout/erase */
				if (cpos != 0) {
#if _Windows
					(*term.t_move)(ttrow, ttcol-1);
					(*term.t_putchar)(' ');
					(*term.t_move)(ttrow, ttcol-1);
#else
					(*term.t_putchar)('\b');
					(*term.t_putchar)(' ');
					(*term.t_putchar)('\b');
#endif
					--ttcol;

					if (buf[--cpos] < 0x20) {
#if _Windows
					(*term.t_move)(ttrow, ttcol-1);
					(*term.t_putchar)(' ');
					(*term.t_move)(ttrow, ttcol-1);
#else
						(*term.t_putchar)('\b');
						(*term.t_putchar)(' ');
						(*term.t_putchar)('\b');
#endif
						--ttcol;
					}

					if (buf[cpos] == '\n') {
#if _Windows
					(*term.t_move)(ttrow, ttcol-2);
					(*term.t_putchar)(' ');
					(*term.t_putchar)(' ');
					(*term.t_move)(ttrow, ttcol-2);
#else
						(*term.t_putchar)('\b');
						(*term.t_putchar)('\b');
						(*term.t_putchar)(' ');
						(*term.t_putchar)(' ');
						(*term.t_putchar)('\b');
						(*term.t_putchar)('\b');
#endif
						--ttcol;
						--ttcol;
					}

					(*term.t_flush)();
				}

			} else if (c == 0x15) {	/* C-U, kill */
				while (cpos != 0) {
#if _Windows
					(*term.t_move)(ttrow, ttcol-1);
					(*term.t_putchar)(' ');
					(*term.t_move)(ttrow, ttcol-1);
#else
					(*term.t_putchar)('\b');
					(*term.t_putchar)(' ');
					(*term.t_putchar)('\b');
#endif
					--ttcol;

					if (buf[--cpos] < 0x20) {
#if _Windows
					(*term.t_move)(ttrow, ttcol-1);
					(*term.t_putchar)(' ');
					(*term.t_move)(ttrow, ttcol-1);
#else
						(*term.t_putchar)('\b');
						(*term.t_putchar)(' ');
						(*term.t_putchar)('\b');
#endif
						--ttcol;
					}
				}

				(*term.t_flush)();

			} else {
				if (cpos < nbuf-1) {
					buf[cpos++] = c;

					if ((c < ' ') && (c != '\n')) {
						(*term.t_putchar)('^');
						++ttcol;
						c ^= 0x40;
					}

					if (c != '\n')
						(*term.t_putchar)(c);
					else {	/* put out <NL> for <ret> */
						(*term.t_putchar)('<');
						(*term.t_putchar)('N');
						(*term.t_putchar)('L');
						(*term.t_putchar)('>');
						ttcol += 3;
					}
				++ttcol;
				(*term.t_flush)();
			}
		}
	}
    return TRUE;
}

/*
 * Write a message into the message line. Keep track of the physical cursor
 * position. A small class of printf like format items is handled. Assumes the
 * stack grows down; this assumption is made by the "++" in the argument scan
 * loop. Set the "message line" flag TRUE.
 */

/*VARARGS1*/
int mlwrite(fmt, arg)
    char *fmt;
    {
    register int c;
    register char *ap;

    if (eolexist == FALSE) {
        mlerase();
        (*term.t_flush)();
    }

    movecursor(term.t_nrow, 0);
    ap = (char *) &arg;
    while ((c = *fmt++) != 0) {
        if (c != '%') {
            (*term.t_putchar)(c);
            ++ttcol;
            }
        else
            {
            c = *fmt++;
            switch (c) {
                case 'd':
                    mlputi(*(int *)ap, 10);
                    ap += sizeof(int);
                    break;

                case 'o':
                    mlputi(*(int *)ap,  8);
                    ap += sizeof(int);
                    break;

                case 'x':
                    mlputi(*(int *)ap, 16);
                    ap += sizeof(int);
                    break;

                case 'D':
                    mlputli(*(long *)ap, 10);
                    ap += sizeof(long);
                    break;

                case 's':
                    mlputs(*(char **)ap);
                    ap += sizeof(char *);
                    break;

                default:
                    (*term.t_putchar)(c);
                    ++ttcol;
                }
            }
        }
    if (eolexist == TRUE)
        (*term.t_eeol)();
    (*term.t_flush)();
    mpresf = TRUE;
    return TRUE;
    }

/*
 * Write out a string. Update the physical cursor position. This assumes that
 * the characters in the string all have width "1"; if this is not the case
 * things will get screwed up a little.
 */
int mlputs(s)
    char *s;
    {
    register int c;

    while ((c = *s++) != 0)
        {
        (*term.t_putchar)(c);
        ++ttcol;
        }
    return TRUE;
    }

/*
 * Write out an integer, in the specified radix. Update the physical cursor
 * position. This will not handle any negative numbers; maybe it should.
 */
int mlputi(i, r)
    {
    register int q;
    static char hexdigits[] = "0123456789ABCDEF";

    if (i < 0)
        {
        i = -i;
        (*term.t_putchar)('-');
        }

    q = i/r;

    if (q != 0)
        mlputi(q, r);

    (*term.t_putchar)(hexdigits[i%r]);
    ++ttcol;
    return TRUE;
    }

/*
 * do the same except as a long integer.
 */
int mlputli(l, r)
    long l;
    {
    register long q;

    if (l < 0)
        {
        l = -l;
        (*term.t_putchar)('-');
        }

    q = l/r;

    if (q != 0)
        mlputli(q, r);

    (*term.t_putchar)((int)(l%r)+'0');
    ++ttcol;
    return TRUE;
    }

#if RAINBOW

int putline(row, col, buf)
    int row, col;
    char buf[];
    {
    int n;

    n = strlen(buf);
    if (col + n - 1 > term.t_ncol)
        n = term.t_ncol - col + 1;
    Put_Data(row, col, n, buf);
    return TRUE;
    }
#endif

/* get a command name from the command line. Command completion means
   that pressing a <SPACE> will attempt to complete an unfinished command
   name if it is unique.
*/

int (*getname())()

{
	register int cpos;	/* current column on screen output */
	register int c;
	register char *sp;	/* pointer to string for output */
	register NBIND *ffp;	/* first ptr to entry in name binding table */
	register NBIND *cffp;	/* current ptr to entry in name binding table */
	register NBIND *lffp;	/* last ptr to entry in name binding table */
	char buf[NSTRING];	/* buffer to hold tentative command name */
	int (*fncmatch())();

	/* starting at the begining of the string buffer */
	cpos = 0;

	/* if we are executing a keyboard macro, fill our buffer from there,
	   and attempt a straight match */
	if (kbdmop != NULL) {
		while ((c = *kbdmop++) != '\0')
			buf[cpos++] = c;

		buf[cpos] = 0;

		/* return the result of a match */
		return(fncmatch(&buf[0]));
	}

	/* if we are executing a command line get the next arg and match it */
	if (clexec) {
		IGNORE(nxtarg(buf));
		return(fncmatch(&buf[0]));
	}

	/* build a name string from the keyboard */
	while (TRUE) {
		c = (*term.t_getchar)();

		/* if we are at the end, just match it */
		if (c == 0x0d) {
			buf[cpos] = 0;

			/* save keyboard macro string if needed */
			if (kbdtext(&buf[0]) == ABORT)
				return( (int (*)()) NULL);

			/* and match it off */
			return(fncmatch(&buf[0]));

		} else if (c == 0x07) {	/* Bell, abort */
			(*term.t_putchar)('^');
			(*term.t_putchar)('G');
			ttcol += 2;
			IGNORE(ctrlg(FALSE, 0));
			(*term.t_flush)();
			return( (int (*)()) NULL);

		} else if (c == 0x7F || c == 0x08) {	/* rubout/erase */
			if (cpos != 0) {
				(*term.t_putchar)('\b');
				(*term.t_putchar)(' ');
				(*term.t_putchar)('\b');
				--ttcol;
				--cpos;
				(*term.t_flush)();
			}

		} else if (c == 0x15) {	/* C-U, kill */
			while (cpos != 0) {
				(*term.t_putchar)('\b');
				(*term.t_putchar)(' ');
				(*term.t_putchar)('\b');
				--cpos;
				--ttcol;
			}

			(*term.t_flush)();

		} else if (c == ' ') {
/* <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
	/* attempt a completion */
	buf[cpos] = 0;		/* terminate it for us */
	ffp = &names[0];	/* scan for matches */
	while (ffp->n_func != NULL) {
		if (strncmp(buf, ffp->n_name, strlen(buf)) == 0) {
			/* a possible match! More than one? */
			if ((ffp + 1)->n_func == NULL ||
			   (strncmp(buf, (ffp+1)->n_name, strlen(buf)) != 0)) {
				/* no...we match, print it */
				sp = ffp->n_name + cpos;
				while (*sp)
					(*term.t_putchar)(*sp++);
				(*term.t_flush)();
				return(ffp->n_func);
			} else {
/* << << << << << << << << << << << << << << << << << */
	/* try for a partial match against the list */

	/* first scan down until we no longer match the current input */
	lffp = (ffp + 1);
	while ((lffp+1)->n_func != NULL) {
		if (strncmp(buf, (lffp+1)->n_name, strlen(buf)) != 0)
			break;
		++lffp;
	}

	/* and now, attempt to partial complete the string, char at a time */
	while (TRUE) {
		/* add the next char in */
		buf[cpos] = ffp->n_name[cpos];

		/* scan through the candidates */
		cffp = ffp + 1;
		while (cffp <= lffp) {
			if (cffp->n_name[cpos] != buf[cpos])
				goto onward;
			++cffp;
		}

		/* add the character */
		(*term.t_putchar)(buf[cpos++]);
	}
/* << << << << << << << << << << << << << << << << << */
			}
		}
		++ffp;
	}

	/* no match.....beep and onward */
	(*term.t_beep)();
onward:;
	(*term.t_flush)();
/* <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
		} else {
			if (cpos < NSTRING-1 && c > ' ') {
				buf[cpos++] = c;
				(*term.t_putchar)(c);
			}

			++ttcol;
			(*term.t_flush)();
		}
	}
}

int kbdtext(buf)	/* add this text string to the current keyboard macro
		   definition						*/

char *buf;	/* text to add to keyboard macro */

{
	/* if we are defining a keyboard macro, save it */
	if (kbdmip != NULL) {
		if (kbdmip+strlen(buf) > &kbdm[NKBDM-4]) {
			IGNORE(ctrlg(FALSE, 0));
			(*term.t_flush)();
			return(ABORT);
		}

		/* copy string in and null terminate it */
		while (*buf)
			*kbdmip++ = *buf++;
		*kbdmip++ = 0;
	}
	return(TRUE);
}
