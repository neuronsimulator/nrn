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

/* the first arg may also be a file.mod (containing the .mod suffix)*/

#include <CLI/CLI.hpp>

#include <stdlib.h>
#include "modl.h"

#include <cstring>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

FILE *fin,    /* input file descriptor for filename.mod */
              /* or file2 from the second argument */
    *fparout, /* output file descriptor for filename.var */
    *fcout;   /* output file descriptor for filename.c */


char* modprefix;

char finname[NRN_BUFSIZE];

#if LINT
char* clint;
int ilint;
Item* qlint;
#endif

int nmodl_text = 1;
List* filetxtlist;

extern int yyparse();

extern int vectorize;
extern int numlist;
extern const char* nmodl_version_;
extern int usederivstatearray;

/*SUPPRESS 763*/
static const char* pgm_name = "nmodl";
extern const char* RCS_version;
extern const char* RCS_date;

static void openfiles(const char* given_filename, const char* output_dir);

int main(int argc, char** argv) {
    std::string output_dir{};
    std::string inputfile{};

    CLI::App app{"Source to source compiler from NMODL to C++"};
    app.add_option("-o,--outdir", output_dir, "directory where output files will be written");
    app.set_version_flag("-v,--version", nmodl_version_, "print version number");
    app.set_help_flag("-h,--help", "print this message");
    app.add_option("Inputfile", inputfile)->required();
    app.allow_extras();

    CLI11_PARSE(app, argc, argv);

    if (!app.remaining().empty()) {
        fprintf(stderr,
                "%s: Warning several input files specified on command line but only one will be "
                "processed\n",
                argv[0]);
    }

    filetxtlist = newlist();

    init(); /* keywords into symbol table, initialize
             * lists, etc. */

    std::strcpy(finname, inputfile.c_str());
    openfiles(inputfile.c_str(),
              output_dir.empty() ? nullptr : output_dir.c_str()); /* .mrg else .mod,  .var, .c */
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
    chk_thread_safe();
    chk_global_state();
    check_useion_variables();

    parout(); /* print .var file.
               * Also #defines which used to be in defs.h
               * are printed into .c file at beginning.
               */
    c_out();  /* print .c file */

#if !defined NMODL_TEXT
#define NMODL_TEXT 1
#endif
#if NMODL_TEXT
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

    if (vectorize) {
        Fprintf(stderr, "Thread Safe\n");
    }
    if (usederivstatearray) {
        fprintf(stderr,
                "Derivatives of STATE array variables are not translated correctly and compile "
                "time errors will be generated.\n");
        fprintf(stderr, "The %s.cpp file may be manually edited to fix these errors.\n", modprefix);
    }

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
    free(modprefix); /* allocated in openfiles below */
    return 0;
}

static void openfiles(const char* given_filename, const char* output_dir) {
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
        try {
            fs::create_directories(output_dir);
        } catch (...) {
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
}

// Post-adjustments for VERBATIM blocks  (i.e  make them compatible with CPP).
void verbatim_adjust(char* q) {
    Fprintf(fcout, "%s", q);
}
