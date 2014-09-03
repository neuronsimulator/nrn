
#include <stdio.h>
#if HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <assert.h>

/*-
  The central data structure throughout modl is the list. Items can be
  inserted before a known item, inserted at the head of a list, or appended
  to the tail of a list. Items can be removed from a list. Lists can contain
  mixed types of items. Often an item which was an input token is edited and
  replaced by a string. The main types of items are strings and symbols. 
  Lists used are:
  
  intoken	Everytime the lexical analyser reads an input token it is
  		placed in this list. Much translation to C takes place during
  		parsing and most of that editing is done in this list. After
  		an entire block is processed, the tokens are moved as a group
  		to another list.
  		
  initfunc	main body of initmodel() from INITIAL. Automatic statements
  		of the form state = state0 are constructed here.
  constructorfunc	statements added to tail of allocation function
  destructorfunc	statements executed when POINT_PROCESS destroyed
  termfunc	main body of terminal() from TERMINAL
  modelfunc	main body of model() from EQUATION. SOLVE statements in the
  		equation block get expanded in this list
  procfunc	all remaining blocks get concatenated to this list. It
  		also gets the declarations. By prepending to procfunc, one
  		can guarantee that a declaration precedes usage.
  initlist	automatically generated statements that are to be executed
  		only once are constructed here. Contains setup of slist's
  		and dist's, etc. i.e. setup of indirect pointer lists to
  		state groups.
  
 Other lists (not global) but used in several files are:
 
  symlist[]	symbol table. One list for every beginning ascii character.
  		see symbol.c and symbol.h. See the Symbol structure below.
  symlistlist	for a stack of local symbol lists for LOCAL variables.
  		Local variable 'name' is  translated as _lname so that
  		there can be no conflict with the 'p' array.
  syminorder	Maintains order of variable declarations in the input file.
  		Used to get good order in the .var file.
  		
  firstlist	Statements that must appear before anything else in the
  		.c file. The only usage at this time is declaration of
  		LOCAL variables outside of blocks.
  		
  plotlist	variables to be plotted on first entry to scop
  
 Other lists used for local or very specific purposes are:
 
  indeplist	Independent variable info.
  parmlist	parameters in a SENS statement
  statelist	states used in a block containing a SENS statement
  sensinfo	see sens.c
  senstmt	see sens.c
  solvq		Item location and method stored when SOLVE statement occurs.
  		Solve statements are processed after all input is read.
  eqnq		Linear equations assembled using this list.
  
 Kinetic.c should be modified to make uniform use of List's instead of
 special structures for Reaction, Rlist, etc.
 */

/* For char buffers that might be called on to hold long path names */
/* Note that paths can exceed MAX_PATH from <limits.h> on some systems */
#define NRN_BUFSIZE 8192
#include <limits.h>
#if MAX_PATH > NRN_BUFSIZE
#undef NRN_BUFSIZE
#define NRN_BUFSIZE MAX_PATH
#endif

typedef struct Item    List;		/* list of mixed items */
typedef struct Item {
	short           itemtype;
	union	{
			struct Item *itm;
			List *lst;
			char *str;
			struct Symbol *sym;
	}	element;	/* pointer to the actual item */
	struct Item    *next;
	struct Item    *prev;
}               Item;
#define ITEM0	(Item *)0
#define LIST0	(List *)0

#define ITERATE(itm,lst) for (itm = (lst)->next; itm != (lst); itm = itm->next)

/*-
The symbol structure gives info about tokens. Not all tokens need all
elements. Eg. the STRING uses only type and name.  Much storage could be
saved and much greater clarity could be attained if each type had its own
sub stucture.  Currently many of the structure elements serve very different
purposes depending on the type.
The following is a list of the current element usage:
 type		token number from parse1.y
 subtype	see definitions below
 u.i		integration method - flag for variable step
 		equation block - function number for generating variables
 u.str		scop variables - max,min,units for .var file
 used		state variable - temporary flag that it is used in an equation
 		equation block - number of state variables used (# unknowns)
		in parout.c - the numeric order in the .var file. Generated
		  and used in parout.c for the plotlist.
 usage		a token is used as a variable (DEP) or function (FUNC)
		Another field, EXPLICIT_DECL, is used to determine if a
		variable appears in the input file or is automatically
		created, thus helping to organize the .var file.
 araydim	arrays - dimension
 discdim	discrete variable - dimension
 varnum		state variable - during processing of a block containing
 		 equations in which simultaneous equations result; column
 		 number of state variable in the matrix.
 level		lowest submodel level number for declarations of this
 		 symbol. Used for parameters ( in explicit_decl()).
 		 The default value is 100.
 name		token name
*/
typedef struct Symbol {
	short           type;
	long            subtype;
#if NMODL
	short		nrntype;
	short		assigned_to_;
	int		no_threadargs; /* introduced for FUNCTION_TABLE table_name */
#if CVODE
	int*		slist_info_; /* blunt instrument for retrieving ion concentration slist value */
#endif
	int		ioncount_; /* ppvar index for ions */
#endif
	union {
		int             i;
		char           *str;
	}               u;
	int             used;
	int             usage;
	int             araydim;
	int		discdim;
	int             varnum;	/* column number of state variable in
				 * equations */
	short		level;
	char           *name;
}               Symbol;
#define SYM0	(Symbol *)0

