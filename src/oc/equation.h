/* $Header$ */
/*
$Log$
Revision 1.1  2003/02/11 18:36:09  hines
Initial revision

Revision 1.1.1.1  2003/01/01 17:46:34  hines
NEURON --  Version 5.4 2002/12/23 from prospero as of 1-1-2003

Revision 1.1.1.1  2002/06/26 16:23:07  hines
NEURON 5.3 2002/06/04

 * Revision 1.1.1.1  2001/01/01  20:30:35  hines
 * preparation for general sparse matrix
 *
 * Revision 1.1.1.1  2000/03/09  13:55:34  hines
 * almost working
 *
 * Revision 1.2  1994/10/26  17:24:59  hines
 * access name changed to an explicit hoc_access and taken out of redef.h
 *
 * Revision 1.1.1.1  1994/10/12  17:22:02  hines
 * NEURON 3.0 distribution
 *
 * Revision 1.1  91/10/11  11:12:26  hines
 * Initial revision
 * 
 * Revision 3.28  89/09/29  16:02:46  mlh
 * need to use -1 as falg for unsigned variable
 * 
 * Revision 2.0  89/07/07  11:30:34  mlh
 * Preparation for newcable
 * 
 * Revision 1.1  89/07/07  11:15:45  mlh
 * Initial revision
 * 
*/

extern int	do_equation;	/* switch for determining access to dep vars */
extern int	*hoc_access;	/* links to next accessed variables */
extern int  var_access;	/* variable number as pointer into access array */
extern void eqn_name(void), eqn_init(void), eqn_lhs(void), eqn_rhs(void), dep_make(void);
