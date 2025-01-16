#ifndef __INTEL_LLVM_COMPILER
#pragma STDC FENV_ACCESS ON
#endif

#include <../../nrnconf.h>
#include "hocstr.h"
#include "equation.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <math.h>
#include <errno.h>
#include "parse.hpp"
#include "hocparse.h"
#include "oc_ansi.h"
#include "ocjump.h"
#include "ocfunc.h"
#include "ocmisc.h"
#include "nrnmpi.h"
#include "nrnpy.h"
#include "nrnfilewrap.h"
#include "../nrniv/backtrace_utils.h"

#include "../utils/profile/profiler_interface.h"

#include <cfenv>
#include <condition_variable>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <thread>
#include <utility>

#include "utils/logger.hpp"

/* for eliminating "ignoreing return value" warnings. */
int nrnignore;

/* only set  in ivoc */
int nrn_global_argc;
char** nrn_global_argv;

#if defined(USE_PYTHON)
int use_python_interpreter = 0;
void (*p_nrnpython_finalize)();
#endif
int nrn_inpython_;
int (*p_nrnpy_pyrun)(const char* fname);

#if 0 /* defined by cmake if rl_event_hook is not available */
#define use_rl_getc_function
#endif

#if defined(MINGW)
extern int stdin_event_ready();
#endif

#define FEEXCEPT (FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW)
static void matherr1(void) {
    const int e = std::fetestexcept(FEEXCEPT);
    /* above gives the signal but for some reason fegetexcept returns 0 */
    switch (e) {
    case FE_DIVBYZERO:
        fprintf(stderr, "Floating exception: Divide by zero\n");
        break;
    case FE_INVALID:
        fprintf(stderr, "Floating exception: Invalid (no well defined result\n");
        break;
    case FE_OVERFLOW:
        fprintf(stderr, "Floating exception: Overflow\n");
        break;
    }
}

int nrn_mpiabort_on_error_{1};

int nrn_feenableexcept_ = 0;  // 1 if feenableexcept(FEEXCEPT) is successful

void nrn_feenableexcept() {
    int result = -2;  // feenableexcept does not exist.
    nrn_feenableexcept_ = 0;
#if NRN_FLOAT_EXCEPTION
    if (ifarg(1) && chkarg(1, 0., 1.) == 0.) {
        result = fedisableexcept(FEEXCEPT);
    } else {
        result = feenableexcept(FEEXCEPT);
        nrn_feenableexcept_ = (result == -1) ? 0 : 1;
    }
#endif
    hoc_ret();
    hoc_pushx((double) result);
}

void start_profile(int i) {}
void add_profile(int i) {}
void pr_profile(void) {}

#if OCSMALL
#define READLINE 0
#endif

#ifndef READLINE
#define READLINE 1
#endif

#if READLINE
extern "C" {
extern char* readline(const char* prompt);
extern void rl_deprep_terminal(void);
extern void add_history(const char*);
}
#endif

int nrn_nobanner_;
int hoc_pipeflag;
int hoc_usegui;
#if 1
/* no longer necessary to distinguish signed from unsigned since EOF
  never stored in a buffer.
 */
#define CHAR char
#else
#if - 1 == '\377'
#define CHAR char
#else
#define CHAR signed char
#endif
#endif
/* buffers will grow automatically if an input line exceeds the following*/
#define TMPBUFSIZE 512
#define CBUFSIZE   512
HocStr* hoc_tmpbuf;
HocStr* hoc_cbufstr;
const char* hoc_promptstr;
static CHAR* hoc_cbuf;
CHAR* hoc_ctp;
int hoc_ictp;

extern char* neuron_home;
extern int hoc_print_first_instance;

#define EPS hoc_epsilon
/*
used to be a FILE* but had fopen problems when 128K cores on BG/P
tried to fopen the same file for reading at once.
*/
NrnFILEWrap* hoc_fin; /* input file pointer */

#include <ctype.h>
const char* progname; /* for error messages */
int hoc_lineno;

#if HAVE_EXECINFO_H
#include <execinfo.h>
#endif
#include <signal.h>
int hoc_intset; /* safer interrupt handling */
int hoc_indef;
const char* infile; /* input file name */
extern size_t hoc_xopen_file_size_;
extern char* hoc_xopen_file_;
const char** gargv; /* global argument list */
int gargc;
static int c = '\n'; /* global for use by warning() */

#if defined(WIN32)
void set_intset() {
    hoc_intset++;
}
extern void ivoc_win32_cleanup();
#endif

static int follow(int expect, int ifyes, int ifno); /* look ahead for >=, etc. */
static int Getc(NrnFILEWrap* fp);
static void unGetc(int c, NrnFILEWrap* fp);
static int backslash(int c);

[[noreturn]] void nrn_exit(int i) {
#if defined(WIN32)
    printf("NEURON exiting abnormally, press return to quit\n");
    fgetc(stdin);
#endif
    exit(i);
}

#if defined(WIN32)
#define HAS_SIGPIPE 0
#else
#define HAS_SIGPIPE 1
#endif
#if HAS_SIGPIPE
/*ARGSUSED*/
static void sigpipe_handler(int sig) {
    fprintf(stderr, "writing to a broken pipe\n");
    signal(SIGPIPE, sigpipe_handler);
}
#endif

int getnb(void) /* get next non-white character */
{
    int c;

    /*EMPTY*/
    while ((c = Getc(hoc_fin)) == ' ' || c == '\t') {
        ;
    }
    return c;
}

/* yylex modified to return -3 when at end of hoc_cbuf. The parser can
   return and take up where it left off later. Supported by modification
   of bison.simple to allow event driven programming.
*/
#define YYNEEDMORE -3
/* for now we say comments or strings that span lines on stdin are in error */
#if 0
#define INCOMMENT 1;
#define INSTRING  2;
static int lexstate = 0;
#endif
/* sometimes is... doesn't work with -3. Hence Getc returns null and look
   at eos to see if true.*/
static int eos;

static char* optarray(char* buf) {
    int c;
    if ((c = Getc(hoc_fin)) == '[') {
        char* s = buf + strlen(buf);
        *s++ = c;
        while (isdigit(c = Getc(hoc_fin)) && (s - buf) < 200) {
            *s++ = c;
        }
        if (c == ']') {
            *s++ = c;
            *s = '\0';
        } else {
            *s = '\0';
            hoc_acterror(buf, " not literal name[int]");
        }
    } else {
        unGetc(c, hoc_fin);
    }
    return buf;
}

