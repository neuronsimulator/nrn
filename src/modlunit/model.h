/* /local/src/master/nrn/src/modlunit/model.h,v 1.2 1997/11/24 16:19:13 hines Exp */
#include "wrap_sprintf.h"
#include <stdio.h>
#if 1
#if defined(STDC_HEADERS) || defined(SYSV)
#include <string.h>
#else
#include <strings.h>
#endif
#endif
#include <assert.h>

#define NRN_BUFSIZE 8192

typedef struct Item {
    short itemtype;
    short itemsubtype;
    void* element; /* pointer to the actual item */
    struct Item* next;
    struct Item* prev;
} Item;
#define ITEM0 (Item*) 0

typedef Item List; /* list of mixed items */
#define ITERATE(itm, lst) for (itm = (lst)->next; itm != (lst); itm = itm->next)

/*-
The symbol structure gives info about tokens. Not all tokens need all
elements. Eg. the STRING uses only type and name.  Much storage could be
saved and much greater clarity could be attained if each type had its own
sub stucture.  Currently many of the structure elements serve very different
purposes depending on the type.
The following is a list of the current element usage:
 type		token number from parse1.ypp
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
 name		token name
*/
typedef struct Symbol {
    short type;
    long subtype;
    Item* info;
    union {
        int i;
        const char* str;
    } u;
    int used;
    int usage;
    int araydim;
    int discdim;
    int varnum; /* column number of state variable in
                 * equations */
    char* name;
} Symbol;
#define SYM0 (Symbol*) 0

/*
 * this is convenient way to get the element pointer if you know what type
 * the item is
 */
#if DEBUG || 1
extern Symbol* _SYM(Item*, char*, int);
extern char* _STR(Item* q, char* file, int line);
extern Item* _ITM(Item* q, char* file, int line);
extern Item** _ITMA(Item* q, char* file, int line); /* array of item pointers */
extern List* _LST(Item* q, char* file, int line);
#define SYM(q)  _SYM(q, (char*) __FILE__, __LINE__)
#define STR(q)  _STR(q, (char*) __FILE__, __LINE__)
#define ITM(q)  _ITM(q, (char*) __FILE__, __LINE__)
#define ITMA(q) _ITMA(q, (char*) __FILE__, __LINE__)
#define LST(q)  _LST(q, (char*) __FILE__, __LINE__)
#else
#define SYM(q)  ((Symbol*) ((q)->element))
#define STR(q)  ((char*) ((q)->element))
#define ITM(q)  ((Item*) ((q)->element))
#define ITMA(q) ((Item**) ((q)->element))
#define LST(q)  ((List*) ((q)->element))
#endif

/* types not defined in parser */
#define SPECIAL 1
/* itemtype of 0 is used by list implementation */
#define SYMBOL    1
#define ITEM      2
#define LIST      3
#define ITEMARRAY 4
/*
 * An item type, STRING is also used as an item type
 * An item type, VERBATIM is also used as an item type which is to be
 *  treated the same as a STRING but with no prepended space on output.
 */

/* subtypes */
#define KEYWORD       01L
#define modlunitCONST 02L
#define INDEP         04L
#define DEP           010L /* also in usage field */
#define STAT          020L
#define ARRAY         040L
#define FUNCT         0100L /* also in usage field */
#define PROCED        0200L
#define NEGATIVE      0400L
#define SEMI          01L /* ";" */
#define BEGINBLK      02L /* "{" */
#define ENDBLK        04L /* "}" */
#define DERF          01000L
#define LINF          02000L
#define NLINF         04000L
#define DISCF         010000L
#define PARF          040000L
#define EXTDEF        0100000L
#define KINF          0200000L
#define LOCL          0400000L
#define CNVFAC        01000000L
#define UFACTOR       02000000L
#define RANGEOBJ      04000000L

#define EXPLICIT_DECL 01 /* usage field, variable occurs in input file */

extern char* emalloc(unsigned);            /* malloc with out of space checking */
extern char* stralloc(const char*, char*); /* copies string to new space */

extern char *inputline(), /* used only by parser to get title line */
    *inputtopar(),        /* used only by parser to get units */
    *Gets(char*);         /* used only in io.c to get string from fin. */
const char* unit_str();
extern const char* decode_units(Symbol*);

