#include <../../nrnconf.h>
/* /local/src/master/nrn/src/oc/spinit.c,v 1.1.1.1 1994/10/12 17:22:14 hines Exp */
/*
spinit.c,v
 * Revision 1.1.1.1  1994/10/12  17:22:14  hines
 * NEURON 3.0 distribution
 *
 * Revision 2.64  1993/11/04  15:58:42  hines
 * forgot to checkin last time
 *
 * Revision 2.16  1993/01/22  17:34:22  hines
 * ocmodl, ivmodl working both for shared and static libraries
 *
 * Revision 1.2  92/08/12  11:56:33  hines
 * hoc_fake_ret() allows calls to functions that do a ret(). These functions
 * can have no arguments and the caller must pop the stack and deal with
 * hoc_returning.
 * This was done so init... could be called both from hoc_spinit and from
 * hoc.
 * last function called by hoc_spinit in hocusr.c is hoc_last_init()
 * 
 * Revision 1.1  91/10/11  11:12:18  hines
 * Initial revision
 * 
 * Revision 2.0  89/07/07  11:33:01  mlh
 * Preparation for newcable
 * 
 * Revision 1.1  89/07/07  11:17:43  mlh
 * Initial revision
 * 
*/


void hoc_spinit(void)	/* Dummy special init */
{
}
