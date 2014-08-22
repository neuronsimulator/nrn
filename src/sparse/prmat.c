#include <../../nrnconf.h>
/*LINTLIBRARY*/
#if LINT
#define IGNORE(arg)	{if(arg);}
#else
#define IGNORE(arg)	arg
#endif
#include "lineq.h"

void prmat(void)
{
	int i, j;
	struct elm *el;

	IGNORE(printf("\n\n    "));
	for (i=10 ; i <= neqn ; i += 10)
		IGNORE(printf("         %1d", (i%100)/10));
	IGNORE(printf("\n    "));
	for (i=1 ; i <= neqn; i++)
		IGNORE(printf("%1d", i%10));
	IGNORE(printf("\n\n"));
	for (i=1 ; i <= neqn ; i++)
	{
		IGNORE(printf("%3d ", i));
		j = 0;
		for (el = rowst[i] ;el != ELM0 ; el = el->c_right)
		{
			for ( j++ ; j < el->col ; j++)
				IGNORE(putchar(' '));
			IGNORE(putchar('*'));
		}
		IGNORE(putchar('\n'));
	}
}
