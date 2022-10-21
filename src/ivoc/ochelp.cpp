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

#if MAC
#define WIN32 1
#endif

#if defined(WIN32) && !defined(MINGW)
#include "nrnbbs.h"
#endif

#ifndef WIN32
#define WIN32
#define UNIX 1
//#include "../uxnrnbbs/nrnbbs.h"
#endif

extern const char* hoc_current_xopen();

declareList(CopyStringList, CopyString) implementList(CopyStringList, CopyString)

    static CopyStringList* filequeue;

void ivoc_help(const char* s) {
#if 1
    //	printf("online help not currently working\n");
    return;
#else
    char buf[256];
    strncpy(buf, s + 4, 256);
    char* p;
    for (p = buf; *p; ++p) {  // eliminate trailing newline
        if (*p == '\n') {
            *p = '\0';
            break;
        }
    }
    for (p = buf; *p; ++p) {  // start at first character
        if (!isspace(*p)) {
            break;
        }
    }
    // queue up the help files if haven't invoked help
    if (!help_pipe) {
        if (!filequeue) {
            filequeue = new CopyStringList();
        }
        if (strncmp(p, "?0", 2) == 0) {
            sprintf(buf, "?0 %s", hoc_current_xopen());
            String str(buf);
            filequeue->append(str);
            return;
        } else if (strncmp(p, "?1", 2) == 0) {
            filequeue->append(p);
            return;
        }
    }
    if (*p) {
        Oc::help(p);
    } else {
        Oc::help("Help_root");
    }
#endif
}

static void readmore() {
#if !defined(WIN32) && !defined(MAC)
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

#if !defined(WIN32) && !defined(MAC)
void Oc::help(const char* s) {
#if 1
    printf("online help not currently working\n");
#else
    if (help_pipe && ferror(help_pipe)) {
        printf(
            "error on the help pipe, restarting\n\
but will be missing this sessions hoc help text\n");
        pclose(help_pipe);
        help_pipe = NULL;
    }
    if (!help_pipe) {
        printf("Starting the help system\n");
        char buf[200];
        sprintf(buf, "%s/ochelp", "$NEURONHOME/bin/$CPU");
        if ((help_pipe = popen(buf, "w")) == (FILE*) 0) {
            printf("Could not start %s\n", buf);
        }
        // printf("help_pipe = %p\n", help_pipe);
        readmore();
        if (filequeue) {
            for (long i = 0; i < filequeue->count(); ++i) {
                fprintf(help_pipe, "%s\n", filequeue->item_ref(i).string());
            }
            filequeue->remove_all();
        }
    }
    if (help_pipe) {
        // printf("|%s|\n", s);
        if (strncmp(s, "?0", 2) == 0) {
            char buf[1024];
            sprintf(buf, "?0 %s", hoc_current_xopen());
            fprintf(help_pipe, "%s\n", buf);
        } else {
            fprintf(help_pipe, "%s\n", s);
        }
        fflush(help_pipe);
    }
#endif
}
#endif

#if defined(WIN32) || defined(MAC)

void Oc::help(const char* s) {
#if 0
#ifndef MINGW
	static bool ran_ochelp = false;
	char buf[1024];
	nrnbbs_connect(); // benign if already connected
	if (!nrnbbs_connected()) {
		printf("Could not connect to nrnbbs service\n");
		return;
	}
	if (!ran_ochelp && !nrnbbs_look("ochelp running")) {
		ran_ochelp = true;
		printf("Starting the help system\n");
		nrnbbs_exec("ochelp");
	}else if (!nrnbbs_look("ochelp running")) {
		printf("proper ochelp version not running\n");
		return;
	}

	readmore();
	if (filequeue) {
		for (long i = 0; i < filequeue->count(); ++i) {
sprintf(buf, "%s\n", filequeue->item_ref(i).string());
			nrnbbs_post_string("ochelp", buf);
		}
		filequeue->remove_all();
	}

	if (strncmp(s, "?0", 2) == 0) {
		sprintf(buf, "?0 %s", hoc_current_xopen());
		nrnbbs_post_string("ochelp", buf);
	}else{
		nrnbbs_post_string("ochelp", s);
	}
#endif  // MINGW
#endif
}
#endif  // WIN32 or MAC

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
#if !defined(UNIX) && (defined(WIN32) || defined(MAC))
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
