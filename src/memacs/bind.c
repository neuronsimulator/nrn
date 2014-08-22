#include <../../nrnconf.h>
/* /local/src/master/nrn/src/memacs/bind.c,v 1.1.1.1 1994/10/12 17:21:23 hines Exp */
/*
bind.c,v
 * Revision 1.1.1.1  1994/10/12  17:21:23  hines
 * NEURON 3.0 distribution
 *
 * Revision 1.3  89/07/10  10:25:05  mlh
 * LINT free
 * 
 * Revision 1.2  89/07/09  12:18:23  mlh
 * lint pass1 now ok
 * 
 * Revision 1.1  89/07/08  15:36:06  mlh
 * Initial revision
 * 
*/

/*	This file is for functions having to do with key bindings,
	descriptions, help commands, and command line execution.

	written 11-feb-86 by Daniel Lawrence
								*/

#include	<stdio.h>
#include	"estruct.h"
#include	"edef.h"
#include	"epath.h"

int deskey(f, n)	/* describe the command for a certain key */
{
	register int c;		/* command character to describe */
	register char *ptr;	/* string pointer to scan output strings */
	register KEYTAB *ktp;	/* pointer into the command table */
	register int found;	/* matched command flag */
	register NBIND *nptr;	/* pointer into the name binding table */
	char outseq[80];	/* output buffer for command sequence */

	LINTUSE(f) LINTUSE(n)
	/* prompt the user to type us a key to describe */
	mlwrite(": describe-key ");

	/* get the command sequence to describe */
	c = getckey();			/* get a command sequence */

	/* change it to something we can print as well */
	cmdstr(c, &outseq[0]);

	/* and dump it out */
	ptr = &outseq[0];
	while (*ptr)
		(*term.t_putchar)(*ptr++);
	(*term.t_putchar)(' ');		/* space it out */

	/* find the right ->function */
	ktp = &keytab[0];
	found = FALSE;
	while (ktp->k_fp != NULL) {
		if (ktp->k_code == c) {
			found = TRUE;
			break;
		}
		++ktp;
	}

	if (!found)
		Strcpy(outseq,"Not Bound");
	else {
		/* match it against the name binding table */
		nptr = &names[0];
		Strcpy(outseq,"[Bad binding]");
		while (nptr->n_func != NULL) {
			if (nptr->n_func == ktp->k_fp) {
				Strcpy(outseq, nptr->n_name);
				break;
			}
			++nptr;
		}
	}

	/* output the command sequence */
	ptr = &outseq[0];
	while (*ptr)
		(*term.t_putchar)(*ptr++);
	return TRUE;
}

int cmdstr(c, seq)	/* change a key command to a string we can print out */

int c;		/* sequence to translate */
char *seq;	/* destination string for sequence */

{
	char *ptr;	/* pointer into current position in sequence */

	ptr = seq;

	/* apply meta sequence if needed */
	if (c & META) {
		*ptr++ = 'M';
		*ptr++ = '-';
	}

	/* apply ^X sequence if needed */
	if (c & CTLX) {
		*ptr++ = '^';
		*ptr++ = 'X';
	}

	/* apply SPEC sequence if needed */
	if (c & SPEC) {
		*ptr++ = 'F';
		*ptr++ = 'N';
	}

	/* apply control sequence if needed */
	if (c & CTRL) {
		*ptr++ = '^';
	}

	c = c & 255;	/* strip the prefixes */

	/* and output the final sequence */

	*ptr++ = c;
	*ptr = 0;	/* terminate the string */
	return TRUE;
}

