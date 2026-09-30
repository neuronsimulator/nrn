#include <../../nrnconf.h>

#ifdef _WIN32

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <string>
#include "hoc.h"
#ifdef MINGW
#include "../mswin/extra/d2upath.h"
#endif
#include "nrn_windows_home.hpp"

#include "gui-redirect.h"

extern char* neuron_home;
extern char* neuron_home_dos;
extern void hoc_quit();

static HCURSOR wait_cursor;
static HCURSOR old_cursor;
#if HAVE_IV
extern "C" int bad_install_ok;
#else
int bad_install_ok;
#endif  // HAVE_IV
extern FILE* hoc_redir_stdout;
char* hoc_back2forward(char* s);
void hoc_forward2back(char* s);

static char* nrn_win_dup_path(const std::string& s) {
    char* p = static_cast<char*>(emalloc(s.size() + 1));
    memcpy(p, s.c_str(), s.size() + 1);
    return p;
}

void setneuronhome(const char* p) {
    bad_install_ok = 1;
    if (!neuron_home) {
        auto const home =
            nrn_win_neuronhome_from_symbol(reinterpret_cast<const void*>(&setneuronhome), p);
        if (home.empty()) {
            return;
        }
        std::string s = home.string();
#ifdef MINGW
        neuron_home = hoc_dos2unixpath(s.c_str());
#else
        neuron_home = nrn_win_dup_path(s);
        hoc_back2forward(neuron_home);
#endif
    }
    if (!neuron_home_dos && neuron_home) {
        neuron_home_dos = nrn_win_dup_path(neuron_home);
        hoc_forward2back(neuron_home_dos);
    }
}
void HandleOutput(char* s) {
    printf("%s", s);
}
static long exception_filter(LPEXCEPTION_POINTERS p) {
    //	hoc_execerror("unhandled exception", "");
    //	return EXCEPTION_CONTINUE_EXECUTION;
    static int n = 0;
    ++n;
    if (n == 1) {
        hoc_execerror(
            "\nUnhandled Exception. This usually means a bad memory \n\
address.",
            "It is not possible to make a judgment as to whether it is safe\n\
to continue. If this happened while compiling a template, you will have to\n\
quit.");
    }
    if (n == 2) {
        MessageBox(NULL,
                   "Second Unhandled Exception: Quitting NEURON. You will be asked to save \
any unsaved em buffers before exiting.",
                   "NEURON Internal ERROR",
                   MB_OK);
        hoc_quit();
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

void hoc_set_unhandled_exception_filter() {
    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER) exception_filter);
}
BOOL hoc_copyfile(const char* src, const char* dest) {
    return CopyFile(src, dest, FALSE);
}

#ifdef MINGW
static FILE* dll_stdio_[] = {(FILE*) 0x0, (FILE*) 0x20, (FILE*) 0x40};

void nrn_mswindll_stdio(FILE* i, FILE* o, FILE* e) {
    if (o != dll_stdio_[1]) {
        printf("nrn_mswindll_stdio stdio in dll = %p but expected %p\n", o, dll_stdio_[1]);
    }
    dll_stdio_[0] = i;
    dll_stdio_[1] = o;
    dll_stdio_[2] = e;
}
#endif  // MINGW

void hoc_forward2back(char* s) {
    char* cp;
    for (cp = s; *cp; ++cp) {
        if (*cp == '/') {
            *cp = '\\';
        }
    }
}

char* hoc_back2forward(char* s) {
    char* cp = s;
    while (*cp) {
        if (*cp == '\\') {
            *cp = '/';
        }
        ++cp;
    }
    return s;
}

#if HAVE_IV
void ivoc_win32_cleanup();
#endif


void hoc_win_exec(void) {
    int i;
    i = SW_SHOW;
    if (ifarg(2)) {
        i = (int) chkarg(2, -1000, 1000);
    }
    i = WinExec(gargstr(1), i);
    hoc_ret();
    hoc_pushx((double) i);
}

void hoc_winio_show(int b) {}

#ifdef MINGW
int getpid() {
    return 1;
}

void hoc_Plt() {
    TRY_GUI_REDIRECT_DOUBLE("plt", NULL);
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_Setcolor() {
    TRY_GUI_REDIRECT_DOUBLE("setcolor", NULL);
    hoc_ret();
    hoc_pushx(0.);
}
void hoc_Lw() {
    hoc_ret();
    hoc_pushx(0.);
}

#endif  // MINGW
#endif  // _WIN32
