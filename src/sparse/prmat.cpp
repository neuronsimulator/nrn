#include <../../nrnconf.h>
/*LINTLIBRARY*/
#if LINT
#define NRN_IGNORE(arg)	{if(arg);}
#else
#define NRN_IGNORE(arg)	arg
#endif
#include "lineq.h"

void prmat(void)
{
	int i, j;
	struct elm *el;

	NRN_IGNORE(printf("\n\n    "));
	for (i=10 ; i <= neqn ; i += 10)
		NRN_IGNORE(printf("         %1d", (i%100)/10));
	NRN_IGNORE(printf("\n    "));
	for (i=1 ; i <= neqn; i++)
		NRN_IGNORE(printf("%1d", i%10));
	NRN_IGNORE(printf("\n\n"));
	for (i=1 ; i <= neqn ; i++)
	{
		NRN_IGNORE(printf("%3d ", i));
		j = 0;
		for (el = rowst[i] ;el != ELM0 ; el = el->c_right)
		{
			for ( j++ ; j < el->col ; j++)
				NRN_IGNORE(putchar(' '));
			NRN_IGNORE(putchar('*'));
		}
		NRN_IGNORE(putchar('\n'));
	}
}