int help(f, n)	/* give me some help!!!!
		   bring up a fake buffer and read the help file
		   into it with view mode			*/
{
	register int status;	/* status of I/O operations */
	register WINDOW *wp;	/* scnaning pointer to windows */
	register int i;		/* index into help file names */
	char fname[NSTRING];	/* buffer to construct file name in */

	LINTUSE(f) LINTUSE(n)
	/* search through the list of help files */
	for (i=2; i < NPNAMES; i++) {
		Strcpy(fname, pathname[i]);
		Strcat(fname, pathname[1]);
		status = ffropen(fname);
		if (status == FIOSUC)
			break;
	}

	if (status == FIOFNF) {
		mlwrite("[Help file is not online]");
		return(FALSE);
	}
	IGNORE(ffclose());	/* close the file to prepare for to read it in */

	/* split the current window to make room for the help stuff */
	if (splitwind(FALSE, 1) == FALSE)
			return(FALSE);

	/* and read the stuff in */
	if (getfile(fname, FALSE) == FALSE)
		return(FALSE);

	/* make this window in VIEW mode, update all mode lines */
	curwp->w_bufp->b_mode |= MDVIEW;
	wp = wheadp;
	while (wp != NULL) {
		wp->w_flag |= WFMODE;
		wp = wp->w_wndp;
	}
	return(TRUE);
}

int (*fncmatch(fname))() /* match fname to a function in the names table
			    and return any match or NULL if none		*/

char *fname;	/* name to attempt to match */

{
	register NBIND *ffp;	/* pointer to entry in name binding table */

	/* scan through the table, returning any match */
	ffp = &names[0];
	while (ffp->n_func != NULL) {
		if (strcmp(fname, ffp->n_name) == 0)
			return(ffp->n_func);
		++ffp;
	}
	return(NULL);
}

/* bindtokey:	add a new key to the key binding table
*/

int bindtokey(f, n)

int f, n;	/* command arguments [IGNORED] */

{
	register int c;		/* command key to bind */
	register int (*kfunc)();	/* ptr to the requexted function to bind to */
	register char *ptr;	/* ptr to dump out input key string */
	register KEYTAB *ktp;	/* pointer into the command table */
	register int found;	/* matched command flag */
	char outseq[80];	/* output buffer for keystroke sequence */
	int (*getname())();

	LINTUSE(f) LINTUSE(n)
	/* prompt the user to type in a key to bind */
	mlwrite(": bind-to-key ");

	/* get the function name to bind it to */
	kfunc = getname();
	if (kfunc == NULL) {
		mlwrite("[No such function]");
		return(FALSE);
	}
	(*term.t_putchar)(' ');		/* space it out */
	(*term.t_flush)();

	/* get the command sequence to bind */
	c = getckey();			/* get a command sequence */

	/* change it to something we can print as well */
	cmdstr(c, &outseq[0]);

	/* and dump it out */
	ptr = &outseq[0];
	while (*ptr)
		(*term.t_putchar)(*ptr++);

	/* search the table to see if it exists */
	ktp = &keytab[0];
	found = FALSE;
	while (ktp->k_fp != NULL) {
		if (ktp->k_code == c) {
			found = TRUE;
			break;
		}
		++ktp;
	}

	if (found) {	/* it exists, just change it then */
		ktp->k_fp = kfunc;
	} else {	/* otherwise we need to add it to the end */
		/* if we run out of binding room, bitch */
		if (ktp >= &keytab[NBINDS]) {
			mlwrite("Binding table FULL!");
			return(FALSE);
		}

		ktp->k_code = c;	/* add keycode */
		ktp->k_fp = kfunc;	/* and the function pointer */
		++ktp;			/* and make sure the next is null */
		ktp->k_code = 0;
		ktp->k_fp = NULL;
	}

	return(TRUE);
}

/* unbindkey:	delete a key from the key binding table
*/

int unbindkey(f, n)

int f, n;	/* command arguments [IGNORED] */

