/* /local/src/master/nrn/src/modlunit/symbol.h,v 1.1.1.1 1994/10/12 17:22:46 hines Exp */

extern List *symlist[];
#define SYMITERALL	{int i; Item *qs; Symbol *s;\
	for (i = 'A'; i <= 'z'; i++)\
	ITERATE(qs, symlist[i])\
		if ((s = SYM(qs))->type == NAME || s->type == PRIME)
		
#define SYMITER(arg1)	{int i; Item *qs; Symbol *s;\
	for (i = 'A'; i <= 'z'; i++)\
	ITERATE(qs, symlist[i])\
		if ((s = SYM(qs))->type == arg1)
		
#define SYMITER_STAT	{int i; Item *qs; Symbol *s;\
	for (i='A'; i <= 'z'; i++)\
	ITERATE(qs, symlist[i])\
		if ((s = SYM(qs))->subtype & STAT)

#define SYMITER_SUB(arg1)	{int i; Item *qs; Symbol *s;\
	for (i='A'; i <= 'z'; i++)\
	ITERATE(qs, symlist[i])\
		if ((s = SYM(qs))->subtype & (arg1))

/* symbol.h,v
 * Revision 1.1.1.1  1994/10/12  17:22:46  hines
 * NEURON 3.0 distribution
 *
 * Revision 1.1  90/11/13  16:12:02  hines
 * Initial revision
 *  */