int yylex(void) /* hoc6 */
{
restart: /* when no token in between comments */
    eos = 0;
#if 0 /* do we really want to have several states? */
	switch (lexstate) {

	case INCOMMENT:
		goto incomment;
	case INSTRING:
		goto instring;
	}
#endif

    if ((c = getnb()) == EOF) {
        return 0;
    }
    if (c == '/' && follow('*', 1, 0)) /* comment */
    {
        while ((c = Getc(hoc_fin)) != '*' || follow('/', 0, 1)) {
            if (c == EOF)
                return (0);
            /*			if (c == YYNEEDMORE) {*/
            if (eos) {
                hoc_acterror("comment not complete", " in hoc_cbuf");
            }
        }
        goto restart;
    }
    if (c == '.' || isdigit(c)) /* number */
    {
        char* npt;
        double d;
        IGNORE(unGetc(c, hoc_fin));
        npt = (char*) hoc_ctp;
        /*EMPTY*/
        while (isdigit(c = Getc(hoc_fin))) {
            ;
        }
        if (c == '.') {
            /*EMPTY*/
            while (isdigit(c = Getc(hoc_fin))) {
                ;
            }
        }
        if (*npt == '.' && !isdigit(npt[1])) {
            IGNORE(unGetc(c, hoc_fin));
            return (int) (*npt);
        }
        if (c == 'E' || c == 'e') {
            if (isdigit(c = Getc(hoc_fin)) || c == '+' || c == '-') {
                /*EMPTY*/
                while (isdigit(c = Getc(hoc_fin))) {
                    ;
                }
            }
        }
        IGNORE(unGetc(c, hoc_fin));
        IGNORE(sscanf(npt, "%lf", &d));
        if (d == 0.)
            return NUMZERO;
        yylval.sym = hoc_install("", NUMBER, d, &hoc_p_symlist);
        return NUMBER;
    }
    if (isalpha(c) || c == '_') {
        Symbol* s;
        char sbuf[256], *p = sbuf;
        do {
            if (p >= (sbuf + 255)) {
                sbuf[255] = '\0';
                hoc_execerror("Name too long:", sbuf);
            }
            *p++ = c;
        } while ((c = Getc(hoc_fin)) != EOF && (isalnum(c) || c == '_'));
        IGNORE(unGetc(c, hoc_fin));
        *p = '\0';
        if (strncmp(sbuf, "__nrnsec_0x", 11) == 0) {
            yylval.ptr = hoc_sec_internal_name2ptr(sbuf, 1);
            return INTERNALSECTIONNAME;
        }
        /* _pysec.name[int] or _pysec.name[int].name[int] where
          [int] is optional must resolve to Section at parse time.
          void* nrn_parsing_pysec_ is 1 to signal the beginning
          of parsing and as a sub-dictionary pointer in case the
          first level __psec.name[int] is the name of a cell.
          On error or success in parse.ypp, nrn_parsing_pysec_ is
          set back to NULL.
        */
        if (strcmp(sbuf, "_pysec") == 0) {
            if (c != '.') {
                hoc_acterror(
                    "Must be of form _pysec.secname where secname is the literal result of "
                    "nrn.Section.name() .",
                    NULL);
            }
            yylval.ptr = NULL;
            nrn_parsing_pysec_ = (void*) 1;
            return PYSEC;
        }
        if (nrn_parsing_pysec_) {
            yylval.ptr = hoc_pysec_name2ptr(optarray(sbuf), 1);
            if (nrn_parsing_pysec_ > (void*) 1) { /* there will be a second part */
                c = Getc(hoc_fin);
                unGetc(c, hoc_fin);
                if (c != '.') { /* so there must be a . */
                    nrn_parsing_pysec_ = NULL;
                    hoc_acterror(
                        "Must be of form _pysec.cellname.secname where cellname.secname is the "
                        "literal result of nrn.Section.name() .",
                        NULL);
                }
            }
            if (yylval.ptr == NULL) {
                return PYSECOBJ;
            } else {
                nrn_parsing_pysec_ = NULL;
                return PYSECNAME;
            }
        }
        if ((s = hoc_lookup(sbuf)) == 0)
            s = hoc_install(sbuf, UNDEF, 0.0, &hoc_symlist);
        yylval.sym = s;
        return s->type == UNDEF ? VAR : s->type;
    }
    if (c == '$') { /* argument? */
        int ith;
        int n = 0;
        int retval = follow('o', OBJECTARG, ARG);
        if (retval == ARG)
            retval = follow('s', STRINGARG, ARG);
        if (retval == ARG)
            retval = follow('&', ARGREF, ARG);
        ith = follow('i', 1, 0);
        if (ith) {
            yylval.narg = 0;
            return retval;
        }
        while (isdigit(c = Getc(hoc_fin)))
            n = 10 * n + c - '0';
        IGNORE(unGetc(c, hoc_fin));
        if (n == 0)
            hoc_acterror("strange $...", (char*) 0);
        yylval.narg = n;
        return retval;
    }
    if (c == '"') /* quoted string */
    {
        static HocStr* sbuf;
        char* p;
        int n;
        if (!sbuf) {
            sbuf = hocstr_create(256);
        }
        for (p = sbuf->buf; (c = Getc(hoc_fin)) != '"'; p++) {
            if (c == '\n' || c == EOF || c == YYNEEDMORE)
                hoc_acterror("missing quote", "");
            n = p - sbuf->buf;
            if (n >= sbuf->size - 1) {
                hocstr_resize(sbuf, n + 200);
                p = sbuf->buf + n;
            }
            *p = backslash(c);
        }
        *p = 0;
        yylval.sym = hoc_install("", CSTRING, 0.0, &hoc_p_symlist);

        (yylval.sym)->u.cstr = (char*) emalloc((unsigned) (strlen(sbuf->buf) + 1));
        Strcpy((yylval.sym)->u.cstr, sbuf->buf);
        return CSTRING;
    }
    switch (c) {
    case 0: {
        if (eos)
            return YYNEEDMORE;
        else
            return 0;
    }
    case '>':
        return follow('=', GE, GT);
    case '<':
        return follow('=', LE, LT);
    case '!':
        return follow('=', NE, NOT);

    case '+':
    case '-':
    case '*': {
        if (follow('=', 1, 0)) {
            yylval.narg = c;
            return ROP;
        } else {
            return c;
        }
    }
    case '=': {
        if (follow('=', EQ, '=') == EQ) {
            return EQ;
        }
        if (hoc_do_equation) {
            return EQNEQ;
        }
        yylval.narg = 0;
        return ROP;
    }
    case '|':
        return follow('|', OR, '|');
    case '&':
        return follow('&', AND, '&');
    case '\\': {
        int i; /* continuation line if last char in line is \ */
        i = follow('\n', 1000, '\\');
        if (i == 1000) {
            return yylex();
        }
        return i;
    }
    case '\r':
        return follow('\n', '\n', '\n');
    case '\n':
        return '\n';
    case '/':
        if (follow('/', 1, 0)) {
            while (*hoc_ctp) {
                ++hoc_ctp;
            }
            return '\n';
        } else if (follow('=', 1, 0)) {
            yylval.narg = c;
            return ROP;
        } else {
            return '/';
        }
    default:
        return c;
    }
}

