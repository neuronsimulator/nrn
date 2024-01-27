#include <../../nrnconf.h>
#if HAVE_IV  // to end of file

// nrnbbs has been removed. This is no longer working.

#include <InterViews/cursor.h>
#include <InterViews/window.h>
#include <OS/list.h>
#include <OS/string.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "ivoc.h"

static FILE* help_pipe;

#if defined(WIN32) && !defined(MINGW)
#include "nrnbbs.h"
#endif

#ifndef WIN32
#define WIN32
#define UNIX 1
#endif

extern const char* hoc_current_xopen();

void ivoc_help(const char* s) {
    //	printf("online help not currently working\n");
    return;
}

static void readmore() {
#if !defined(WIN32)
    char buf[1024];
    char* cmd = "ls $NEURONHOME/doc/help/*.help";
    FILE* p = popen(cmd, "r");
    if (!p) {
        printf("couldn't do: %s\n", cmd);
        return;
    }
    while (fgets(buf, 1024, p)) {
        fprintf(help_pipe, "?0 %s", buf);
    }
#endif
}

#if !defined(WIN32)
void Oc::help(const char* s) {
    printf("online help not currently working\n");
}
#endif

#if defined(WIN32)
void Oc::help(const char* s) {}
#endif  // WIN32

void Oc::helpmode(bool b) {
    helpmode_ = b;
}

void Oc::helpmode(Window* w) {
    if (helpmode()) {
        if (w->cursor() != Oc::help_cursor()) {
            w->push_cursor();
            w->cursor(Oc::help_cursor());
        }
    } else {
        if (w->cursor() == Oc::help_cursor()) {
            w->pop_cursor();
        }
    }
}

static const CursorPattern question_pat = {0x0000,
                                           0x0000,
                                           0x0000,
                                           0x7c00,
                                           0xce00,
                                           0x0600,
                                           0x0600,
                                           0x0c00,
                                           0x3000,
                                           0x6000,
                                           0x6000,
                                           0x6000,
                                           0x0000,
                                           0x0000,
                                           0x6000,
                                           0x6000};

static const CursorPattern question_mask = {
//    0x0000, 0x0000, 0x7c00, 0xfe00, 0xff00, 0xcf00, 0x0f00, 0x3e00,
//    0x7c00, 0xf000, 0xf000, 0xf000, 0xf000, 0xf000, 0xf000, 0xf000
#if !defined(UNIX) && defined(WIN32)
    0xffff,
    0xffff,
    0xffff,
    0xffff,
    0xffff,
    0xffff,
    0xffff,
    0xffff,
    0xffff,
    0xffff,
    0xffff,
    0xffff,
    0xffff,
    0xffff,
    0xffff,
    0xffff
#else
    0xfe00,
    0xfe00,
    0xfe00,
    0xfe00,
    0xfe00,
    0xfe00,
    0xfe00,
    0xfe00,
    0xfe00,
    0xfe00,
    0xfe00,
    0xfe00,
    0xfe00,
    0xfe00,
    0xfe00,
    0xfe00
#endif
};


Cursor* Oc::help_cursor_;

Cursor* Oc::help_cursor() {
    if (!Oc::help_cursor_) {
        help_cursor_ = new Cursor(4, 7, question_pat, question_mask);
    }
    return help_cursor_;
}


#endif
