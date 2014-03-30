#include <../../nmodlconf.h>
/* /local/src/master/nrn/src/nmodl/discrete.c,v 4.1 1997/08/30 20:45:19 hines Exp */
/*
discrete.c,v
 * Revision 4.1  1997/08/30  20:45:19  hines
 * cvs problem with branches. Latest nmodl stuff should now be a top level
 *
 * Revision 4.0.1.1  1997/08/08  17:23:42  hines
 * nocmodl version 4.0.1
 *
 * Revision 4.0  1997/08/08  17:06:07  hines
 * proper nocmodl version number
 *
 * Revision 1.1.1.1  1994/10/12  17:21:35  hines
 * NEURON 3.0 distribution
 *
 * Revision 9.5  90/07/18  07:59:36  hines
 * define for arrays now (p + n) instead of &p[n]. This allows the c file
 * to have arrays that look like a[i] instead of *(a + i).
 * 
 * Revision 8.1  90/01/03  16:13:01  mlh
 * discrete array variables had their for loops switched.
 * 
 * Revision 8.0  89/09/22  17:26:05  nfh
 * Freezing
 * 
 * Revision 7.0  89/08/30  13:31:32  nfh
 * Rev 7 is now Experimental; Rev 6 is Testing
 * 
 * Revision 6.0  89/08/14  16:26:16  nfh
 * Rev 6.0 is latest of 4.x; now the Experimental version
 * 
 * Revision 4.0  89/07/24  17:02:45  nfh
 * Freezing rev 3.  Rev 4 is now Experimental
 * 
 * Revision 3.3  89/07/21  09:28:14  mlh
 * Discrete equitions evaluated at time given by independent variable
 * in the sense that the state on the left hand side refers to state(t)
 * and explicit dependence on t works naturally.
 * 
 * Revision 1.3  89/07/21  09:14:08  mlh
 * Discrete equations evaluated at time given by ndependent variable
 * in the sense that state on the left hand side refers to state(t)
 * and explicit dependence on t works naturally.  This is done by
 * incrementing t before calling discrete block and updating the
 * history at the beginning instead of the end.
 * 
 * Revision 1.2  89/07/12  13:57:21  mlh
 * state@1 now refers to fianl value at previous step
 * state@0 is a syntax error
 * 
 * Revision 1.1  89/07/06  14:48:27  mlh
 * Initial revision
 * 
*/

#include <stdlib.h>
#include "modl.h"
#include "parse1.h"
#include "symbol.h"

void disc_var_seen(q1, q2, q3, array)	/*NAME '@' NUMBER --- array flag*/
	Item *q1, *q2, *q3;
	int array;
{
	Symbol *s;
	int num;
	
	num = atoi(STR(q3));
	s = SYM(q1);
	if (num < 1) {
		diag("Discrete variable must have @index >= 1", (char *)0);
	}  
	num--;
	if (!(s->subtype & STAT)) {
	diag(s->name, " must be a STATE for use as discrete variable");
	}
	if (array && !(s->subtype & ARRAY)) {
		diag(s->name, " must be a scalar discrete variable");
	}
	if (!array && (s->subtype & ARRAY)) {
		diag(s->name, " must be an array discrete variable");
	}
	if (s->discdim <= num) {
		s->discdim = num+1;
	}
	Sprintf(buf, "__%s", s->name);
	replacstr(q1, buf);
	delete(q2);
	Sprintf(buf, "[%d]", num);
	replacstr(q3, buf);
}

void massagediscblk(q1, q2, q3, q4) /*DISCRETE NAME stmtlist '}'*/
	Item *q1, *q2, *q3, *q4;
{
	int i;
	Symbol *s;
	Item *qs;
	
	replacstr(q1, "int");
	Insertstr(q3, "()\n{\n");
	Insertstr(q4, "}\n");
	SYM(q2)->subtype |= DISCF;
	SYMITER(NAME) if (s->subtype & STAT && s->used && s->discdim) {
		if (s->subtype & ARRAY) {
Sprintf(buf, "{int _i, _j; for (_j=%d; _j >=0; _j--) {\n\
for (_i=%d; _i>0; _i--) __%s[_i][_j] = __%s[_i-1][_j];\n\
 __%s[0][_j] = %s[_j];\n\
}}\n",
			s->araydim -1,
			s->discdim -1, s->name, s->name, s->name, s->name);
		}else{
Sprintf(buf, "{int _i; for (_i=%d; _i>0; _i--) __%s[_i] = __%s[_i-1];\n\
 __%s[0] = %s;\n}\n",
			s->discdim -1, s->name, s->name, s->name, s->name);
		}
		Insertstr(q3, buf);
		s->used = 0;
	}
	/*initialization and declaration done elsewhere*/
	movelist(q1, q4, procfunc);
}

void init_disc_vars()
{
	int i;
	Item *qs;
	Symbol *s;
	SYMITER(NAME) if (s->subtype & STAT && s->discdim) {
		if (s->subtype & ARRAY) {
Sprintf(buf, "{int _i, _j; for (_j=%d; _j >=0; _j--) {\n\
for (_i=%d; _i>=0; _i--) __%s[_i][_j] = %s0;}}\n",
			s->araydim -1,
			s->discdim -1, s->name, s->name);
			Linsertstr(initfunc, buf);
Sprintf(buf, "static double __%s[%d][%d];\n", s->name, s->discdim, s->araydim);
			Linsertstr(procfunc, buf);
		}else{
Sprintf(buf, "{int _i; for (_i=%d; _i>=0; _i--) __%s[_i] = %s0;}\n",
			s->discdim -1, s->name, s->name);
			Linsertstr(initfunc, buf);
Sprintf(buf, "static double __%s[%d];\n", s->name, s->discdim);
			Linsertstr(procfunc, buf);
		}
	}
}




