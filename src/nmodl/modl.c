#include <../../nmodlconf.h>

/*
 * int main(int argc, char *argv[]) --- returns 0 if translation is
 * successful. Diag will exit with 1 if error. 
 *
 * ---The overall strategy of the translation consists of three phases. 
 *
 * 1) read in the whole file as a sequence of tokens, parsing as we go. Most of
 * the trivial C translation such as appending ';' to statements is performed
 * in this phase as is the creation of the symbol table. Item lists maintain
 * the proper token order. Ater a whole block is read in, nontrivial
 * manipulation may be performed on the entire block. 
 *
 * 2) Some blocks and statements can be manipulated only after the entire file
 * has been read in. The solve statement is an example since it can be
 * analysed only after we know what is the type of the associated block.  The
 * kinetic block is another example whose translation depends on the SOLVE
 * method and so cannot be processed until the whole input file has been
 * read. 
 *
 * 3) Output the lists. 
 *
 * void openfiles(int argc, char *argv[]) parse the argument list, and open
 * files. Print usage message and exit if no argument 
 *
 */

/*
 * In order to interface this process with merge, a second argument is
 * allowed which gives the complete input filename.  The first argument
 * still gives the prefix of the .c and .var files.
 */
 
/* the first arg may also be a file.mod (containing the .mod suffix)*/
#if MAC
#include <sioux.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include "modl.h"
FILE
	* fin,			/* input file descriptor for filename.mod */
				/* or file2 from the second argument */
	*fparout,		/* output file descriptor for filename.var */
	*fcout;			/* output file descriptor for filename.c */
#if SIMSYS
FILE	*fctlout,		/* filename.ctl */
	*fnumout;		/* filename.num */
#endif


char		*modprefix, prefix_[NRN_BUFSIZE];	/* the first argument */

char            finname[NRN_BUFSIZE];	/* filename.mod  or second argument */

#if LINT
char           *clint;
int             ilint;
Item           *qlint;
#endif

extern int yyparse();

#if NMODL && VECTORIZE
extern int vectorize;
extern int numlist;
extern char* nmodl_version_;
extern int usederivstatearray;
#endif

/*SUPPRESS 763*/
static char pgm_name[] =	"nmodl";
extern char *RCS_version;
extern char *RCS_date;
static void openfiles();

int main(argc, argv)
	int             argc;
	char           *argv[]; {
	/*
	 * arg 1 is the prefix to the input file and output .c and .par
	 * files 
	 * We first look for a .mrg file and then a .mod file
	 */
#if NMODL
	if (argc > 1 && strcmp(argv[1], "--version") == 0) {
		printf("%s\n", nmodl_version_);
		exit(0);
	}
#endif
#if MAC
	SIOUXSettings.asktosaveonclose = false;
#if !SIMSYS
	Fprintf(stderr, "%s   %s   %s\n",
		pgm_name, RCS_version, RCS_date);
#endif
#endif	
							
	modprefix = prefix_;
	init();			/* keywords into symbol table, initialize
				 * lists, etc. */
#if MAC
	modl_units(); /* since we will be changing the cwd */
	mac_cmdline(&argc, &argv);
	{
	char cs[NRN_BUFSIZE], *cp;
	strncpy(cs, argv[1], 256);
	cp  = strrchr(cs, ':');
	if (cp) {
		*cp = '\0';
		 if (chdir(cs) == 0) {
			printf("current directory is \"%s\"\n", cs);
			strcpy(argv[1], cp+1);
		}
	}
	}		

#endif

	openfiles(argc, argv);	/* .mrg else .mod,  .var, .c */
#if NMODL || HMODL
	Fprintf(stderr, "Translating %s into %s.c\n", finname,
		modprefix);
#else
#if !SIMSYS
	Fprintf(stderr, "Translating %s into %s.c and %s.var\n", finname,
		modprefix, modprefix);
#endif
#endif
	IGNORE(yyparse());
	/*
	 * At this point all blocks are fully processed except the kinetic
	 * block and the solve statements. Even in these cases the 
	 * processing doesn't involve syntax since the information is
	 * held in intermediate lists of specific structure.
	 *
	 */
	/*
	 * go through the list of solve statements and construct the model()
	 * code 
	 */
	solvhandler();
	/* 
	 * NAME's can be used in many cases before they were declared and
	 * no checking up to this point has been done to make sure that
	 * names have been used in only one way.
	 *
	 */
	consistency();
#if 0 && !_CRAY && NMODL && VECTORIZE
/* allowing Kinetic models to be vectorized on cray. So nonzero numlist is
no longer adequate for saying we can not */
	if (numlist) {
		vectorize = 0;
	}
#endif
	chk_thread_safe();
	parout();		/* print .var file.
				 * Also #defines which used to be in defs.h
				 * are printed into .c file at beginning.
				 */
	c_out();			/* print .c file */
#if HMODL || NMODL
#else
	IGNORE(fclose(fparout));
#endif
#if SIMSYS
	IGNORE(fclose(fctlout));
	IGNORE(fclose(fnumout));
#endif
	IGNORE(fclose(fcout));

#if NMODL && VECTORIZE
	if (vectorize) {
		Fprintf(stderr, "Thread Safe\n");
	}
	if (usederivstatearray) {
fprintf(stderr, "Derivatives of STATE array variables are not translated correctly and compile time errors will be generated.\n");
fprintf(stderr, "The %s.c file may be manually edited to fix these errors.\n", modprefix);
	}
#endif

#if LINT
	{			/* for lex */
		extern int      yytchar, yylineno;
		extern FILE    *yyin;
		IGNORE(yyin);
		IGNORE(yytchar);
		IGNORE(yylineno);
		IGNORE(yyinput());
		yyunput(ilint);
		yyoutput(ilint);
	}
#endif
#if MAC
	printf("Done\n");
	SIOUXSettings.autocloseonquit = true;
#endif
	return 0;
}

static void openfiles(argc, argv)
	int             argc;
	char           *argv[];
{
	char            s[100];
	char *cp;

	if (argc > 1) {
		sprintf(modprefix, "%s", argv[1]);
		cp = strstr(modprefix, ".mod");
		if (cp) {
			*cp = '\0';
		}
	}
	if (argc == 2) {
		Sprintf(finname, "%s.mrg", modprefix);
	} else if (argc == 3) {
		Sprintf(finname, "%s", argv[2]);
	} else {
		diag("Usage:", "modl prefixto.mod [inputfile]");
	}
	if ((fin = fopen(finname, "r")) == (FILE *) 0) {
		Sprintf(finname, "%s.mod", modprefix);
		if ((fin = fopen(finname, "r")) == (FILE *) 0) {
			diag("Can't open input file: ", finname);
		}
	}
#if HMODL || NMODL
#else
	Sprintf(s, "%s.var", modprefix);
	if ((fparout = fopen(s, "w")) == (FILE *) 0) {
		diag("Can't create variable file: ", s);
	}
#endif
	Sprintf(s, "%s.c", modprefix);
	if ((fcout = fopen(s, "w")) == (FILE *) 0) {
		diag("Can't create C file: ", s);
	}
#if SIMSYS
	Sprintf(s, "%s.ctl", modprefix);
	if ((fctlout = fopen(s, "w")) == (FILE *) 0) {
		diag("Can't create variable file: ", s);
	}
	Sprintf(s, "%s.num", modprefix);
	if ((fnumout = fopen(s, "w")) == (FILE *) 0) {
		diag("Can't create C file: ", s);
	}
#endif
}
