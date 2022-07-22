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

#include <getopt.h>

#if MAC
#include <sioux.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include "modl.h"
FILE *fin,    /* input file descriptor for filename.mod */
              /* or file2 from the second argument */
    *fparout, /* output file descriptor for filename.var */
    *fcout;   /* output file descriptor for filename.c */
#if SIMSYS
FILE *fctlout, /* filename.ctl */
    *fnumout;  /* filename.num */
#endif


char* modprefix;

char* finname;

#if LINT
char* clint;
int ilint;
Item* qlint;
#endif

int nmodl_text = 1;
List* filetxtlist;

extern int yyparse();
extern int mkdir_p(const char*);

#if NMODL && VECTORIZE
extern int vectorize;
extern int numlist;
extern char* nmodl_version_;
extern int usederivstatearray;
#endif

/*SUPPRESS 763*/
static char pgm_name[] = "nmodl";
extern char* RCS_version;
extern char* RCS_date;

static struct option long_options[] = {{"version", no_argument, 0, 'v'},
                                       {"help", no_argument, 0, 'h'},
                                       {"outdir", required_argument, 0, 'o'},
                                       {0, 0, 0, 0}};

static void show_options(char** argv) {
    fprintf(stderr, "Source to source compiler from NMODL to C++\n");
    fprintf(stderr, "Usage: %s [options] Inputfile\n", argv[0]);
    fprintf(stderr, "Options:\n");
    fprintf(stderr,
            "\t-o | --outdir <OUTPUT_DIRECTORY>    directory where output files will be written\n");
    fprintf(stderr, "\t-h | --help                         print this message\n");
    fprintf(stderr, "\t-v | --version                      print version number\n");
}

static void openfiles(char* given_filename, char* output_dir);

int main(int argc, char** argv) {
    int option = -1;
    int option_index = 0;
    char* output_dir = NULL;

    if (argc < 2) {
        show_options(argv);
        exit(1);
    }

    while ((option = getopt_long(argc, argv, ":vho:", long_options, &option_index)) != -1) {
        switch (option) {
        case 'v':
            printf("%s\n", nmodl_version_);
            exit(0);

        case 'o':
            output_dir = strdup(optarg);
            break;

        case 'h':
            show_options(argv);
            exit(0);

        case ':':
            fprintf(stderr, "%s: option '-%c' requires an argument\n", argv[0], optopt);
            exit(-1);

        case '?':
        default:
            fprintf(stderr, "%s: invalid option `-%c' \n", argv[0], optopt);
            exit(-1);
        }
    }
    if ((argc - optind) > 1) {
        fprintf(stderr,
                "%s: Warning several input files specified on command line but only one will be "
                "processed\n",
                argv[0]);
    }

    filetxtlist = newlist();

#if MAC
    SIOUXSettings.asktosaveonclose = false;
#if !SIMSYS
    Fprintf(stderr, "%s   %s   %s\n", pgm_name, RCS_version, RCS_date);
#endif
#endif

    init(); /* keywords into symbol table, initialize
             * lists, etc. */

    finname = argv[optind];

    openfiles(finname, output_dir); /* .mrg else .mod,  .var, .c */
#if NMODL || HMODL
#else
#if !SIMSYS
    Fprintf(stderr, "Translating %s into %s.cpp and %s.var\n", finname, modprefix, modprefix);
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
    netrec_discon();
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
    chk_global_state();
    parout(); /* print .var file.
               * Also #defines which used to be in defs.h
               * are printed into .c file at beginning.
               */
    c_out();  /* print .c file */
#if HMODL || NMODL
#else
    IGNORE(fclose(fparout));
#endif
#if SIMSYS
    IGNORE(fclose(fctlout));
    IGNORE(fclose(fnumout));
#endif

#if !defined NMODL_TEXT
#define NMODL_TEXT 1
#endif
#if NMODL && NMODL_TEXT
#if 0
/* test: temp.txt should be identical to text of input file except for INCLUDE */
{
	FILE* f = fopen("temp.txt", "w");
	assert(f);
	Item* q;
	ITERATE(q, filetxtlist) {
		char* s = STR(q);
		fprintf(f, "%s", s);
	}
	fclose(f);
}
#endif
    if (nmodl_text) {
        Item* q;
        char* pf{nullptr};
#if HAVE_REALPATH && !defined(NRN_AVOID_ABSOLUTE_PATHS)
        pf = realpath(finname, nullptr);
#endif
        fprintf(
            fcout,
            "\n#if NMODL_TEXT\nstatic void register_nmodl_text_and_filename(int mech_type) {\n");
        fprintf(fcout, "    const char* nmodl_filename = \"%s\";\n", pf ? pf : finname);
        if (pf) {
            free(pf);
        }
        fprintf(fcout, "    const char* nmodl_file_text = \n");
        ITERATE(q, filetxtlist) {
            char* s = STR(q);
            char* cp;
            fprintf(fcout, "  \"");
            /* Escape double quote, backslash, and end each line with \n */
            for (cp = s; *cp; ++cp) {
                if (*cp == '"' || *cp == '\\') {
                    fprintf(fcout, "\\");
                }
                if (*cp == '\n') {
                    fprintf(fcout, "\\n\"");
                }
                fputc(*cp, fcout);
            }
        }
        fprintf(fcout, "  ;\n");
        fprintf(fcout, "    hoc_reg_nmodl_filename(mech_type, nmodl_filename);\n");
        fprintf(fcout, "    hoc_reg_nmodl_text(mech_type, nmodl_file_text);\n");
        fprintf(fcout, "}\n");
        fprintf(fcout, "#endif\n");
    }
#endif

    IGNORE(fclose(fcout));

#if NMODL && VECTORIZE
    if (vectorize) {
        Fprintf(stderr, "Thread Safe\n");
    }
    if (usederivstatearray) {
        fprintf(stderr,
                "Derivatives of STATE array variables are not translated correctly and compile "
                "time errors will be generated.\n");
        fprintf(stderr, "The %s.cpp file may be manually edited to fix these errors.\n", modprefix);
    }
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
#if MAC
    printf("Done\n");
    SIOUXSettings.autocloseonquit = true;
#endif
    free(modprefix); /* allocated in openfiles below */
    return 0;
}