{
	register int c;		/* command key to unbind */
	register char *ptr;	/* ptr to dump out input key string */
	register KEYTAB *ktp;	/* pointer into the command table */
	register KEYTAB *sktp;	/* saved pointer into the command table */
	register int found;	/* matched command flag */
	char outseq[80];	/* output buffer for keystroke sequence */

	LINTUSE(f) LINTUSE(n)
	/* prompt the user to type in a key to unbind */
	mlwrite(": unbind-key ");

	/* get the command sequence to unbind */
	c = getckey();			/* get a command sequence */

	/* change it to something we can print as well */
	cmdstr(c, &outseq[0]);

	/* and dump it out */
	ptr = &outseq[0];
	while (*ptr)
		(*term.t_putchar)(*ptr++);

	/* search the table to see if the key exists */
	ktp = &keytab[0];
	found = FALSE;
	while (ktp->k_fp != NULL) {
		if (ktp->k_code == c) {
			found = TRUE;
			break;
		}
		++ktp;
	}

	/* if it isn't bound, bitch */
	if (!found) {
		mlwrite("[Key not bound]");
		return(FALSE);
	}

	/* save the pointer and scan to the end of the table */
	sktp = ktp;
	while (ktp->k_fp != NULL)
		++ktp;
	--ktp;		/* backup to the last legit entry */

	/* copy the last entry to the current one */
	sktp->k_code = ktp->k_code;
	sktp->k_fp   = ktp->k_fp;

	/* null out the last one */
	ktp->k_code = 0;
	ktp->k_fp = NULL;
	return(TRUE);
}

/* namedcmd:	execute a named command even if it is not bound
*/

int namedcmd(f, n)

int f, n;	/* command arguments [passed through to command executed] */

{
	register int (*kfunc)();	/* ptr to the requexted function to bind to */
	int (*getname())();

	/* prompt the user to type a named command */
	mlwrite(": ");

	/* and now get the function name to execute */
	kfunc = getname();
	if (kfunc == NULL) {
		mlwrite("[No such function]");
		return(FALSE);
	}

	/* and then execute the command */
	return((*kfunc)(f, n));
}

int desbind(f, n)	/* describe bindings
		   bring up a fake buffer and list the key bindings
		   into it with view mode			*/
{
	register WINDOW *wp;	/* scnaning pointer to windows */
	register KEYTAB *ktp;	/* pointer into the command table */
	register NBIND *nptr;	/* pointer into the name binding table */
	register BUFFER *bp;	/* buffer to put binding list into */
	char *strp;		/* pointer int string to send */
	int cpos;		/* current position to use in outseq */
	char outseq[80];	/* output buffer for keystroke sequence */

	LINTUSE(f) LINTUSE(n)
	/* split the current window to make room for the binding list */
	if (splitwind(FALSE, 1) == FALSE)
			return(FALSE);

	/* and get a buffer for it */
	bp = bfind("Binding list", TRUE, 0);
	if (bp == NULL || bclear(bp) == FALSE) {
		mlwrite("Can not display binding list");
		return(FALSE);
	}

	/* let us know this is in progress */
	mlwrite("[Building buffer list]");

	/* disconect the current buffer */
        if (--curbp->b_nwnd == 0) {             /* Last use.            */
                curbp->b_dotp  = curwp->w_dotp;
                curbp->b_doto  = curwp->w_doto;
                curbp->b_markp = curwp->w_markp;
                curbp->b_marko = curwp->w_marko;
        }

	/* connect the current window to this buffer */
	curbp = bp;	/* make this buffer current in current window */
	bp->b_mode = 0;		/* no modes active in binding list */
	bp->b_nwnd++;		/* mark us as more in use */
	wp = curwp;
	wp->w_bufp = bp;
	wp->w_linep = bp->b_linep;
	wp->w_flag = WFHARD|WFFORCE|WFHARD;
	wp->w_dotp = bp->b_dotp;
	wp->w_doto = bp->b_doto;
	wp->w_markp = NULL;
	wp->w_marko = 0;

	/* build the contents of this window, inserting it line by line */
	nptr = &names[0];
	while (nptr->n_func != NULL) {

		/* add in the command name */
		Strcpy(outseq, nptr->n_name);
		cpos = strlen(outseq);
		
		/* search down any keys bound to this */
		ktp = &keytab[0];
		while (ktp->k_fp != NULL) {
			if (ktp->k_fp == nptr->n_func) {
				/* padd out some spaces */
				while (cpos < 25)
					outseq[cpos++] = ' ';

				/* add in the command sequence */
				cmdstr(ktp->k_code, &outseq[cpos]);
				while (outseq[cpos] != 0)
					++cpos;

				/* and add it as a line into the buffer */
				strp = &outseq[0];
				while (*strp != 0)
					{IGNORE(linsert(1, *strp++));}
				IGNORE(lnewline());

				cpos = 0;	/* and clear the line */
			}
			++ktp;
		}

		/* if no key was bound, we need to dump it anyway */
		if (cpos > 0) {
			outseq[cpos] = 0;
			strp = &outseq[0];
			while (*strp != 0)
				{IGNORE(linsert(1, *strp++));}
			IGNORE(lnewline());
		}

		/* and on to the next name */
		++nptr;
	}

	curwp->w_bufp->b_mode |= MDVIEW;/* put this buffer view mode */
	curbp->b_flag &= ~BFCHG;	/* don't flag this as a change */
	wp->w_dotp = lforw(bp->b_linep);/* back to the begining */
	wp->w_doto = 0;
	wp = wheadp;			/* and update ALL mode lines */
	while (wp != NULL) {
		wp->w_flag |= WFMODE;
		wp = wp->w_wndp;
	}
	mlwrite("");	/* clear the mode line */
	return(TRUE);
}

