#include <../../nrnconf.h>
/* /local/src/master/nrn/src/oc/plt.c,v 1.2 1995/07/22 13:01:54 hines Exp */
/*
plt.c,v
 * Revision 1.2  1995/07/22  13:01:54  hines
 * avoid unhandled exceptions in mswindows due to some function stubs
 *
 * Revision 1.1.1.1  1994/10/12  17:22:13  hines
 * NEURON 3.0 distribution
 *
 * Revision 1.1  91/10/11  11:12:16  hines
 * Initial revision
 * 
 * Revision 4.16  91/03/14  13:12:24  hines
 * x11 graphics has fast mode (plt(-5)) which is more than 5 times
 * faster than default mode in which server is flushed on every plot
 * call. in fast mode plt(-1) will flush server.
 * 
 * Revision 3.61  90/05/19  12:58:35  mlh
 * switch from TEK to gfxtool using suncore.
 * setcolor(0) draws invisible line setcolor(1) draws visible line
 * for graphing at this time you must run hoc in a gfxtool window.
 * 
 * Revision 3.35  89/11/08  10:09:20  mlh
 * merge turboc and unix versions. Lw can take second argument
 * to switch between hp and fig
 * 
 * Revision 3.20  89/08/15  08:29:57  mlh
 * compiles under turbo-c 1.5 -- some significant bugs found
 * 
 * Revision 3.7  89/07/13  08:21:49  mlh
 * stack functions involve specific types instead of Datum
 * 
 * Revision 3.4  89/07/12  10:28:11  mlh
 * Lint free
 * 
 * Revision 2.0  89/07/07  11:32:53  mlh
 * Preparation for newcable
 * 
 * Revision 1.1  89/07/07  11:17:35  mlh
 * Initial revision
 * 
*/

#define FIG 1/* version 7.1.1 12/8/88
   added plots in fig format
   */
#include "hoc.h"

Plt()
{
	int mode;
	double x, y;
#ifndef WIN32
	mode = *getarg(1);
	if (mode >= 0 || ifarg(2))
	{
		if ((x = *getarg(2)) > 2047)
			x = 2047;
		else if (x < 0)
			x = 0;
		if ((y = *getarg(3)) > 2047)
			y = 2047;
		else if (y < 0)
			y = 0;
	}else{
		x=y=0.;
	}
	plt(mode, x, y);
#endif
	ret();
	pushx(1.);
}

Setcolor()
{
	double i;
	i = set_color((int)*getarg(1));
	ret();
	pushx(i);
}

hoc_Lw()
{
	char *s;
	static int dev=2;
#ifndef WIN32
	if (ifarg(1)) {
		s = gargstr(1);
		if (ifarg(2)) {
			dev = *getarg(2);
		}
		if (s[0] != '\0') {
			Fig_file(s, dev);
		} else {
			Fig_file((char *)0, dev);
		}
	} else {
		Fig_file((char *) 0, dev);
	}
#endif
	ret();
	pushx(0.);
}