static void openfiles(char* given_filename, char* output_dir) {
    char s[NRN_BUFSIZE];

    char output_filename[NRN_BUFSIZE];
    char input_filename[NRN_BUFSIZE];
    modprefix = strdup(given_filename);  // we want to keep original string to open input file
    // find last '.' after last '/' that delimit file name from extension
    // we are not bothering to deal with filenames that begin with a . but do
    // want to deal with paths like ../foo/hh
    char* first_ext_char = strrchr(modprefix, '.');
    if (strrchr(modprefix, '/') > first_ext_char) {
        first_ext_char = NULL;
    }

    Sprintf(input_filename, "%s", given_filename);

    if (first_ext_char)
        *first_ext_char = '\0';  // effectively cut the extension from prefix if it exist in
                                 // given_filename
    if ((fin = fopen(input_filename, "r")) == (FILE*) 0) {  // first try to open given_filename
        Sprintf(input_filename, "%s.mod", given_filename);  // if it dont work try to add ".mod"
                                                            // extension and retry
        Sprintf(finname, "%s.mod", given_filename);  // finname is still a global variable, so we
                                                     // need to update it
        if ((fin = fopen(input_filename, "r")) == (FILE*) 0) {
            diag("Can't open input file: ", input_filename);
        }
    }
    if (output_dir) {
        if (mkdir_p(output_dir) != 0) {
            fprintf(stderr, "Can't create output directory %s\n", output_dir);
            exit(1);
        }
        char* basename = strrchr(modprefix, '/');
        if (basename) {
            Sprintf(output_filename, "%s%s.cpp", output_dir, basename);
        } else {
            Sprintf(output_filename, "%s/%s.cpp", output_dir, modprefix);
        }
    } else {
        Sprintf(output_filename, "%s.cpp", modprefix);
    }

    if ((fcout = fopen(output_filename, "w")) == (FILE*) 0) {
        diag("Can't create C file: ", output_filename);
    }
    Fprintf(stderr, "Translating %s into %s\n", input_filename, output_filename);

#if HMODL || NMODL
#else
    Sprintf(s, "%s.var", modprefix);
    if ((fparout = fopen(s, "w")) == (FILE*) 0) {
        diag("Can't create variable file: ", s);
    }
#endif
#if SIMSYS
    Sprintf(s, "%s.ctl", modprefix);
    if ((fctlout = fopen(s, "w")) == (FILE*) 0) {
        diag("Can't create variable file: ", s);
    }
    Sprintf(s, "%s.num", modprefix);
    if ((fnumout = fopen(s, "w")) == (FILE*) 0) {
        diag("Can't create C file: ", s);
    }
#endif
}

// Post-adjustments for VERBATIM blocks  (i.e  make them compatible with CPP).
void verbatim_adjust(char* q) {
    Fprintf(fcout, "%s", q);
}