/*	execcmd:	Execute a command line command to be typed in
			by the user					*/

int execcmd(f, n)

int f, n;	/* default Flag and Numeric argument */

{
	register int status;		/* status return */
	char cmdstr[NSTRING];		/* string holding command to execute */

	LINTUSE(f) LINTUSE(n)
	/* get the line wanted */
	if ((status = mlreply(": ", cmdstr, NSTRING)) != TRUE)
		return(status);

	return(docmd(cmdstr));
}

/*	docmd:	take a passed string as a command line and translate
		it to be executed as a command. This function will be
		used by execute-command-line and by all source and
		startup files.

	format of the command line is:

		{# arg} <command-name> {<argument string(s)>}
*/

int docmd(cline)

char *cline;	/* command line to execute */

{
	register char *cp;	/* pointer to current position in command */
	register char *tp;	/* pointer to current position in token */
	register int f;		/* default argument flag */
	register int n;		/* numeric repeat value */
	register int sign;	/* sign of numeric argument */
	register int (*fnc)();	/* function to execute */
	register int status;	/* return status of function */
	register int oldcle;	/* old contents of clexec flag */
	char token[NSTRING];	/* next token off of command line */
	int (*fncmatch())();
	char *gettok();

	/* first set up the default command values */
	f = FALSE;
	n = 1;

	cp = cline;		/* start at the begining of the line */
	cp = gettok(cp, token);	/* and grab the first token */

	/* check for and process numeric leadin argument */
	if ((token[0] >= '0' && token[0] <= '9') || token[0] == '-') {
		f = TRUE;
		n = 0;
		tp = &token[0];

		/* check for a sign! */
		sign = 1;
		if (*tp == '-') {
			++tp;
			sign = -1;
		}

		/* calc up the digits in the token string */
		while(*tp) {
			if (*tp >= '0' && *tp <= '9')
				n = n * 10 + *tp - '0';
			++tp;
		}
		n *= sign;	/* adjust for the sign */

		/* and now get the command to execute */
		cp = gettok(cp, token);		/* grab the next token */
	}

	/* and match the token to see if it exists */
	if ((fnc = fncmatch(token)) == NULL) {
		mlwrite("[No such Function]");
		return(FALSE);
	}
	
	/* save the arguments and go execute the command */
	Strcpy(sarg, cp);		/* save the rest */
	oldcle = clexec;		/* save old clexec flag */
	clexec = TRUE;			/* in cline execution */
	status = (*fnc)(f, n);		/* call the function */
	clexec = oldcle;		/* restore clexec flag */
	return(status);
}