/*
 * this is convenient way to get the element pointer if you know what type
 * the item is 
 */
#define SYM(q)	((q)->element.sym)
#define STR(q)	((q)->element.str)
#define ITM(q)	((q)->element.itm)
#define LST(q)	((q)->element.lst)
/* types not defined in parser */
#define	SPECIAL 1
#define SYMBOL	1
#define ITEM	2
#define LIST	3
/*
 * An item type, STRING is also used as an item type 
 * An item type, VERBATIM is also used as an item type which is to be
 *  treated the same as a STRING but with no prepended space on output.
 */

/* subtypes */
#define	KEYWORD 01
#define PARM	02
#define	INDEP	04
#define	DEP	010		/* also in usage field */
#define	STAT	020
#define	ARRAY	040
#define	FUNCT	0100		/* also in usage field */
#define PROCED	0200
#define	NEGATIVE 0400
#define SEMI	01		/* ";" */
#define BEGINBLK 02		/* "{" */
#define ENDBLK	04		/* "}" */
#define DERF	01000
#define KINF	02000
#define NLINF	04000
#define DISCF	010000
#define STEP1   020000
#define PARF	040000
#define EXTDEF	0100000
#define LINF	0200000
#define UNITDEF 0400000L
#define EXTDEF2 01000000L	/* functions that can take array or function name arguments */
#define nmodlCONST 02000000L	/* constants that do not appear in .var file */
#define EXTDEF3 04000000L	/* get two extra reset arguments at beginning */
#define INTGER	010000000L	/* must be cast to double in expr */
#define EXTDEF4 020000000L	/* get extra NrnThread* arg at beginning */
#define EXTDEF5 040000000L	/* not threadsafe from the extdef list */
#define EXPLICIT_DECL 01	/* usage field, variable occurs in input file */

extern char
		*emalloc(),	/* malloc with out of space checking */
		*stralloc(),	/* copies string to new space */
		*inputline(),	/* used only by parser to get title line */
		*inputtopar(),	/* used only by parser to get units */
		*Gets();	/* used only in io.c to get string from fin. */

#if 0
#if __TURBOC__ || SYSV || NeXT || LINUX
#else
extern char    *sprintf();
#endif
#endif

extern List
		*newlist(),	/* begins new empty list */
		*inputtext();	/* used by parser to get block text from
				 * VERBATIM and COMMENT */
extern Item
		*putintoken(),	/* construct symbol and store input tokens */
		*insertstr(),	/* before a known Item */
		*insertsym(),
		*linsertstr(),	/* prepend to list */
		*lappendstr(),	/* append to list */
		*linsertsym(),
		*lappendsym(),
		*lappenditem(),
		*lappendlst(),
		*next(),	/* not used but should be instead of q->next */
		*prev();

extern Symbol
		*install(),	/* Install token in symbol table */
		*lookup(),	/* lookup name in symbol table */
		*copylocal(),	/* install LOCAL variable symbol */
		*ifnew_parminstall();	/* new .var info only if
					 * not already done. */
#include "nmodlfunc.h"

extern char     finname[],	/* the input file prefix */
                buf[];		/* general purpose temporary buffer */

extern List
		*intoken,	/* Main list of input tokens */
		*initfunc,	/* see discussion above */
		*constructorfunc, 
		*destructorfunc, 
		*modelfunc,
		*termfunc,
		*procfunc,
		*initlist,
		*firstlist,
		*plotlist;

extern FILE
		*fin,		/* .mod input file descriptor */
		*fparout,	/* .var file */
		*fcout;		/* .c file */

extern Symbol
		*semi,		/* ';'. When seen on output, causes newline */
		*beginblk,	/* '{'. Used for rudimentary indentation */
		*endblk;	/* on output. */

extern void printlist(List*);

/* the following is to get lint to shut up */
#if LINT
#undef assert
#define assert(arg)	{if (arg) ;}	/* so fprintf doesn't give lint */
extern char    *clint;
extern int      ilint;
extern Item    *qlint;
#define Sprintf		clint = sprintf
#define Fprintf		ilint = fprintf
#define Fclose		ilint = fclose
#define Fflush		ilint = fflush
#define Printf		ilint = printf
#define Strcpy		clint = strcpy
#define Strcat		clint = strcat
#define Insertstr	qlint = insertstr
#define Insertsym	qlint = insertsym
#define Linsertsym	qlint = linsertsym
#define Linsertstr	qlint = linsertstr
#define Lappendsym	qlint = lappendsym
#define Lappendstr	qlint = lappendstr
#define Lappenditem	qlint = lappenditem
#define Lappendlst	qlint = lappendlst
#define IGNORE(arg)	{if (arg);}
#define Free(arg)	free((char *)(arg))
#else
#define Sprintf		sprintf
#define Fprintf		fprintf
#define Fclose		fclose
#define Fflush		fflush
#define Printf		printf
#define Strcpy		strcpy
#define Strcat		strcat
#define Insertstr	insertstr
#define Insertsym	insertsym
#define Linsertsym	linsertsym
#define Linsertstr	linsertstr
#define Lappendsym	lappendsym
#define Lappendstr	lappendstr
#define Lappenditem	lappenditem
#define Lappendlst	lappendlst
#define IGNORE(arg)	arg
#define Free(arg)	free((void *)(arg))
#endif