static int backslash(int c) { /* get next char with \'s interpreted */
    static char transtab[] = "b\bf\fn\nr\rt\t";
    if (c != '\\')
        return c;
    c = Getc(hoc_fin);
    if (islower(c) && strchr(transtab, c))
        return strchr(transtab, c)[1];
    return c;
}

static int follow(int expect, int ifyes, int ifno) /* look ahead for >=, etc. */
{
    int c = Getc(hoc_fin);

    if (c == expect)
        return ifyes;
    IGNORE(unGetc(c, hoc_fin));
    return ifno;
}

void hoc_arayinstal(void) /* allocate storage for arrays */
{
    int i, nsub;
    Symbol* sp;

    nsub = (hoc_pc++)->i;
    sp = hoc_spop();

    hoc_freearay(sp);
    sp->type = VAR;
    sp->s_varn = 0;
    i = hoc_arayinfo_install(sp, nsub);
    if ((OPVAL(sp) = (double*) hoc_Ecalloc((unsigned) i, sizeof(double))) == (double*) 0) {
        hoc_freearay(sp);
        Fprintf(stderr, "Not enough space for array %s\n", sp->name);
        hoc_malchk();
        hoc_execerror("", (char*) 0);
    }
}

int hoc_arayinfo_install(Symbol* sp, int nsub) {
    double total, subscpt;
    int i;
    hoc_free_arrayinfo(sp->arayinfo);
    sp->arayinfo = (Arrayinfo*) emalloc((unsigned) (sizeof(Arrayinfo) + nsub * sizeof(int)));
    /*printf("emalloc arrayinfo at %lx\n", sp->arayinfo);*/
    sp->arayinfo->a_varn = (unsigned*) 0;
    sp->arayinfo->nsub = nsub;
    sp->arayinfo->refcount = 1;
    total = 1.;
    while (nsub) {
        subscpt = floor(hoc_xpop() + EPS);
        if (subscpt <= 0.)
            hoc_execerror("subscript < 1", sp->name);
        total = total * subscpt;
        sp->arayinfo->sub[--nsub] = (int) subscpt;
    }
    if (total > 2e9) {
        /*
        following gives purify uninitialized memory read and cannot work
        around it with anything involving i. Must be a bug in purify because
        the i=(int)total gives the UMR  when it is just above this if statement.
        but no UMR if in present location just above return.
            if ( (double)(i = (int)total) != total) {
        */
        free((char*) sp->arayinfo);
        sp->arayinfo = (Arrayinfo*) 0;
        hoc_execerror(sp->name, ":total subscript too large");
    }
    if (OPARINFO(sp)) {
        /* probably never get here */
        hoc_free_arrayinfo(OPARINFO(sp));
    }
    OPARINFO(sp) = sp->arayinfo;
    ++sp->arayinfo->refcount;
    i = (int) total;
    return i;
}

void hoc_freearay(Symbol* sp) {
    Arrayinfo** pa = &(OPARINFO(sp));
    if (sp->type == VAR) {
        hoc_free_val_array(OPVAL(sp), hoc_total_array(sp));
        sp->type = UNDEF;
    }
    hoc_free_arrayinfo(*pa);
    hoc_free_arrayinfo(sp->arayinfo);
    sp->arayinfo = (Arrayinfo*) 0;
    *pa = (Arrayinfo*) 0;
}

void hoc_free_arrayinfo(Arrayinfo* a) {
    if (a != (Arrayinfo*) 0) {
        if ((--a->refcount) <= 0) {
            if (a->a_varn != (unsigned*) 0)
                free((char*) (a->a_varn));
            free((char*) a);
            /*		printf("free arrayinfo at %lx\n", a);*/
        }
    }
}

void hoc_defnonly(const char* s) { /* warn if illegal definition */
    if (!hoc_indef)
        hoc_acterror(s, "used outside definition");
}

/* messages can turned off but the user had better check the return
value of oc_run()
*/
static int debug_message_;
void hoc_show_errmess_always(void) {
    double x;
    x = chkarg(1, 0., 1.);
    debug_message_ = (int) x;
    hoc_ret();
    hoc_pushx(x);
}

int hoc_execerror_messages;
int nrn_try_catch_nest_depth{0};
int yystart;

void hoc_execerror_mes(const char* s, const char* t, int prnt) { /* recover from run-time error */
    hoc_in_yyparse = 0;
    yystart = 1;
    hoc_errno_check();
    if (debug_message_ || prnt) {
        hoc_warning(s, t);
        frame_debug();
        nrn_err_dialog(s);
    }
    // In case hoc_warning not called
    hoc_ctp = hoc_cbuf;
    *hoc_ctp = '\0';
    // There used to be some logic here to abort here if we are inside an OcJump call.
#if NRNMPI
    if (nrnmpi_numprocs_world > 1 && nrn_mpiabort_on_error_) {
        nrnmpi_abort(-1);
    }
#endif
    hoc_execerror_messages = 1;
    if (hoc_fin && hoc_pipeflag == 0 && (!nrn_fw_eq(hoc_fin, stdin) || !nrn_istty_)) {
        IGNORE(nrn_fw_fseek(hoc_fin, 0L, 2)); /* flush rest of file */
    }

    // If the exception is due to a multiple ^C interrupt, then onintr
    // will not exit normally (because of the throw below) and the signal
    // would remain in a SIG_BLOCK state.
    // It is not clear to me if this would be better done in every catch.
#if HAVE_SIGPROCMASK
    if (hoc_intset > 1) {
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGINT);
        sigprocmask(SIG_UNBLOCK, &set, NULL);
    }
#endif  // HAVE_SIGPROCMASK

    hoc_intset = 0;
    hoc_oop_initaftererror();
    std::string message{"hoc_execerror: "};
    message.append(s);
    if (t) {
        message.append(1, ' ');
        message.append(t);
    }
    throw neuron::oc::runtime_error(std::move(message));
}

void hoc_execerror(const char* s, const char* t) /* recover from run-time error */
{
    hoc_execerror_mes(s, t, hoc_execerror_messages);
}

void onintr(int /* sig */) /* catch interrupt */
{
    /*ARGSUSED*/
    stoprun = 1;
    if (hoc_intset++)
        hoc_execerror("interrupted", (char*) 0);
    IGNORE(signal(SIGINT, onintr));
}

static int coredump;

void hoc_coredump_on_error(void) {
    coredump = 1;
    hoc_ret();
    hoc_pushx(1.);
}

