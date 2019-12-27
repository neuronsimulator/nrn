#include <../../nmodlconf.h>
/* /local/src/master/nrn/src/modlunit/passn.c,v 1.1.1.1 1994/10/12 17:22:50 hines Exp */

/* Returns parse tokens in same order that lexical analyzer did */

#include "model.h"
#include "parse1.h"

#define DEBUG 0
#if DEBUG
static int debugtoken=1;
#else
static int debugtoken=0;
#endif

Item *lex_tok;
int parse_pass=0;
int restart_pass=0;
extern int yylex();

void parsepass(n)
	int n;
{
	unitonflag = 1;
	parse_pass = n;
	if (parse_pass != 1) {
		lex_tok = intoken;
	}
}

void parse_restart(q, i)
	Item *q;
	int i;
{
	if (i == restart_pass) {
		restart_pass = 0;
		return;
	}
	restart_pass = i;
	lex_tok = prev_parstok(q);
	if (!lex_tok) {
		lex_tok = intoken;
	}
}

int next_intoken(pitem)
	Item **pitem;
{
	if (parse_pass == 1) {
		return yylex();
	}
	lex_tok = next_parstok(lex_tok);
	if (lex_tok) {
		*pitem = lex_tok;
		if (debugtoken) {
			debugitem(*pitem);
		}
		return (*pitem)->itemsubtype;
	}
	return 0;
}

Item *
next_parstok(intok)
	Item *intok;
{
	if (!intok) {
		return ITEM0;
	}
	while ((intok = intok->next) != intoken) {
		/*EMPTY*/
		if (intok->itemtype == NEWLINE) {
			;
		}else{
			switch (intok->itemsubtype) {
			case STUFF:
			case SPACE:
				break;
			default:
				return intok;
			}
		}
	}
	return ITEM0;
}

Item *
prev_parstok(intok)
	Item *intok;
{
	if (!intok) {
		return ITEM0;
	}
	while ((intok = intok->prev) != intoken) {
		/*EMPTY*/
		if (intok->itemtype == NEWLINE) {
			;
		}else{
			switch (intok->itemsubtype) {
			case STUFF:
			case SPACE:
				break;
			default:
				return intok;
			}
		}
	}
	return ITEM0;
}

/* passn.c,v
 * Revision 1.1.1.1  1994/10/12  17:22:50  hines
 * NEURON 3.0 distribution
 *
 * Revision 1.2  91/01/07  14:17:12  hines
 * in kinunit, wrong itemsubtype.  Fix lint messages
 * 
 * Revision 1.1  90/11/13  16:10:22  hines
 * Initial revision
 *  */
