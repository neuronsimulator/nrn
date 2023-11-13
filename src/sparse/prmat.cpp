#include <../../nrnconf.h>
#include "lineq.h"

void prmat(void)
{
	int i, j;
	struct elm *el;

	printf("\n\n    ");
	for (i=10 ; i <= neqn ; i += 10)
		printf("         %1d", (i%100)/10);
	printf("\n    ");
	for (i=1 ; i <= neqn; i++)
		printf("%1d", i%10);
	printf("\n\n");
	for (i=1 ; i <= neqn ; i++)
	{
		printf("%3d ", i);
		j = 0;
		for (el = rowst[i] ;el != ELM0 ; el = el->c_right)
		{
			for ( j++ ; j < el->col ; j++)
				putchar(' ');
			putchar('*');
		}
		putchar('\n');
	}
}
