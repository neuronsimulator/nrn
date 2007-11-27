#include <../../nmodlconf.h>
/* /local/src/master/nrn/src/nmodl/modl.c,v 4.5 1998/01/22 18:50:33 hines Exp */
/*
modl.c,v
 * Revision 4.5  1998/01/22  18:50:33  hines
 * allow stralloc with null string (creates empty string)
 *
 * Revision 4.4  1997/12/01  14:51:44  hines
 * mac port to codewarrior pro2 more complete
 *
 * Revision 4.3  1997/11/26  14:04:49  hines
 * port of nmodl to MAC
 *
 * Revision 4.2  1997/10/20  14:59:32  hines
 * nocmodl file.mod allowed (ie suffix is allowed)
 *
 * Revision 4.1  1997/08/30  20:45:26  hines
 * cvs problem with branches. Latest nmodl stuff should now be a top level
 *
 * Revision 4.0.1.1  1997/08/08  17:23:51  hines
 * nocmodl version 4.0.1
 *
 * Revision 4.0  1997/08/08  17:06:17  hines
 * proper nocmodl version number
 *
 * Revision 1.1.1.1  1994/10/12  17:21:36  hines
 * NEURON 3.0 distribution
 *
 * Revision 9.186  1994/08/04  20:03:55  hines
 * forgot to remove 1 && from kinetic vectorize testbed
 *
 * Revision 9.182  1994/07/21  13:11:16  hines
 * allow vectorization of kinetic blocks on cray when using sparse method.
 * not tested yet
 *
 * Revision 9.180  1994/05/18  18:11:56  hines
 * INCLUDE "file"
 * tries originalpath/file ./file MODL_INCLUDEpaths/file
 *
 * Revision 9.157  1993/02/01  15:17:47  hines
 * static functions should be declared before use.
 * inline is keyword for some compilers.
 *
 * Revision 9.142  92/08/05  16:23:09  hines
 * can vectorize hh. need work on tables though.
 * 
 * Revision 9.128  92/02/05  14:47:50  hines
 * saber warning free
 * FUNCTION made global for use by other models. #define GLOBFUNC 1
 * 
 * Revision 9.76  90/12/07  09:27:16  hines
 * new list structure that uses unions instead of void *element
 * 
 * Revision 9.73  90/12/04  12:00:13  hines
 * model version displayed only as comment in generated c file
 * format of plot lines for scalar in .var file is
 * name nl vindex pindex 1 nl
 * for vector with specific index:
 * name[index] vindex pindex 1
 * for vector without index
 * name[size] vindex pindex size
 * 
 * Revision 9.31  90/10/08  11:34:00  hines
 * simsys prototype
 * 
 * Revision 9.22  90/08/17  08:14:58  hines
 * HMODL change define from HOC to HMODL, ensure all .c files compiled
 * 
 * Revision 9.11  90/07/30  11:51:10  hines
 * NMODL getting better, almost done
 * 
 * Revision 9.10  90/07/27  13:58:04  hines
 * nmodl handles declarations about right for first pass at this.
 * 
 * Revision 9.8  90/07/26  07:58:38  hines
 * interface modl to hoc and call the translator hmodl
 * 
 * Revision 3.73  90/07/19  08:29:50  hines
 * doesn't produce .var file
 * 
 * Revision 8.3  90/02/15  10:08:51  mlh
 * calls to parout and cout reversed so that parout can put defs.h defines
 * at beginning
 * 
 * Revision 8.2  90/02/15  08:56:18  mlh
 * first check for .mrg file and translate if it exists
 * if not the check for .mor file and translate that.
 * This is done if there is only one argument.
 * If two arguments then use first as basename and
 * second as input file
 * print message about what it is doing
 * 
 * Revision 8.1  89/11/01  10:17:40  mlh
 * consist moved after solvehandler to avoid warning that delta_indep is
 * undeclared.
 * 
 * Revision 8.0  89/09/22  17:26:34  nfh
 * Freezing
 * 
 * Revision 7.0  89/08/30  13:32:04  nfh
 * Rev 7 is now Experimental; Rev 6 is Testing
 * 
 * Revision 6.0  89/08/14  16:26:49  nfh
 * Rev 6.0 is latest of 4.x; now the Experimental version
 * 
 * Revision 4.0  89/07/24  17:03:16  nfh
 * Freezing rev 3.  Rev 4 is now Experimental
 * 
 * Revision 3.2  89/07/12  16:33:08  mlh
 * 2nd optional argument gives input file.  First arg is prefix to .var
 * and .c files (and .mod input file if 2nd arg not present).
 * 
 * Revision 3.1  89/07/07  16:54:23  mlh
 * FIRST LAST START in independent SWEEP higher order derivatives
 * 
 * Revision 1.2  89/07/06  15:34:50  mlh
 * inteface with version.c
 * 
 * Revision 1.1  89/07/06  14:49:52  mlh
 * Initial revision
 * 
*/

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


char		*modprefix, prefix_[200];	/* the first argument */

char            finname[200];	/* filename.mod  or second argument */

#if LINT
char           *clint;
int             ilint;
Item           *qlint;
#endif

#if NMODL && VECTORIZE
extern int vectorize;
extern int numlist;
extern char* nmodl_version_;
#endif

/*SUPPRESS 763*/
static char pgm_name[] =	"nmodl";
extern char *RCS_version;
extern char *RCS_date;
static openfiles();

int
main(argc, argv)
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
	char cs[256], *cp;
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
#if !_CRAY && NMODL && VECTORIZE
/* allowing Kinetic models to be vectorized on cray. So nonzero numlist is
no longer adequate for saying we can not */
	if (numlist) {
		vectorize = 0;
	}
#endif
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
		Fprintf(stderr, "VECTORIZED\n");
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

static          openfiles(argc, argv)
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