/* gettok:	chop a token off a string
		return a pointer past the token
*/

char *gettok(src, tok)

char *src, *tok;	/* source string, destination token */

{
	/* first scan past any whitespace in the source string */
	while (*src == ' ' || *src == '\t')
		++src;

	/* if quoted, go till next quote */
	if (*src == '"') {
		++src;		/* past the quote */
		while (*src != 0 && *src != '"')
			*tok++ = *src++;
		++src;		/* past the last quote */
		*tok = 0;	/* terminate token and return */
		return(src);
	}

	/* copy until we find the end or whitespace */
	while (*src != 0 && *src != ' ' && *src != '\t')
		*tok++ = *src++;

	/* terminate tok and return */
	*tok = 0;
	return(src);
}

/* nxtarg:	grab the next token out of sarg, return it, and
		chop it of sarg					*/

int nxtarg(tok)

char *tok;	/* buffer to put token into */

{
	char *newsarg;	/* pointer to new begining of sarg */
	char *gettok();

	newsarg = gettok(sarg, tok);	/* grab the token */
	Strcpy(sarg, newsarg);		/* and chop it of sarg */
	return(TRUE);
}

int getckey()	/* get a command key sequence from the keyboard	*/

{
	register int c;		/* character fetched */
	register char *tp;	/* pointer into the token */
	char tok[NSTRING];	/* command incoming */

	/* check to see if we are executing a command line */
	if (clexec) {
		IGNORE(nxtarg(tok));	/* get the next token */

		/* parse it up */
		tp = &tok[0];
		c = 0;

		/* first, the META prefix */
		if (*tp == 'M' && *(tp+1) == '-') {
			c = META;
			tp += 2;
		}

		/* next the function prefix */
		if (*tp == 'F' && *(tp+1) == 'N') {
			c |= SPEC;
			tp += 2;
		}

		/* control-x as well... */
		if (*tp == '^' && *(tp+1) == 'X') {
			c |= CTLX;
			tp += 2;
		}

		/* a control char? */
		if (*tp == '^' && *(tp+1) != 0) {
			c |= CTRL;
			++tp;
		}

		/* make sure we are not lower case */
		if (c >= 'a' && c <= 'z')
			c -= 32;

		/* the final sequence... */
		c |= *tp;

		return(c);
	}

	/* or the normal way */
	c = getkey();			/* get a command sequence */
	if (c == (CTRL|'X'))		/* get control-x sequence */
		c = CTLX | getctl();
	return(c);
}

/*	execbuf:	Execute the contents of a named buffer	*/

int execbuf(f, n)

int f, n;	/* default flag and numeric arg */

{
        register BUFFER *bp;		/* ptr to buffer to execute */
        register int status;		/* status return */
        char bufn[NBUFN];		/* name of buffer to execute */

	LINTUSE(f)
	/* find out what buffer the user wants to execute */
        if ((status = mlreply("Execute buffer: ", bufn, NBUFN)) != TRUE)
                return(status);

	/* find the pointer to that buffer */
        if ((bp=bfind(bufn, TRUE, 0)) == NULL)
                return(FALSE);

	/* and now execute it as asked */
	while (n-- > 0)
		if ((status = dobuf(bp)) != TRUE)
			return(status);
	return(TRUE);
}

/*	dobuf:	execute the contents of the buffer pointed to
		by the passed BP				*/

int dobuf(bp)

BUFFER *bp;	/* buffer to execute */

