#include <../../nrnconf.h>
/* /local/src/master/nrn/src/memacs/buffer.c,v 1.2 1994/10/25 17:21:28 hines Exp */
/*
buffer.c,v
 * Revision 1.2  1994/10/25  17:21:28  hines
 * bsearch and atoi conflict with names in djg libraries
 *
 * Revision 1.1.1.1  1994/10/12  17:21:24  hines
 * NEURON 3.0 distribution
 *
 * Revision 1.4  91/08/22  12:33:06  hines
 * works with turboc
 * 
 * Revision 1.3  89/07/10  10:25:14  mlh
 * LINT free
 * 
 * Revision 1.2  89/07/09  12:18:30  mlh
 * lint pass1 now ok
 * 
 * Revision 1.1  89/07/08  15:36:11  mlh
 * Initial revision
 * 
*/

/*
 * Buffer management.
 * Some of the functions are internal,
 * and some are actually attached to user
 * keys. Like everyone else, they set hints
 * for the display system.
 */
#include        <stdio.h>
#include	"estruct.h"
#include        "edef.h"

/*
 * Attach a buffer to a window. The
 * values of dot and mark come from the buffer
 * if the use count is 0. Otherwise, they come
 * from some other window.
 */
int usebuffer(f, n)
{
        register BUFFER *bp;
        register int    s;
        char            bufn[NBUFN];

	LINTUSE(f) LINTUSE(n)
        if ((s=mlreply("Use buffer: ", bufn, NBUFN)) != TRUE)
                return (s);
        if ((bp=bfind(bufn, TRUE, 0)) == NULL)
                return (FALSE);
	return(swbuffer(bp));
}

int nextbuffer(f, n)	/* switch to the next buffer in the buffer list */

{
        register BUFFER *bp;

	LINTUSE(f) LINTUSE(n)
	bp = curbp->b_bufp;
	/* cycle through the buffers to find an eligable one */
	while (bp == NULL || bp->b_flag & BFTEMP) {
		if (bp == NULL)
			bp = bheadp;
		else
			bp = bp->b_bufp;
	}
	return(swbuffer(bp));
}

int swbuffer(bp)	/* make buffer BP current */

BUFFER *bp;

{
        register WINDOW *wp;

        if (--curbp->b_nwnd == 0) {             /* Last use.            */
                curbp->b_dotp  = curwp->w_dotp;
                curbp->b_doto  = curwp->w_doto;
                curbp->b_markp = curwp->w_markp;
                curbp->b_marko = curwp->w_marko;
        }
        curbp = bp;                             /* Switch.              */
	if (curbp->b_active != TRUE) {		/* buffer not active yet*/
		/* read it in and activate it */
		IGNORE(readin(curbp->b_fname, TRUE));
		curbp->b_dotp = lforw(curbp->b_linep);
		curbp->b_doto = 0;
		curbp->b_active = TRUE;
	}
        curwp->w_bufp  = bp;
        curwp->w_linep = bp->b_linep;           /* For macros, ignored. */
        curwp->w_flag |= WFMODE|WFFORCE|WFHARD; /* Quite nasty.         */
        if (bp->b_nwnd++ == 0) {                /* First use.           */
                curwp->w_dotp  = bp->b_dotp;
                curwp->w_doto  = bp->b_doto;
                curwp->w_markp = bp->b_markp;
                curwp->w_marko = bp->b_marko;
                return (TRUE);
        }
        wp = wheadp;                            /* Look for old.        */
        while (wp != NULL) {
                if (wp!=curwp && wp->w_bufp==bp) {
                        curwp->w_dotp  = wp->w_dotp;
                        curwp->w_doto  = wp->w_doto;
                        curwp->w_markp = wp->w_markp;
                        curwp->w_marko = wp->w_marko;
                        break;
                }
                wp = wp->w_wndp;
        }
        return (TRUE);
}

/*
 * Dispose of a buffer, by name.
 * Ask for the name. Look it up (don't get too
 * upset if it isn't there at all!). Get quite upset
 * if the buffer is being displayed. Clear the buffer (ask
 * if the buffer has been changed). Then free the header
 * line and the buffer header. Bound to "C-X K".
 */
int killbuffer(f, n)

{
	register BUFFER *bp;
        register int    s;
        char bufn[NBUFN];

	LINTUSE(f) LINTUSE(n)
        if ((s=mlreply("Kill buffer: ", bufn, NBUFN)) != TRUE)
                return(s);
        if ((bp=bfind(bufn, FALSE, 0)) == NULL) /* Easy if unknown.     */
                return (TRUE);
	return(zotbuf(bp));
}

