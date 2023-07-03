#include <../../nmodlconf.h>
/* /local/src/master/nrn/src/modlunit/model.c,v 1.6 1998/07/12 13:19:02 hines Exp */

/*
 * int main(int argc, char *argv[]) --- returns 0 if translation is
 * successful. Diag will exit with 1 if error.
 *
 * ---The overall strategy of the translation consists of three phases.
 *
 * 1) read in the whole file as a sequence of tokens, building a parse tree.
 * The entire file can be printed exactly by printing the intoken list. No
 * translation is done here but the symbol table is constructed.
 * 2) Manipulate the blocks.
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

#include "model.h"
#include "parse1.hpp"

extern int yyparse();

FILE *fin,    /* input file descriptor for filename.mod */
              /* or file2 from the second argument */
    *fparout, /* output file descriptor for filename.var */
    *fcout;   /* output file descriptor for filename.c */

char finname[NRN_BUFSIZE]; /* filename.mod  or second argument */
Item* parseroot;
Item* title;

#if LINT
char* clint;
int ilint;
Item* qlint;
#endif

static const char* pgm_name = "model";
extern const char* RCS_version;
extern const char* RCS_date;
static void openfiles(int, char**);
static void debug_item(Item* q, int indent, FILE* file);

int main(int argc, char* argv[]) {
    /*
     * arg 1 is the prefix to the input file and output .c and .par
     * files
     * We first look for a .mrg file and then a .mod file
     */
    Fprintf(stderr, "%s   %s   %s\n", pgm_name, RCS_version, RCS_date);


    init(); /* keywords into symbol table, initialize
             * lists, etc. */
    unit_init();
    nrn_unit_init();
    openfiles(argc, argv); /* .mrg else .mod,  .var, .c */
    Fprintf(stderr, "Checking units of %s\n", finname);

    lex_start();
    /* declare all used variables */
    parsepass(1);
    IGNORE(yyparse());
    declare_implied();
    /* At this point The input file is in the intoken list */
#if 0
	printlist(intoken);
	debug_item((Item *)intoken, 0, fparout);
#endif
    /* give all names their proper units */
    /* all variables used consistently (arrays) */
    parsepass(2);
    yyparse();
    /*
     * NAME's can be used in many cases before they were declared and
     * no checking up to this point has been done to make sure that
     * names have been used in only one way.
     *
     */
    consistency();
    /* check unit consistency */
    parsepass(3);
    yyparse();
#if 0
	parout();		/* print .var file.
				 * Also #defines which used to be in defs.h
				 * are printed into .c file at beginning.
				 */
	cout();			/* print .c file */
#endif
#if 0
	IGNORE(fclose(fparout));
	IGNORE(fclose(fcout));
	memory_usage();
#endif
#if LINT
    { /* for lex */
        extern int yytchar, yylineno;
        extern FILE* yyin;
        IGNORE(yyin);
        IGNORE(yytchar);
        IGNORE(yylineno);
        IGNORE(yyinput());
        yyunput(ilint);
        yyoutput(ilint);
    }
#endif
    return 0;
}

static void openfiles(int argc, char* argv[]) {
    char *cp, modprefix[NRN_BUFSIZE - 5];
    if (argc > 1) {
        assert(strlen(argv[1]) < NRN_BUFSIZE);
        Sprintf(modprefix, "%s", argv[1]);
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
    if ((fin = fopen(finname, "r")) == (FILE*) 0) {
        Sprintf(finname, "%s.mod", modprefix);
        if ((fin = fopen(finname, "r")) == (FILE*) 0) {
            diag("Can't open input file: ", finname);
        }
    }
}

void printlist(List* list) {
    Item* q;

    ITERATE(q, list) {
        printitem(q, fcout);
    }
}

void printitems(Item* q1, Item* q2) {
    Item* q;

    for (q = q1; q->prev != q2; q = q->next) {
        printitem(q, stderr);
    }
}

void printitem(Item* q, FILE* fp) {
    switch (q->itemtype) {
    case SYMBOL:
        Fprintf(fp, "%s", SYM(q)->name);
        break;
    case STRING:
        Fprintf(fp, "%s", STR(q));
        break;
    case NEWLINE:
        Fprintf(fp, "\n");
        break;
    default:
        Fprintf(stderr, "\nq->itemtype = %d\n", q->itemtype);
        diag("printlist handles only a few types of items", (char*) 0);
        break;
    }
    fflush(fp);
}

void debugitem(Item* q) {
    debug_item(q, 0, stderr);
}

static void debug_item(Item* q, int indent, FILE* file) {
    int i;
    List* list;
    Item* q1;

    for (i = 0; i < indent; i++) {
        Fprintf(file, " ");
    }
    if (!q) {
        Fprintf(file, "NULL ITEM\n");
    } else
        switch (q->itemtype) {
        case SYMBOL:
            Fprintf(file, "SYMBOL |%s| %p\n", SYM(q)->name, SYM(q));
            break;
        case STRING:
            Fprintf(file, "STRING |%s|\n", STR(q));
            break;
        case LIST:
            Fprintf(file, "LIST\n");
            list = LST(q);
            ITERATE(q1, list) {
                debug_item(q1, indent + 2, file);
            }
            break;
        case 0:
            list = (List*) q;
            Fprintf(file, "HEAD/TAIL of list\n");
            ITERATE(q1, list) {
                debug_item(q1, indent, file);
            }
            break;
        case ITEM:
            Fprintf(file, "ITEM\n");
            debug_item(ITM(q), indent + 2, file);
            break;
        case ITEMARRAY: {
            Item** qa;
            int i;
            long n;
            qa = ITMA(q);
            n = (size_t) qa[-1];
            Fprintf(file, "ITEMARRAY %ld\n", n);
            for (i = 0; i < n; i++) {
                debug_item(qa[i], indent + 2, file);
            }
        } break;
        case NEWLINE:
            Fprintf(file, "NEWLINE %d\n", q->itemsubtype);
            break;
        default:
            Fprintf(stderr, "\nq->itemtype = %d\n", q->itemtype);
            diag("unknown itemtype", (char*) 0);
            break;
        }
    fflush(file);
}

/* model.c,v
 * Revision 1.6  1998/07/12  13:19:02  hines
 * error when no args to modelunit fixed
 *
 * Revision 1.5  1997/12/01  14:51:39  hines
 * mac port to codewarrior pro2 more complete
 *
 * Revision 1.4  1997/11/28  14:57:52  hines
 * more changes for port to mac of modlunit
 *
 * Revision 1.3  1997/11/24  16:19:12  hines
 * modlunit port to mac (not complete)
 *
 * Revision 1.2  1997/10/20  14:58:07  hines
 * modlunit file.mod accepted (ie suffix allowed)
 *
 * Revision 1.1.1.1  1994/10/12  17:22:49  hines
 * NEURON 3.0 distribution
 *
 * Revision 1.6  1994/05/18  18:08:13  hines
 * INCLUDE "file"
 * tries originalpath/file ./file MODL_INCLUDEpaths/file
 *
 * Revision 1.5  1993/02/01  15:15:48  hines
 * static functions should be declared before use
 *
 * Revision 1.4  91/02/09  16:39:35  hines
 * special neuron variables checked for correct units.
 *
 * Revision 1.3  91/01/07  14:17:10  hines
 * in kinunit, wrong itemsubtype.  Fix lint messages
 *
 * Revision 1.2  90/11/16  07:53:34  hines
 * take out the .c and .var file
 *
 * Revision 1.1  90/11/13  16:10:21  hines
 * Initial revision
 *  */