void print_bt() {
#ifdef USE_BACKWARD
    backward_wrapper();
#else
#if HAVE_EXECINFO_H
    const size_t nframes = 12;
    void* frames[nframes];
    size_t size;
    char** bt_strings = NULL;
    // parsed elements from stacktrace line:
    size_t funcname_size = 256;
    // symbol stores the symbol at which the signal was invoked
    char* symbol = static_cast<char*>(malloc(sizeof(char) * funcname_size));
    // the function name where the signal was invoked
    char* funcname = static_cast<char*>(malloc(sizeof(char) * funcname_size));
    // offset stores the relative address from the function where the signal was invoked
    char* offset = static_cast<char*>(malloc(sizeof(char) * 10));
    // the memory address of the function
    void* addr = NULL;
    // get void*'s for maximum last 16 entries on the stack
    size = backtrace(frames, nframes);

    // print out all the frames to stderr
    Fprintf(stderr, "Backtrace:\n");
    // get the stacktrace as an array of strings
    bt_strings = backtrace_symbols(frames, size);
    if (bt_strings) {
        size_t i;
        // start printing at third frame to skip the signal handler and printer function
        for (i = 2; i < size; ++i) {
            // parse the stack frame line
            int status = parse_bt_symbol(bt_strings[i], &addr, symbol, offset);
            if (status) {
                status = cxx_demangle(symbol, &funcname, &funcname_size);
                if (status == 0) {  // demangling worked
                    Fprintf(stderr, "\t%s : %s+%s\n", bt_strings[i], funcname, offset);
                } else {  // demangling failed, fallback
                    Fprintf(stderr, "\t%s : %s()+%s\n", bt_strings[i], symbol, offset);
                }
            } else {  // could not parse, simply print the stackframe as is
                Fprintf(stderr, "\t%s\n", bt_strings[i]);
            }
        }
        free(bt_strings);
    }
    free(funcname);
    free(offset);
    free(symbol);
#else
    Fprintf(stderr, "No backtrace info available.\n");
#endif
#endif
}

void fpecatch(int /* sig */) /* catch floating point exceptions */
{
    /*ARGSUSED*/
#if NRN_FLOAT_EXCEPTION
    matherr1();
#endif
    Fprintf(stderr, "Floating point exception\n");
    print_bt();
    if (coredump) {
        abort();
    }
    signal(SIGFPE, fpecatch);
    hoc_execerror("Floating point exception.", (char*) 0);
}

__attribute__((noreturn)) void sigsegvcatch(int /* sig */) /* segmentation violation probably due to
                                                              arg type error */
{
    Fprintf(stderr, "Segmentation violation\n");
    print_bt();
    /*ARGSUSED*/
    if (coredump) {
        abort();
    }
    hoc_execerror("Aborting.", (char*) 0);
}

#if HAVE_SIGBUS
__attribute__((noreturn)) void sigbuscatch(int /* sig */) {
    Fprintf(stderr, "Bus error\n");
    print_bt();
    /*ARGSUSED*/
    if (coredump) {
        abort();
    }
    hoc_execerror("Aborting. ", "See $NEURONHOME/lib/help/oc.help");
}
#endif

int hoc_pid(void) {
    return (int) getpid();
} /* useful for making unique temporary file names */

/* readline should be avoided if stdin is not a terminal */
int nrn_istty_;

int hoc_main1_inited_;

/* has got to be called first. oc can only be event driven after this returns */
void hoc_main1_init(const char* pname, const char** envp) {
    extern NrnFILEWrap* hoc_frin;
    extern FILE* hoc_fout;

    if (!hoc_xopen_file_) {
        hoc_xopen_file_size_ = 200;
        hoc_xopen_file_ = static_cast<char*>(emalloc(hoc_xopen_file_size_));
    }
    hoc_xopen_file_[0] = '\0';

    hoc_promptstr = "oc>";
    yystart = 1;
    hoc_lineno = 0;
    if (hoc_main1_inited_) {
        return;
    }

    /* could have been forced with the -isatty option */
    if (nrn_istty_ == 0) { /* if not set then */
#ifdef HAVE_ISATTY
        nrn_istty_ = isatty(0);
#else
        /* if we do not know, then assume so */
        nrn_istty_ = 1;
#endif
    }
    if (nrn_istty_ == -1) {
        nrn_istty_ = 0;
    }
    hoc_tmpbuf = hocstr_create(TMPBUFSIZE);
    hoc_cbufstr = hocstr_create(CBUFSIZE);
    hoc_cbuf = hoc_cbufstr->buf;
    hoc_ctp = hoc_cbuf;
    hoc_frin = nrn_fw_set_stdin();
    hoc_fout = stdout;
    if (!nrn_is_cable()) {
        Fprintf(stderr,
                "Copyright 1992 -  Michael Hines, Neurobiology Dept., DUMC, Durham, NC.  27710\n");
    }
    progname = pname;
    hoc_init();
    initplot();
    hoc_main1_inited_ = 1;
}

HocStr* hocstr_create(size_t size) {
    HocStr* hs;
    hs = (HocStr*) emalloc(sizeof(HocStr));
    hs->size = size;
    hs->buf = static_cast<char*>(emalloc(size + 1));
    return hs;
}

static CHAR* fgets_unlimited_nltrans(HocStr* s, NrnFILEWrap* f, int nltrans);

char* fgets_unlimited(HocStr* s, NrnFILEWrap* f) {
    return fgets_unlimited_nltrans(s, f, 0);
}

void hocstr_delete(HocStr* hs) {
    free(hs->buf);
    free(hs);
}

void hocstr_resize(HocStr* hs, size_t n) {
    if (hs->size < n) {
        /*printf("hocstr_resize to %d\n", n);*/
        hs->buf = static_cast<char*>(erealloc(hs->buf, n + 1));
        hs->size = n;
    }
}

void hocstr_copy(HocStr* hs, const char* buf) {
    hocstr_resize(hs, strlen(buf) + 1);
    strcpy(hs->buf, buf);
}

#ifdef MINGW
static int cygonce; /* does not need the '-' after a list of hoc files */
#endif

static int hoc_run1();

// hoc6
int hoc_main1(int argc, const char** argv, const char** envp) {
    int exit_status = EXIT_SUCCESS;
#ifdef WIN32
    extern void hoc_set_unhandled_exception_filter();
    hoc_set_unhandled_exception_filter();
#endif
#if 0
	int controlled;
#endif
#if PVM
    init_parallel(&argc, argv);
#endif

    hoc_audit_from_hoc_main1(argc, argv, envp);
    hoc_main1_init(argv[0], envp);
    try {
#if HAS_SIGPIPE
        signal(SIGPIPE, sigpipe_handler);
#endif
        gargv = argv;
        gargc = argc;
        if (argc > 2 && strcmp(argv[1], "-bbs_nhost") == 0) {
            /* if IV not running this may still be here */
            gargv += 2;
            gargc -= 2;
        }

        if (gargc == 1) /* fake an argument list */
        {
            static const char* stdinonly[] = {"-"};

#ifdef MINGW
            cygonce = 1;
#endif
            gargv = stdinonly;
            gargc = 1;
        } else {
            ++gargv;
            --gargc;
        }
        // If we pass multiple HOC files to special then this loop runs once for each one of them
        while (hoc_moreinput()) {
            exit_status = hoc_run1();
            if (exit_status) {
                // Abort if one of the HOC files we're processing gives an error
                break;
            }
        }
        return exit_status;
    } catch (std::exception const& e) {
        Fprintf(stderr, fmt::format("hoc_main1 caught exception: {}\n", e.what()).c_str());
        nrn_exit(1);
    }
}