int zotbuf(bp)	/* kill the buffer pointed to by bp */

register BUFFER *bp;

{
        register BUFFER *bp1;
        register BUFFER *bp2;
        register int    s;

        if (bp->b_nwnd != 0) {                  /* Error if on screen.  */
                mlwrite("Buffer is being displayed");
                return (FALSE);
        }
        if ((s=bclear(bp)) != TRUE)             /* Blow text away.      */
                return (s);
        free((char *) bp->b_linep);             /* Release header line. */
        bp1 = NULL;                             /* Find the header.     */
        bp2 = bheadp;
        while (bp2 != bp) {
                bp1 = bp2;
                bp2 = bp2->b_bufp;
        }
        bp2 = bp2->b_bufp;                      /* Next one in chain.   */
        if (bp1 == NULL)                        /* Unlink it.           */
                bheadp = bp2;
        else
                bp1->b_bufp = bp2;
        free((char *) bp);                      /* Release buffer block */
        return (TRUE);
}

int namebuffer(f,n)		/*	Rename the current buffer	*/

int f, n;		/* default Flag & Numeric arg */

{
	register BUFFER *bp;	/* pointer to scan through all buffers */
	char bufn[NBUFN];	/* buffer to hold buffer name */

	LINTUSE(f) LINTUSE(n)
	/* prompt for and get the new buffer name */
ask:	if (mlreply("Change buffer name to: ", bufn, NBUFN) != TRUE)
		return(FALSE);

	/* and check for duplicates */
	bp = bheadp;
	while (bp != NULL) {
		if (bp != curbp) {
			/* if the names the same */
			if (strcmp(bufn, bp->b_bname) == 0)
				goto ask;  /* try again */
		}
		bp = bp->b_bufp;	/* onward */
	}

	Strcpy(curbp->b_bname, bufn);	/* copy buffer name to structure */
	curwp->w_flag |= WFMODE;	/* make mode line replot */
	mlerase();
	return TRUE;
}

/*
 * List all of the active
 * buffers. First update the special
 * buffer that holds the list. Next make
 * sure at least 1 window is displaying the
 * buffer list, splitting the screen if this
 * is what it takes. Lastly, repaint all of
 * the windows that are displaying the
 * list. Bound to "C-X C-B".
 */
int listbuffers(f, n)
{
        register WINDOW *wp;
        register BUFFER *bp;
        register int    s;

	LINTUSE(f) LINTUSE(n)
        if ((s=makelist()) != TRUE)
                return (s);
        if (blistp->b_nwnd == 0) {              /* Not on screen yet.   */
                if ((wp=wpopup()) == NULL)
                        return (FALSE);
                bp = wp->w_bufp;
                if (--bp->b_nwnd == 0) {
                        bp->b_dotp  = wp->w_dotp;
                        bp->b_doto  = wp->w_doto;
                        bp->b_markp = wp->w_markp;
                        bp->b_marko = wp->w_marko;
                }
                wp->w_bufp  = blistp;
                ++blistp->b_nwnd;
        }
        wp = wheadp;
        while (wp != NULL) {
                if (wp->w_bufp == blistp) {
                        wp->w_linep = lforw(blistp->b_linep);
                        wp->w_dotp  = lforw(blistp->b_linep);
                        wp->w_doto  = 0;
                        wp->w_markp = NULL;
                        wp->w_marko = 0;
                        wp->w_flag |= WFMODE|WFHARD;
                }
                wp = wp->w_wndp;
        }
        return (TRUE);
}

/*
 * This routine rebuilds the
 * text in the special secret buffer
 * that holds the buffer list. It is called
 * by the list buffers command. Return TRUE
 * if everything works. Return FALSE if there
 * is an error (if there is no memory).
 */
