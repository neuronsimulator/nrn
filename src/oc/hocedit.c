#include <../../nrnconf.h>
/* LINTLIBRARY */
/* /local/src/master/nrn/src/oc/hocedit.c,v 1.6 1999/07/03 14:20:24 hines Exp */
/*
hocedit.c,v
 * Revision 1.6  1999/07/03  14:20:24  hines
 * no limit on input string size or execute strings.
 * (although an execute string could overrun the program buffer if
 * it generates more than NPROG instructions, see code.c)
 *
 * Revision 1.5  1997/03/21  21:28:33  hines
 * syntax errors now give correct file and line number message
 *
 * Revision 1.4  1997/03/13  14:18:12  hines
 * Merge Macintosh changes. NEURON sources compile with Codewarrior 11.
 * No readline or microemacs for mac.
 *
 * Revision 1.3  1996/02/16  16:19:29  hines
 * OCSMALL used to throw out things not needed by teaching programs
 *
 * Revision 1.2  1994/11/08  19:37:12  hines
 * command line editing works while in graphics mode with GRX and GO32
 *
 * Revision 1.1.1.1  1994/10/12  17:22:10  hines
 * NEURON 3.0 distribution
 *
 * Revision 2.62  1993/10/18  13:24:19  hines
 * A start on an audit system. see audit.c
 *
 * Revision 1.1  91/10/11  11:12:07  hines
 * Initial revision
 * 
 * Revision 4.43  91/10/01  11:34:24  hines
 * gather console input at one place in preparation for adding
 * emacs like command line editing. Hoc input now reads entire line.
 * 
 * Revision 4.37  91/08/13  19:50:49  hines
 * can now use em after window size is changed.
 * 
 * Revision 3.108  90/10/24  09:44:12  hines
 * saber warnings gone
 * 
 * Revision 3.83  90/07/25  10:40:25  hines
 * almost lint free on sparc 1+ under sunos 4.1
 * 
 * Revision 3.4  89/07/12  10:27:18  mlh
 * Lint free
 * 
 * Revision 3.3  89/07/10  15:46:16  mlh
 * Lint pass1 is silent. Inst structure changed to union.
 * 
 * Revision 2.0  89/07/07  11:31:46  mlh
 * Preparation for newcable
 * 
 * Revision 1.1  89/07/07  11:16:25  mlh
 * Initial revision
 * 
*/

/* example to show how microemacs can be embedded in a larger program */

#include <stdio.h>
#include <hocdec.h>
#include <hocparse.h>
#include <setjmp.h>

#if MAC
	#define OCSMALL 1
#endif

#if defined(WITHOUT_MEMACS)
#undef OCSMALL
#define OCSMALL 1
#endif

#if !OCSMALL

#include "estruct.h"

extern int emacs_main(int n, char** cpp);
extern int emacs_refresh(int i, int j);
extern int emacs_vtinit(void);
#define IGNORE(arg)	arg
#define Fprintf		fprintf

extern TERM emacs_term;
extern BUFFER *emacs_curbp;
extern WINDOW *emacs_curwp;

extern int hoc_pipeflag;
extern int hoc_lineno;

static jmp_buf	emacs_begin;

static int called = 0;
static LINE *lp, *lhead;
static short cnt;

static char *argv[] = { "embedded", (char *)0};

#endif /* !OCSMALL */

void hoc_edit(void)
{
#if !OCSMALL
#if	DOS || defined(__GO32__) /*must erase screen if in graphics mode*/
	extern int egagrph;
	if (egagrph) {
		plt(-3,0.,0.);
	}
#endif
	if (hoc_retrieving_audit()) {
		hoc_emacs_from_audit();
		return;
	}
	if (setjmp(emacs_begin)) {
		return;
	}
	if (!called) {
		called = 1;
		emacs_main(1, argv);
	} else {
		emacs_vtinit();
		emacs_refresh(0, 1);
		emacs_main(-1, argv);
	}
#endif
}

void hoc_edit_quit(void)
{
#if !OCSMALL
	char s[2];
	if (called) {
		argv[0] = s;
		argv[0][0] = 'Z' & 037;
		rewind(stdin);	/* else continuous EOF */
		hoc_edit();
	}
#endif
}

int emacs_exit(int status) {
#if !OCSMALL
	if (status) {
		Fprintf(stderr, "emacs--status = %d\n", status);
		hoc_pipeflag = 0;
		hoc_execerror("Error in emacs return", (char *)0);
	}
		
	if (status == 0) {
		lhead = emacs_curbp->b_linep;
		lp = lforw(lhead);
		cnt = 0;
		hoc_pipeflag = 1;
		hoc_lineno = 0;
		hoc_audit_from_emacs(emacs_curbp->b_bname, emacs_curbp->b_fname);
		lp = lforw(lhead);
		cnt = 0;
	}
	longjmp(emacs_begin, 1);
#endif
	return 0;
}

#if !OCSMALL
	static LINE *lastlp;
#endif

void hoc_pipeflush(void)
{
#if !OCSMALL
if (hoc_pipeflag == 1) {
	extern int hoc_ictp;
	if (ired("\nReenter emacs", 1, 0, 1)) {
		emacs_curwp->w_dotp = lback(lp);
		if (hoc_ictp < llength(lback(lp))) {
			emacs_curwp->w_doto = hoc_ictp;
		}else{
			emacs_curwp->w_doto = llength(lback(lp))-1;
		}
		IGNORE(emacs_refresh(1, 1));	/* recenters "." in current window */
		hoc_edit();
	} else {
		cnt = 0;
		lp = lastlp = lhead;
	}
}
#endif
}

size_t hoc_pipegets_need(void) {
	int hoc_strgets_need();
#if !OCSMALL
	if (hoc_pipeflag == 1) {
		if (lp == lhead) {
			return 0;
		}else{
			return llength(lp);
		}
	}else{
		return hoc_strgets_need();
	}
#else
	return hoc_strgets_need();
#endif
}

char* hoc_pipegets(char* cbuf, int nc) {
	char *hoc_strgets(), *cp=cbuf;
	
	nc--;
#if !OCSMALL
if (hoc_pipeflag == 1) {
	if (lp == lhead) {
		cnt = 0;
		return (char *)0;
	}
	for (cnt=0; cnt < llength(lp) && cnt < nc; cnt++) {
		*cp++ = lgetc(lp, cnt);
	}
	*cp++ = '\n';
	*cp = '\0';
	lp = lforw(lp);
	return cbuf;
}else{
#else
{
#endif
	return hoc_strgets(cbuf, nc);
}
}
