#include <../../nrnconf.h>
/* LINTLIBRARY */
/* /local/src/master/nrn/src/oc/hocedit.c,v 1.6 1999/07/03 14:20:24 hines Exp */
/*
hocedit.c,v
 * Revision 1.6  1999/07/03  14:20:24  hines
 * no limit on input string size or execute strings.
 * (although an execute string could overrun the program buffer if
 * it generates more than NPROG instructions, see code.cpp)
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

#include <stdio.h>
#include <hocdec.h>
#include <hocparse.h>
#include <setjmp.h>

size_t hoc_pipegets_need(void) {
    return hoc_strgets_need();
}

char* hoc_pipegets(char* cbuf, int nc) {
    return hoc_strgets(cbuf, nc - 1);
}