#ifdef MINGW
namespace {
std::mutex inputMutex_;
std::condition_variable inputCond_;
// This is intentionally leaked because it is never joined, and the destructor
// would call terminate.
std::thread* inputReady_;
int inputReadyFlag_;
int inputReadyVal_;
}  // namespace
extern "C" int getch();

void inputReadyThread() {
    for (;;) {
        // The pthread version had pthread_testcancel() here, but the thread was
        // never cancelled.
        int i = getch();
        std::unique_lock<std::mutex> lock{inputMutex_};
        inputReadyFlag_ = 1;
        inputReadyVal_ = i;
        stdin_event_ready();
        inputCond_.wait(lock);
    }
    printf("inputReadyThread done\n");
}
#endif

void hoc_final_exit(void) {
#if defined(USE_PYTHON)
    if (neuron::python::methods.interpreter_start) {
        neuron::python::methods.interpreter_start(0);
    }
#endif
    bbs_done();
    hoc_audit_from_final_exit();

    /* Don't close the plots for the sub-processes when they finish,
       by default they are then closed when the master process ends */
    hoc_close_plot();
#if READLINE && !defined(MINGW)
    rl_deprep_terminal();
#endif
    ivoc_cleanup();
#if defined(WIN32) && HAVE_IV
    ivoc_win32_cleanup();
#endif
    auto tmp_dir = std::getenv("TEMP");
    auto path = std::filesystem::path(tmp_dir ? std::string(tmp_dir) : "/tmp") /
                fmt::format("oc{}.hl", hoc_pid());
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

void hoc_quit(void) {
    hoc_final_exit();
    ivoc_final_exit();
#if defined(USE_PYTHON)
    /* if python was launched and neuron is an extension */
    if (p_nrnpython_finalize) {
        extern int bbs_poll_;
        // don't want an MPI_Iprobe after MPI_Finalize if python gui
        // thread calls doNotify().
        bbs_poll_ = -1;
        (*p_nrnpython_finalize)();
    }
#endif
    int exit_code = ifarg(1) ? int(*getarg(1)) : 0;
    nrn::Instrumentor::finalize_profile();
    exit(exit_code);
}

#ifdef MINGW
static const char* double_at2space(const char* infile) {
    char* buf;
    const char* cp1;
    char* cp2;
    int replace = 0;
    for (cp1 = infile; *cp1; ++cp1) {
        if (*cp1 == '@' && cp1[1] == '@') {
            replace = 1;
            break;
        }
    }
    if (!replace) {
        return infile;
    }
    buf = static_cast<char*>(emalloc(strlen(infile) + 1));
    for (cp1 = infile, cp2 = buf; *cp1; ++cp1, ++cp2) {
        if (*cp1 == '@' && cp1[1] == '@') {
            *cp2 = ' ';
            ++cp1;
        } else {
            *cp2 = *cp1;
        }
    }
    *cp2 = '\0';
    return buf;
}
#endif /*MINGW*/

int hoc_moreinput() {
    if (hoc_pipeflag) {
        hoc_pipeflag = 0;
        return 1;
    }
#if defined(WIN32)
    /* like mswin, do not need a '-' after hoc files, but ^D works */
    if (gargc == 0 && cygonce == 0) {
        cygonce = 1;
        hoc_fin = nrn_fw_set_stdin();
        infile = 0;
        hoc_xopen_file_[0] = 0;
#if defined(USE_PYTHON)
        return use_python_interpreter ? 0 : 1;
#else
        return 1;
#endif
    }
#endif  // WIN32
    if (hoc_fin && !nrn_fw_eq(hoc_fin, stdin)) {
        IGNORE(nrn_fw_fclose(hoc_fin));
    }
    hoc_fin = nrn_fw_set_stdin();
    infile = 0;
    hoc_xopen_file_[0] = 0;
    if (gargc-- <= 0) {
        return 0;
    }
    infile = *gargv++;
#if defined(WIN32)
    if (infile[0] == '"') {
        char* cp = strdup(infile + 1);
        for (++cp; *cp; ++cp) {
            if (*cp == '"') {
                *cp = '\0';
                break;
            }
        }
        infile = cp;
    }
#endif  // WIN32
#ifdef MINGW
    /* have difficulty passing spaces within arguments from neuron.exe
    through the neuron.sh shell script to nrniv.exe. Therefore
    neuron.exe converts the ' ' to "@@" and here we need to convert
    it back */
    infile = double_at2space(infile);
#endif
    hoc_lineno = 0;
#if defined(USE_PYTHON)
    if (use_python_interpreter) {
        /* deal only with .hoc files. The hoc files are intended
        only for legacy code and there is no notion of stdin interaction
        with the hoc interpreter.
        */
        if (strlen(infile) < 4 || strcmp(infile + strlen(infile) - 4, ".hoc") != 0) {
            return hoc_moreinput();
        }
    }
#endif
    if (strcmp(infile, "-") == 0) {
        hoc_fin = nrn_fw_set_stdin();
        infile = 0;
        hoc_xopen_file_[0] = 0;
    } else if (strcmp(infile, "-parallel") == 0) {
        /* ignore "val" as next argument */
        infile = *gargv++;
        gargc--;
        return hoc_moreinput();
    } else if (strcmp(infile, "-c") == 0) {
        int hpfi, err;
        HocStr* hs;
        infile = *gargv++;
        gargc--;
#ifdef MINGW
        infile = double_at2space(infile);
#endif
        hs = hocstr_create(strlen(infile) + 2);
        std::snprintf(hs->buf, hs->size + 1, "%s\n", infile);
        /* now infile is a hoc statement */
        hpfi = hoc_print_first_instance;
        hoc_fin = (NrnFILEWrap*) 0;
        hoc_print_first_instance = 0;
        // This is processing HOC code via -c on the commandline. That HOC code could include
        // nrnpython(...), so the Python interpreter needs to be configured appropriately for
        // that (i.e. sys.path[0] = '').
        if (neuron::python::methods.interpreter_set_path) {
            neuron::python::methods.interpreter_set_path({});
        }
        err = hoc_oc(hs->buf);
        hoc_print_first_instance = hpfi;
        hocstr_delete(hs);
        if (err) {
            hoc_execerror("arg not valid statement:", infile);
        }
        return hoc_moreinput();
    } else if (strlen(infile) > 3 && strcmp(infile + strlen(infile) - 3, ".py") == 0) {
        if (!p_nrnpy_pyrun) {
            hoc_execerror("Python not available to interpret", infile);
        }
        if (!p_nrnpy_pyrun(infile)) {
            hoc_execerror("Python error", infile);
        }
        return hoc_moreinput();
    } else if ((hoc_fin = nrn_fw_fopen(infile, "r")) == (NrnFILEWrap*) 0) {
        Fprintf(stderr, "%d %s: can't open %s\n", nrnmpi_myid_world, progname, infile);
#if NRNMPI
        if (nrnmpi_numprocs_world > 1) {
            nrnmpi_abort(-1);
        }
#endif
        return hoc_moreinput();
    }
    if (infile) {
        if (strlen(infile) >= hoc_xopen_file_size_) {
            hoc_xopen_file_size_ = strlen(infile) + 100;
            hoc_xopen_file_ = static_cast<char*>(erealloc(hoc_xopen_file_, hoc_xopen_file_size_));
        }
        strcpy(hoc_xopen_file_, infile);
        // This is, unfortunately rather implicitly, how we trigger execution of HOC files on a
        // commandline like `nrniv a.hoc b.hoc`. To make HOC treatment similar to Python treatment
        // we would pass hoc_xopen_file_ here, which would imply that nrnpython("...") inside
        // test.hoc sees sys.path[0] == "/dir/" when we run `nrniv /dir/test.hoc`, however it seems
        // that legacy models (183300) assume that sys.path[0] == '' in this context, so we stick
        // with that. There is no particular reason to follow Python conventions when launching HOC
        // scripts, in contrast to Python scripts where we strive to make `nrniv foo.py` and
        // `python foo.py` behave in the same way.
        if (neuron::python::methods.interpreter_set_path) {
            neuron::python::methods.interpreter_set_path({});
        }
    }
    return 1;
}

using SignalType = void(int);
static SignalType* signals[4];

static void set_signals(void) {
    signals[0] = signal(SIGINT, onintr);
    signals[1] = signal(SIGFPE, fpecatch);
    signals[2] = signal(SIGSEGV, sigsegvcatch);
#if HAVE_SIGBUS
    signals[3] = signal(SIGBUS, sigbuscatch);
#endif
}

static void restore_signals(void) {
    signals[0] = signal(SIGINT, signals[0]);
    signals[1] = signal(SIGFPE, signals[1]);
    signals[2] = signal(SIGSEGV, signals[2]);
#if HAVE_SIGBUS
    signals[3] = signal(SIGBUS, signals[3]);
#endif
}

struct signal_handler_guard {
    signal_handler_guard() {
        set_signals();
    }
    ~signal_handler_guard() {
        restore_signals();
    }
};

// Helper to temporarily set a global to something and then restore the original
// value when the helper goes out of scope
template <typename T>
struct temporarily_change {
    temporarily_change(T& global, T new_value)
        : m_global_value{global}
        , m_saved_value{std::exchange(global, new_value)} {}
    ~temporarily_change() {
        m_global_value = m_saved_value;
    }

  private:
    T& m_global_value;
    T m_saved_value;
};

// execute until EOF
// called from a try { ... } block in hoc_main1
static int hoc_run1() {
    auto* const sav_fin = hoc_fin;
    hoc_pipeflag = 0;
    hoc_execerror_messages = 1;
    auto const loop_body = []() {
        hoc_initcode();
        if (!hoc_yyparse()) {
            if (hoc_intset) {
                hoc_execerror("interrupted", nullptr);
            }
            return false;
        }
        hoc_execute(hoc_progbase);
        return true;
    };
    if (nrn_try_catch_nest_depth) {
        // This is not the most shallowly nested call to hoc_run1(), allow the
        // most shallowly nested call to handle exceptions.
        while (loop_body())
            ;
    } else {
        // This is the most shallowly nested call to hoc_run1(), handle exceptions.
        signal_handler_guard _{};  // install signal handlers
        try_catch_depth_increment tell_children_we_will_catch{};
        hoc_intset = 0;
        for (;;) {
            try {
                if (!loop_body()) {
                    break;
                }
            } catch (std::exception const& e) {
                hoc_fin = sav_fin;
                Fprintf(stderr, "hoc_run1: caught exception");
                std::string_view what{e.what()};
                if (!what.empty()) {
                    Fprintf(stderr, fmt::format(": {}", what).c_str());
                }
                Fprintf(stderr, "\n");
                // Exit if we're not in interactive mode
                if (!nrn_fw_eq(hoc_fin, stdin)) {
                    return EXIT_FAILURE;
                }
            }
        }
    }
    return EXIT_SUCCESS;
}

/* event driven interface to oc. This routine always returns after processing
   its argument. The normal case is where the argument consists of a complete
   statement which can be parsed and executed. However this routine can
   be called with single tokens as well and the parsing will proceed
   incrementally until the last token that completes a statement is seen
   at which time the statement is executed.
   The routine can also be called with a string containing multiple statements
   separated by '\n's.
   During execution a ^C will halt
   processing and this routine will return to the calling statement. (if it
   is the controlling call for the jmp_buf; see below).
   Parse errors and execution errors will also result in a normal return to the
   calling statement.

   To avoid blocking other input events, this routine should not be called
   with statements which take too much time to execute or block
   waiting for input. With regard to long executing statements, at least in
   xview it is possible to run the event loop from the execute loop at
   intervals in code.cpp or else to run the event loop once via a hoc command.

   Where do we go in case of an hoc_execerror. We have to go to the beginning
   of hoc. But just maybe that is here. However hoc_oc may be called
   recursively. Or it may be called from the original hoc_run. Or it may be
   There is therefore a notion of the controlling routine for the jmp_buf begin.
   We only do a setjmp and set the signals when there is no other controlling
   routine. Note that setjmp is no longer used, but for now the same notion of a
   controlling routine is maintained.
*/

/* allow hoc_oc(buf) to handle any number of multiline statements */
static const char* nrn_inputbufptr;
static void nrn_inputbuf_getline(void) {
    CHAR* cp;
    cp = hoc_ctp = hoc_cbuf = hoc_cbufstr->buf;
    while (*nrn_inputbufptr) {
        *cp++ = *nrn_inputbufptr++;
        if (cp[-1] == '\n') {
            break;
        }
    }
    if (cp != hoc_ctp) {
        if (cp[-1] != '\n') {
            *cp++ = '\n';
        }
    }
    *cp = '\0';
}

// used by ocjump.cpp
void oc_save_input_info(const char** i1, int* i2, int* i3, NrnFILEWrap** i4) {
    *i1 = nrn_inputbufptr;
    *i2 = hoc_pipeflag;
    *i3 = hoc_lineno;
    *i4 = hoc_fin;
}
void oc_restore_input_info(const char* i1, int i2, int i3, NrnFILEWrap* i4) {
    nrn_inputbufptr = i1;
    hoc_pipeflag = i2;
    hoc_lineno = i3;
    hoc_fin = i4;
}

int hoc_oc(const char* buf) {
    return hoc_oc(buf, std::cerr);
}

int hoc_oc(const char* buf, std::ostream& os) {
    // the substantive code to execute, everything else is to do with handling
    // errors here or elsewhere
    auto const kernel = [buf]() {
        hoc_intset = 0;
        hocstr_resize(hoc_cbufstr, strlen(buf) + 10);
        nrn_inputbuf_getline();
        while (*hoc_ctp || *nrn_inputbufptr) {
            hoc_ParseExec(yystart);
            if (hoc_intset) {
                hoc_execerror("interrupted", nullptr);
            }
        }
    };
    auto const lineno_manager = temporarily_change{hoc_lineno, 1};
    auto const pipeflag_manager = temporarily_change{hoc_pipeflag, 3};
    auto const inputbufptr_manager = temporarily_change{nrn_inputbufptr, buf};
    if (nrn_try_catch_nest_depth) {
        // Someone else is responsible for catching errors
        kernel();
    } else {
        // This is the highest level try/catch
        try_catch_depth_increment tell_children_we_will_catch{};
        try {
            signal_handler_guard _{};
            kernel();
        } catch (std::exception const& e) {
            os << "hoc_oc caught exception: " << e.what() << std::endl;
            hoc_initcode();
            hoc_intset = 0;
            return 1;
        }
    }
    hoc_execerror_messages = 1;
    return 0;
}

void hoc_warning(const char* s, const char* t) /* print warning message */
{
    CHAR* cp;
    char id[10];
    int n;
    if (nrnmpi_numprocs_world > 1) {
        Sprintf(id, "%d ", nrnmpi_myid_world);
    } else {
        id[0] = '\0';
    }
    if (t) {
        Fprintf(stderr, "%s%s: %s %s\n", id, progname, s, t);
    } else {
        Fprintf(stderr, "%s%s: %s\n", id, progname, s);
    }
    if (hoc_xopen_file_ && hoc_xopen_file_[0]) {
        Fprintf(stderr, "%s in %s near line %d\n", id, hoc_xopen_file_, hoc_lineno);
    } else {
        Fprintf(stderr, "%s near line %d\n", id, hoc_lineno);
    }
    n = strlen(hoc_cbuf);
    for (cp = hoc_cbuf; cp < (hoc_cbuf + n); ++cp) {
        if (!isprint((int) (*cp)) && !isspace((int) (*cp))) {
            Fprintf(stderr,
                    "%scharacter \\%03o at position %ld is not printable\n",
                    id,
                    ((int) (*cp) & 0xff),
                    cp - hoc_cbuf);
            break;
        }
    }
    Fprintf(stderr, "%s %s", id, hoc_cbuf);
    if (nrnmpi_numprocs_world > 0) {
        for (cp = hoc_cbuf; cp != hoc_ctp; cp++) {
            Fprintf(stderr, " ");
        }
        Fprintf(stderr, "^\n");
    }
    hoc_ctp = hoc_cbuf;
    *hoc_ctp = '\0';
}


static int Getc(NrnFILEWrap* fp) {
    /*ARGSUSED*/
    if (*hoc_ctp) {
        ++hoc_ictp;
        return *hoc_ctp++;
    }

#if 0
/* don't allow parser to block. Actually partial statements were never
allowed anyway */
	if (!hoc_pipeflag && nrn_fw_eq(fp,stdin)) {
		eos = 1;
		return 0;
	}
#endif

    if (hoc_get_line() == EOF) {
        return EOF;
    }
    return *hoc_ctp++;
}

static void unGetc(int c, NrnFILEWrap* fp) {
    /*ARGSUSED*/
    if (c != EOF && c && hoc_ctp != hoc_cbuf) {
        *(--hoc_ctp) = c;
    }
}

int hoc_in_yyparse = 0;

int hoc_yyparse(void) {
    /* read line before calling yyparse() and set flag that we
    are inside yyparse() since re-entry is not allowed.
    This allows xview menus to work since most of the time
    we are reading a line from this point instead of in
    Getc which was called from yyparse().
    */
    /*
    the above may be obsolete.
    I switched to a modified bison generator from yacc which is generated
    a parser exactly like it used to be but may return YYNEEDMORE=-3
    instead of blocking in yylex. If so another call to yyparse will
    take up exactly where it left off.  This makes it easier to
    support event driven processing.
    Maybe it's time to redo the get_line process which fills hoc_cbuf with
    the next line to read. A real event driven program would fill hoc_cbuf
    and then call yyparse() directly. yyparse() returns
    0 : end of file
    '\n' : ready to execute a command
    -3: need more input, not at a point where it accepts or rejects the
        input.
    */

    int i;

    if (hoc_in_yyparse) {
        hoc_execerror("Cannot re-enter parser", (char*) 0);
    }
    do {
        if (hoc_get_line() == EOF) {
            return 0;
        }
        hoc_in_yyparse = 1;
        i = yyparse();
        hoc_in_yyparse = 0;
        if (i == -3) {  // need more input
            hoc_in_yyparse = 1;
            i = '\n';
        }
    } while (i == '\n');
    return i;
}

#ifdef WIN32
#define INTERVIEWS 1
#endif

int hoc_interviews = 0;
#if INTERVIEWS
extern int run_til_stdin(); /* runs the interviews event loop. Returns 1
                if there is input to be read. Returns 0
                if somebody said quit but there is no input
                */

extern void hoc_notify_value(void);

#if READLINE
#ifdef MINGW
extern "C" int (*rl_getc_function)(void);
extern "C" int rl_getc(void);
static int getc_hook(void) {
    if (!inputReady_) {
        stdin_event_ready(); /* store main thread id */
        inputReady_ = new std::thread{inputReadyThread};
    } else {
        inputMutex_.unlock();
    }
    //	printf("run til stdin\n");
    while (!inputReadyFlag_) {
        run_til_stdin();
        usleep(10000);
    }
    inputReadyFlag_ = 0;
    int i = inputReadyVal_;
    inputMutex_.lock();
    inputCond_.notify_one();
    return i;
}
#else /* not MINGW */

#if defined(use_rl_getc_function)
/* e.g. mac libedit.3.dylib missing rl_event_hook */

extern int iv_dialog_is_running;
extern "C" int (*rl_getc_function)(void);
static int getc_hook(void) {
    while (1) {
        int r;
        unsigned char c;
        if (hoc_interviews && !hoc_in_yyparse && run_til_stdin() == 0) {
            // nothing in stdin  (happens when windows are dismissed)
            continue;
        }
        if ((r = read(0, &c, sizeof(c))) == sizeof(c)) {
            return (int) c;
        } else {
            /* this seems consistent with the internal readline and the
                current master version 8.0. That is, rl_getc in the
                internal readline gets the return value of read and only
                cares about r == 1, loops if errno == EINTR, and returns
                EOF otherwise. (Note: internal readline does not have
                rl_getc_function.). Version 8.0 rl_getc is complex but in
                terms of read, it returns c if r == 1, returns EOF if
                r == 0, loops if errno == EINTR, and generally returns EOF
                if some other error occurred.
            */
            if (errno != EINTR) {
                return EOF;
            }
        }
    }
}

#else /* not use_rl_getc_function */

extern int (*rl_event_hook)(void);
static int event_hook(void) {
    int i;
    i = run_til_stdin();
    return i;
}

#endif /* not use_rl_getc_function */

#endif /* not MINGW */
#endif /* READLINE */
#endif /* INTERVIEWS */

/*
 On Mac combinations of /n /r /r/n require binary mode
 (otherwise /r/n is /n/n)
 but then /r does not end a line in fgets (keeps reading til /n)
 so we make our own fgets. May need to use it for data as well.
*/
/*
   for unix and mac this allows files created on any machine to be
   read on any machine
*/
CHAR* hoc_fgets_unlimited(HocStr* bufstr, NrnFILEWrap* f) {
    return fgets_unlimited_nltrans(bufstr, f, 1);
}

static CHAR* fgets_unlimited_nltrans(HocStr* bufstr, NrnFILEWrap* f, int nltrans) {
    int c, i;
    int nl1, nl2;
    if (!f) {
        hoc_execerr_ext("No file (or stdin) for input");
    }
    if (nltrans) {
        nl1 = 26;
        nl2 = 4;
    } else {
        nl1 = nl2 = EOF;
    }
    for (i = 0;; ++i) {
        c = nrn_fw_getc(f);
        if (c == EOF || c == nl1 || c == nl2) { /* ^Z and ^D are end of file */
            /* some editors don't put a newline at last line */
            if (i > 0) {
                nrn_fw_ungetc(c, f);
                c = '\n';
            } else {
                break;
            }
        }
        if (c == '\r') {
            int c2 = nrn_fw_getc(f);
            if (c2 != '\n') {
                nrn_fw_ungetc(c2, f);
            }
            c = '\n';
        }
        if (bufstr->size <= i) {
            hocstr_resize(bufstr, bufstr->size * 2);
        }
        bufstr->buf[i] = c;
        if (c == '\n') {
            bufstr->buf[i + 1] = '\0';
            return bufstr->buf;
        }
    }
    return (CHAR*) 0;
}

int hoc_get_line(void) { /* supports re-entry. fill hoc_cbuf with next line */
    if (*hoc_ctp) {
        hoc_execerror("Internal error:", "Not finished with previous input line");
    }
    hoc_ctp = hoc_cbuf = hoc_cbufstr->buf;
    *hoc_ctp = '\0';
    if (hoc_pipeflag == 3) {
        nrn_inputbuf_getline();
        if (*hoc_ctp == '\0') {
            return EOF;
        }
    } else if (hoc_pipeflag) {
        if (hoc_strgets_need() > hoc_cbufstr->size) {
            hocstr_resize(hoc_cbufstr, hoc_strgets_need() + 100);
        }
        if (hoc_strgets(hoc_cbuf, CBUFSIZE - 1) == (char*) 0) {
            return EOF;
        }
    } else {
#if READLINE
        if (nrn_fw_eq(hoc_fin, stdin) && nrn_istty_) {
            char* line;
            int n;
#if INTERVIEWS
#ifdef MINGW
            if (hoc_usegui) {
                if (hoc_interviews && !hoc_in_yyparse) {
                    rl_getc_function = getc_hook;
                    hoc_notify_value();
                } else {
                    rl_getc_function = rl_getc;
                }
            }
#else /* not MINGW */
#if defined(use_rl_getc_function)
            if (hoc_interviews && !hoc_in_yyparse) {
                rl_getc_function = getc_hook;
                hoc_notify_value();
            } else {
                rl_getc_function = getc_hook;
            }
#else  /* not use_rl_getc_function */
            if (hoc_interviews && !hoc_in_yyparse) {
                rl_event_hook = event_hook;
                hoc_notify_value();
            } else {
                rl_event_hook = NULL;
            }
#endif /* not use_rl_getc_function */
#endif /* not MINGW */
#endif /* INTERVIEWS */
            if ((line = readline(hoc_promptstr)) == (char*) 0) {
                extern int hoc_notify_stop;
                return EOF;
            }
            n = strlen(line);
            for (int i = 0; i < n; ++i) {
                if (!isascii(line[i])) {
                    hoc_execerr_ext("Non-ASCII character value 0x%hhx at input position %d\n",
                                    (unsigned) line[i],
                                    i);
                }
            }
            if (n >= hoc_cbufstr->size - 3) {
                hocstr_resize(hoc_cbufstr, n + 100);
                hoc_ctp = hoc_cbuf = hoc_cbufstr->buf;
            }
            strcpy(hoc_cbuf, line);
            hoc_cbuf[n] = '\n';
            hoc_cbuf[n + 1] = '\0';
            if (line && *line) {
                add_history(line);
            }
            free(line);
            hoc_audit_command(hoc_cbuf);
        } else {
            fflush(stdout);
            if (hoc_fgets_unlimited(hoc_cbufstr, hoc_fin) == (CHAR*) 0) {
                return EOF;
            }
        }
#else  // READLINE
#if INTERVIEWS
        if (nrn_fw_eq(hoc_fin, stdin) && hoc_interviews && !hoc_in_yyparse) {
            run_til_stdin());
        }
#endif  // INTERVIEWS
#if defined(WIN32)
        if (nrn_fw_eq(hoc_fin, stdin)) {
            if (gets(hoc_cbuf) == (char*) 0) {
                /*DebugMessage("gets returned NULL\n");*/
                return EOF;
            }
            strcat(hoc_cbuf, "\n");
        } else
#endif  // WIN32
        {
            if (hoc_fgets_unlimited(hoc_cbufstr, hoc_fin) == (char*) 0) {
                return EOF;
            }
        }
#endif  // READLINE
    }
    errno = 0;
    hoc_lineno++;
    hoc_ctp = hoc_cbuf = hoc_cbufstr->buf;
    hoc_ictp = 0;
    return 1;
}

void hoc_help(void) {
#if INTERVIEWS
    if (hoc_interviews) {
        ivoc_help(hoc_cbuf);
    } else
#endif
    {
        if (hoc_usegui) {
            hoc_warning("Help only available from version with ivoc library", 0);
        }
    }
    hoc_ctp = hoc_cbuf + strlen(hoc_cbuf) - 1;
}
