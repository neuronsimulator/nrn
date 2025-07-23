#include <../../nrnconf.h>

#ifdef WIN32

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <windows.h>
#ifdef _MSC_VER
#include <process.h>  // _getpid
#endif
#include <filesystem>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "hoc.h"
#include "../mswin/extra/d2upath.h"

#include "gui-redirect.h"

extern char* neuron_home;
extern char* neuron_home_dos;
extern void hoc_quit();

static HCURSOR wait_cursor;
static HCURSOR old_cursor;
#if HAVE_IV
extern int bad_install_ok;
#else
int bad_install_ok;
#endif  // HAVE_IV
void setneuronhome(const char* p) {
    // if the program lives in .../bin/neuron.exe
    // and .../lib exists then use ... as the
    // NEURONHOME
    const auto executable = std::filesystem::path(p);
    // Windows defaults to wchar for paths: go through std::string
    neuron_home = strdup((executable.parent_path().parent_path() / "share" / "nrn").string().c_str());
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

static FILE* dll_stdio_[] = {(FILE*) 0x0, (FILE*) 0x20, (FILE*) 0x40};

void nrn_mswindll_stdio(FILE* i, FILE* o, FILE* e) {
    if (o != dll_stdio_[1]) {
        printf("nrn_mswindll_stdio stdio in dll = %p but expected %p\n", o, dll_stdio_[1]);
    }
    dll_stdio_[0] = i;
    dll_stdio_[1] = o;
    dll_stdio_[2] = e;
}

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

#endif  // WIN32