extern List *makelist(int narg, ...),
    *itemarray(int narg, ...), /* item  ITEMARRAY, array of item pointers */
    *prepend(), *newlist(),    /* begins new empty list */
    *inputtext();              /* used by parser to get block text from
                                * VERBATIM and COMMENT */
extern Item *putintoken(const char*s, short type, short), /* construct symbol and store input tokens
                                                           */
    *insertstr(Item*item, const char*str),                /* before a known Item */
    *insertsym(List*list, Symbol*sym), *linsertstr(List*list, char*str), /* prepend to list */
    *lappendstr(List*list, const char*str),                              /* append to list */
    *linsertsym(List*list, Symbol*sym), *lappendsym(List*list, Symbol*sym),
    *lappenditem(List*list, Item*item), *listtype(), *next_parstok(Item*), *prev_parstok(Item*),
    *car(List*), *next(Item*), *prev(Item*);


#include "modlunit.h" /* void functions */

extern Symbol *install(const char*, int), /* Install token in symbol table */
    *lookup(const char*),                 /* lookup name in symbol table */
    *ifnew_constinstall();                /* new .var info only if
                                           * not already done. */

extern int unitonflag;

extern char finname[NRN_BUFSIZE], /* the input file prefix */
    buf[512];                     /* general purpose temporary buffer */

extern Item *parseroot, *lex_tok; /* intoken pointer for nonzero parse passes */

extern List *intoken, /* Main list of input tokens */
    *initfunc,        /* see discussion above */
    *modelfunc, *termfunc, *procfunc, *initlist, *firstlist, *plotlist,
    *misc; /* place to stick isolated items */

extern FILE *fin, /* .mod input file descriptor */
    *fparout,     /* .var file */
    *fcout;       /* .c file */

extern Symbol *indepsym, /* The model independent variable */
    *semi,               /* ';'. When seen on output, causes newline */
    *beginblk,           /* '{'. Used for rudimentary indentation */
    *endblk;             /* on output. */


/* the following is to get lint to shut up */
#if LINT
#undef assert
#define assert(arg) \
    {               \
        if (arg)    \
            ;       \
    } /* so fprintf doesn't give lint */
extern char* clint;
extern int ilint;
extern Item* qlint;
#define Fprintf     ilint = fprintf
#define Fclose      ilint = fclose
#define Fflush      ilint = fflush
#define Printf      ilint = printf
#define Strcpy      clint = strcpy
#define Strcat      clint = strcat
#define Insertstr   qlint = insertstr
#define Insertsym   qlint = insertsym
#define Linsertsym  qlint = linsertsym
#define Linsertstr  qlint = linsertstr
#define Lappendsym  qlint = lappendsym
#define Lappendstr  qlint = lappendstr
#define Lappenditem qlint = lappenditem
#define IGNORE(arg) \
    {               \
        if (arg)    \
            ;       \
    }
#else
#define Fprintf     fprintf
#define Fclose      fclose
#define Fflush      fflush
#define Printf      printf
#define Strcpy      strcpy
#define Strcat      strcat
#define Insertstr   insertstr
#define Insertsym   insertsym
#define Linsertsym  linsertsym
#define Linsertstr  linsertstr
#define Lappendsym  lappendsym
#define Lappendstr  lappendstr
#define Lappenditem lappenditem
#define IGNORE(arg) arg
#endif
using neuron::Sprintf;

/* model.h,v
 * Revision 1.2  1997/11/24  16:19:13  hines
 * modlunit port to MAC (not complete)
 *
 * Revision 1.1.1.1  1994/10/12  17:22:45  hines
 * NEURON 3.0 distribution
 *
 * Revision 1.5  1993/11/04  15:52:23  hines
 * port to solaris2 (no more warnings)
 *
 * Revision 1.4  1991/08/13  10:05:08  hines
 * to work on rs6000
 *
 * Revision 1.3  90/11/20  15:33:05  hines
 * added 4 varieties of unit factors. They are
 * name = (real)
 * name = ((unit) -> (unit))	must be conformable
 * name = (physical_constant)
 * name = (physical_constant (unit))	must be conformable
 *
 * Revision 1.2  90/11/15  13:01:17  hines
 * function units and number units work. accepts NEURON block
 *
 * Revision 1.1  90/11/13  16:12:00  hines
 * Initial revision
 *  */
