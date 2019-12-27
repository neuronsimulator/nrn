/* /local/src/master/nrn/src/nmodl/symbol.h,v 4.1 1997/08/30 20:45:37 hines Exp */
/*
symbol.h,v
 * Revision 4.1  1997/08/30  20:45:37  hines
 * cvs problem with branches. Latest nmodl stuff should now be a top level
 *
 * Revision 4.0.1.1  1997/08/08  17:24:05  hines
 * nocmodl version 4.0.1
 *
 * Revision 4.0  1997/08/08  17:06:32  hines
 * proper nocmodl version number
 *
 * Revision 1.1.1.1  1994/10/12  17:21:33  hines
 * NEURON 3.0 distribution
 *
 * Revision 8.0  89/09/22  17:27:04  nfh
 * Freezing
 * 
 * Revision 7.0  89/08/30  13:32:42  nfh
 * Rev 7 is now Experimental; Rev 6 is Testing
 * 
 * Revision 6.0  89/08/14  16:27:21  nfh
 * Rev 6.0 is latest of 4.x; now the Experimental version
 * 
 * Revision 4.0  89/07/24  17:03:52  nfh
 * Freezing rev 3.  Rev 4 is now Experimental
 * 
 * Revision 3.1  89/07/07  16:55:21  mlh
 * FIRST LAST START in independent SWEEP higher order derivatives
 * 
 * Revision 1.1  89/07/06  14:52:48  mlh
 * Initial revision
 * 
*/

extern List *symlist[];
#define SYMITER(arg1)	for (i = 'A'; i <= 'z'; i++)\
	ITERATE(qs, symlist[i])\
		if ((s = SYM(qs))->type == arg1)
		
#define SYMITER_STAT	for (i='A'; i <= 'z'; i++)\
	ITERATE(qs, symlist[i])\
		if ((s = SYM(qs))->subtype & STAT)
		

