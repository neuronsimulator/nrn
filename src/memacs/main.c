#include <../../nrnconf.h>

/*mlh 7/20/87 dummy main routine to allow emacs to be embedded
 *in larger programs. This is used for stand alone emacs.
 *See main1.c for the real start
 */
 /*
#if defined(_Windows) && !defined(__MWERKS__)
#define main winio_main
#endif
*/
#include <stdlib.h>

extern int emacs_main();

int main(argc, argv)
	int argc;
	char **argv;
{
	emacs_main(argc, argv);
	return 0;
}

/*
#ifdef _Windows
void set_intset(){}
#endif
*/

#if defined(__MWERKS__)
void set_intset(){}
void hoc_quit(){winio_closeall();}
#endif

int emacs_exit(status)
	int status;
{
	exit(status);
	return 0;
}