{
        register int status;		/* status return */
	register LINE *lp;		/* pointer to line to execute */
	register LINE *hlp;		/* pointer to line header */
	register int linlen;		/* length of line to execute */
	register WINDOW *wp;		/* ptr to windows to scan */
	char eline[NSTRING];		/* text of line to execute */

	/* starting at the beginning of the buffer */
	hlp = bp->b_linep;
	lp = hlp->l_fp;
	while (lp != hlp) {
		/* calculate the line length and make a local copy */
		linlen = lp->l_used;
		if (linlen > NSTRING - 1)
			linlen = NSTRING - 1;
		Strncpy(eline, lp->l_text, linlen);
		eline[linlen] = 0;	/* make sure it ends */

		/* if it is not a comment, execute it */
		if (eline[0] != ';' && eline[0] != 0) {
			status = docmd(eline);
			if (status != TRUE) {	/* a command error */
				/* look if buffer is showing */
				wp = wheadp;
				while (wp != NULL) {
					if (wp->w_bufp == bp) {
						/* and point it */
						wp->w_dotp = lp;
						wp->w_doto = 0;
						wp->w_flag |= WFHARD;
					}
					wp = wp->w_wndp;
				}
				/* in any case set the buffer . */
				bp->b_dotp = lp;
				bp->b_doto = 0;
				return(status);
			}
		}
		lp = lp->l_fp;		/* on to the next line */
	}
        return(TRUE);
}

int execfile(f, n)	/* execute a series of commands in a file
*/

int f, n;	/* default flag and numeric arg to pass on to file */

{
	register int status;	/* return status of name query */
	/* mlh found bug *fname[NSTRING] */
	char fname[NSTRING];	/* name of file to execute */

	LINTUSE(f)
	if ((status = mlreply("File to execute: ", fname, NSTRING -1)) != TRUE)
		return(status);

	/* otherwise, execute it */
	while (n-- > 0)
		if ((status=dofile(fname)) != TRUE)
			return(status);

	return(TRUE);
}

/*	dofile:	yank a file into a buffer and execute it
		if there are no errors, delete the buffer on exit */

int dofile(fname)

char *fname;	/* file name to execute */

{
	register BUFFER *bp;	/* buffer to place file to exeute */
	register BUFFER *cb;	/* temp to hold current buf while we read */
	register int status;	/* results of various calls */
	char bname[NBUFN];	/* name of buffer */

	makename(bname, fname);		/* derive the name of the buffer */
	if ((bp = bfind(bname, TRUE, 0)) == NULL) /* get the needed buffer */
		return(FALSE);

	bp->b_mode = MDVIEW;	/* mark the buffer as read only */
	cb = curbp;		/* save the old buffer */
	curbp = bp;		/* make this one current */
	/* and try to read in the file to execute */
	if ((status = readin(fname, FALSE)) != TRUE) {
		curbp = cb;	/* restore the current buffer */
		return(status);
	}

	/* go execute it! */
	curbp = cb;		/* restore the current buffer */
	if ((status = dobuf(bp)) != TRUE)
		return(status);

	/* if not displayed, remove the now unneeded buffer and exit */
	if (bp->b_nwnd == 0)
		{IGNORE(zotbuf(bp));}
	return(TRUE);
}


/* execute the startup file */

int startup()

{
	register int status;	/* status of I/O operations */
	register int i;		/* index into help file names */
	char fname[NSTRING];	/* buffer to construct file name in */

#if	(MSDOS & (LATTICE | MSC)) | V7
	char *homedir;		/* pointer to your home directory */
	char *getenv();
	
	/* get the HOME from the environment */
	if ((homedir = getenv("HOME")) != NULL) {
		/* build the file name */
		Strcpy(fname, homedir);
		Strcat(fname, "/");
		Strcat(fname, pathname[0]);

		/* and test it */
		status = ffropen(fname);
		if (status == FIOSUC) {
			IGNORE(ffclose());
			return(dofile(fname));
		}
	}
#endif

	/* search through the list of startup files */
	for (i=2; i < NPNAMES; i++) {
		Strcpy(fname, pathname[i]);
		Strcat(fname, pathname[0]);
		status = ffropen(fname);
		if (status == FIOSUC)
			break;
	}

	/* if it isn't around, don't sweat it */
	if (status == FIOFNF)
		return(TRUE);

	IGNORE(ffclose()); /* close the file to prepare for to read it in */

	return(dofile(fname));
}