int makelist()
{
        register char   *cp1;
        register char   *cp2;
        register int    c;
        register BUFFER *bp;
        register LINE   *lp;
        register int    nbytes;
        register int    s;
	register int	i;
        char            b[6+1];
        char            line[128];

        blistp->b_flag &= ~BFCHG;               /* Don't complain!      */
        if ((s=bclear(blistp)) != TRUE)         /* Blow old text away   */
                return (s);
        Strcpy(blistp->b_fname, "");
        if (addline("AC MODES    Size Buffer         File") == FALSE
        ||  addline("-- -----    ---- ------         ----") == FALSE)
                return (FALSE);
        bp = bheadp;                            /* For all buffers      */

	/* build line to report global mode settings */
	cp1 = &line[0];
	*cp1++ = ' ';
	*cp1++ = ' ';
	*cp1++ = ' ';

	/* output the mode codes */
	for (i = 0; i < NUMMODES; i++)
		if (gmode & (1 << i))
			*cp1++ = modecode[i];
		else
			*cp1++ = '.';
	Strcpy(cp1, "        Global Modes");
	if (addline(line) == FALSE)
		return(FALSE);

	/* output the list of buffers */
        while (bp != NULL) {
                if ((bp->b_flag&BFTEMP) != 0) { /* Skip magic ones.     */
                        bp = bp->b_bufp;
                        continue;
                }
                cp1 = &line[0];                 /* Start at left edge   */

		/* output status of ACTIVE flag (has the file been read in? */
                if (bp->b_active == TRUE)    /* "@" if activated       */
                        *cp1++ = '@';
                else
                        *cp1++ = ' ';

		/* output status of changed flag */
                if ((bp->b_flag&BFCHG) != 0)    /* "*" if changed       */
                        *cp1++ = '*';
                else
                        *cp1++ = ' ';
                *cp1++ = ' ';                   /* Gap.                 */

		/* output the mode codes */
		for (i = 0; i < NUMMODES; i++) {
			if (bp->b_mode & (1 << i))
				*cp1++ = modecode[i];
			else
				*cp1++ = '.';
		}
                *cp1++ = ' ';                   /* Gap.                 */
                nbytes = 0;                     /* Count bytes in buf.  */
                lp = lforw(bp->b_linep);
                while (lp != bp->b_linep) {
                        nbytes += llength(lp)+1;
                        lp = lforw(lp);
                }
                emacs_itoa(b, 6, nbytes);             /* 6 digit buffer size. */
                cp2 = &b[0];
                while ((c = *cp2++) != 0)
                        *cp1++ = c;
                *cp1++ = ' ';                   /* Gap.                 */
                cp2 = &bp->b_bname[0];          /* Buffer name          */
                while ((c = *cp2++) != 0)
                        *cp1++ = c;
                cp2 = &bp->b_fname[0];          /* File name            */
                if (*cp2 != 0) {
                        while (cp1 < &line[2+1+5+1+6+1+NBUFN])
                                *cp1++ = ' ';
                        while ((c = *cp2++) != 0) {
                                if (cp1 < &line[128-1])
                                        *cp1++ = c;
                        }
                }
                *cp1 = 0;                       /* Add to the buffer.   */
                if (addline(line) == FALSE)
                        return (FALSE);
                bp = bp->b_bufp;
        }
        return (TRUE);                          /* All done             */
}

int emacs_itoa(buf, width, num)
register char   buf[];
register int    width;
register int    num;
{
        buf[width] = 0;                         /* End of string.       */
        while (num >= 10) {                     /* Conditional digits.  */
                buf[--width] = (num%10) + '0';
                num /= 10;
        }
        buf[--width] = num + '0';               /* Always 1 digit.      */
        while (width != 0)                      /* Pad with blanks.     */
                buf[--width] = ' ';
	return TRUE;
}

/*
 * The argument "text" points to
 * a string. Append this line to the
 * buffer list buffer. Handcraft the EOL
 * on the end. Return TRUE if it worked and
 * FALSE if you ran out of room.
 */
int addline(text)
char    *text;
{
        register LINE   *lp;
        register int    i;
        register int    ntext;

        ntext = strlen(text);
        if ((lp=lalloc(ntext)) == NULL)
                return (FALSE);
        for (i=0; i<ntext; ++i)
                lputc(lp, i, text[i]);
        blistp->b_linep->l_bp->l_fp = lp;       /* Hook onto the end    */
        lp->l_bp = blistp->b_linep->l_bp;
        blistp->b_linep->l_bp = lp;
        lp->l_fp = blistp->b_linep;
        if (blistp->b_dotp == blistp->b_linep)  /* If "." is at the end */
                blistp->b_dotp = lp;            /* move it to new line  */
        return (TRUE);
}

/*
 * Look through the list of
 * buffers. Return TRUE if there
 * are any changed buffers. Buffers
 * that hold magic internal stuff are
 * not considered; who cares if the
 * list of buffer names is hacked.
 * Return FALSE if no buffers
 * have been changed.
 */
int anycb()
{
        register BUFFER *bp;

        bp = bheadp;
        while (bp != NULL) {
                if ((bp->b_flag&BFTEMP)==0 && (bp->b_flag&BFCHG)!=0)
                        return (TRUE);
                bp = bp->b_bufp;
        }
        return (FALSE);
}

#if	MLH
/* buffer names have a prefix of the form "(nnn) " which is the buffer
 * number.  One can identify a buffer from either its number or name
 */
static int nextnum;

static int bufname_match(pattern, string)
	char *pattern, *string;
{
	char *bufnum, *bufname, *cp, buf[NBUFN];

	strncpy(buf, string, NBUFN);
	buf[NBUFN-1] = '\0';
	for (cp=buf; cp < &buf[NBUFN]; cp++){
		if (*cp == ')') break;
	}
	if (cp >= &buf[NBUFN]) { /*old style-- buffer name has no number*/
		bufnum = buf;
		bufname = buf;
	} else {
		*cp = '\0';	/* every name should have a prefix */
		bufname = cp + 2;
		bufnum = buf+1;
	}
	return (strcmp(pattern, bufnum) == 0 || strcmp(pattern, bufname) == 0);
}
#endif	/*MLH*/

/*
 * Find a buffer, by name. Return a pointer
 * to the BUFFER structure associated with it. If
 * the named buffer is found, but is a TEMP buffer (like
 * the buffer list) conplain. If the buffer is not found
 * and the "cflag" is TRUE, create it. The "bflag" is
 * the settings for the flags in in buffer.
 */
BUFFER  *bfind(bname, cflag, bflag)
register char   *bname;
{
        register BUFFER *bp;
	register BUFFER *sb;	/* buffer to insert after */
        register LINE   *lp;

        bp = bheadp;
        while (bp != NULL) {
#if	MLH
		if (bufname_match(bname, bp->b_bname)) {
#else
		if (strcmp(bname, bp->b_bname) == 0) {
#endif	/*MLH*/
                        if ((bp->b_flag&BFTEMP) != 0) {
                                mlwrite("Cannot select builtin buffer");
                                return (NULL);
                        }
                        return (bp);
                }
                bp = bp->b_bufp;
        }
        if (cflag != FALSE) {
                if ((bp=(BUFFER *)malloc(sizeof(BUFFER))) == NULL)
                        return (NULL);
                if ((lp=lalloc(0)) == NULL) {
                        free((char *) bp);
                        return (NULL);
                }
		/* find the place in the list to insert this buffer */
		if (bheadp == NULL || strcmp(bheadp->b_bname, bname) > 0) {
			/* insert at the begining */
	                bp->b_bufp = bheadp;
        	        bheadp = bp;
        	} else {
			sb = bheadp;
			while (sb->b_bufp != NULL) {
				if (strcmp(sb->b_bufp->b_bname, bname) > 0)
					break;
				sb = sb->b_bufp;
			}

			/* and insert it */
       			bp->b_bufp = sb->b_bufp;
        		sb->b_bufp = bp;
       		}

		/* and set up the other buffer fields */
		bp->b_active = TRUE;
                bp->b_dotp  = lp;
                bp->b_doto  = 0;
                bp->b_markp = NULL;
                bp->b_marko = 0;
                bp->b_flag  = bflag;
		bp->b_mode  = gmode;
                bp->b_nwnd  = 0;
                bp->b_linep = lp;
                Strcpy(bp->b_fname, "");
#if	MLH
		{char name[NBUFN + 10];		
		Sprintf(name, "(%d) %s", nextnum++, bname);
		Strncpy(bp->b_bname, name,NBUFN);
		bp->b_bname[NBUFN-1] = '\0';
		}
#else
		Strcpy(bp->b_bname, bname);
#endif	/*MLH*/
                lp->l_fp = lp;
                lp->l_bp = lp;
        }
        return (bp);
}

/*
 * This routine blows away all of the text
 * in a buffer. If the buffer is marked as changed
 * then we ask if it is ok to blow it away; this is
 * to save the user the grief of losing text. The
 * window chain is nearly always wrong if this gets
 * called; the caller must arrange for the updates
 * that are required. Return TRUE if everything
 * looks good.
 */
int bclear(bp)
register BUFFER *bp;
{
        register LINE   *lp;
        register int    s;

        if ((bp->b_flag&BFTEMP) == 0            /* Not scratch buffer.  */
        && (bp->b_flag&BFCHG) != 0              /* Something changed    */
        && (s=mlyesno("Discard changes")) != TRUE)
                return (s);
        bp->b_flag  &= ~BFCHG;                  /* Not changed          */
        while ((lp=lforw(bp->b_linep)) != bp->b_linep)
                lfree(lp);
        bp->b_dotp  = bp->b_linep;              /* Fix "."              */
        bp->b_doto  = 0;
        bp->b_markp = NULL;                     /* Invalidate "mark"    */
        bp->b_marko = 0;
        return (TRUE);
}
