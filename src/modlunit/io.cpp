#include <../../nmodlconf.h>
/* /local/src/master/nrn/src/modlunit/io.c,v 1.2 1997/11/24 16:19:09 hines Exp */

/* file.mod input routines */
#include <stdlib.h>
#include "model.h"
#include <ctype.h>
#undef METHOD
#include "parse1.hpp"
Item* lastok; /*should be last token accepted by parser that gives
successful reduction */

/*
 * We painfully constuct our own input buffer so that when user errors occur
 * we can print the whole line. Often, even this is not enough if the error
 * is at the end of a line.  We also count lines so the user can go right to
 * the error in most cases
 */
static int linenum = 0;
static char inlinebuf[600], *inlinep = inlinebuf + 30, *ctp = inlinebuf + 30;
static int file_stack_empty();

char* Fgets(char* buf, int size, FILE* f) {
    char* p = buf;
    int c, i;
    for (i = 0; i < size; ++i) {
        c = getc(f);
        if (c == EOF || c == 26 || c == 4) { /* ^Z and ^D are end of file */
            /* some editors don't put a newline at last line */
            if (p > buf) {
                ungetc(c, f);
                c = '\n';
            } else {
                break;
            }
        }
        if (c == '\r') {
            int c2 = getc(f);
            if (c2 != '\n') {
                ungetc(c2, f);
            }
            c = '\n';
        }
        *p++ = c;
        if (c == '\n') {
            *p = '\0';
            return buf;
        }
    }
    if (i >= size) {
        buf[size - 1] = 0;
        diag("Line too long:", buf);
    }
    return (char*) 0;
}

int Getc() {
    int c;
    if (ctp == (char*) 0 || *ctp == '\0') {
        ctp = Fgets(inlinep, 512, fin);
        if (ctp)
            linenum++;
    }
    if (ctp == (char*) 0) {
        ctp = inlinep;
        *ctp = '\0';
        if (file_stack_empty()) {
            return EOF;
        } else {
            pop_file_stack();
            return Getc();
        }
    }
    c = *ctp++;
    return c;
}

int unGetc(int c) {
    if (c == EOF)
        return c;
    if (ctp > inlinebuf) {
        ctp--;
        *ctp = c;
    } else {
        diag("internal error in unGetc", "");
    }
    return c;
}

char* Gets(char* buf) {
    char* cp;
    int c;

    cp = buf;
    while ((c = Getc()) != EOF && c != '\n') {
        *cp++ = c;
    }
    if (c == '\n') {
        *cp++ = c;
        *cp++ = '\0';
        return buf;
    } else if (c == EOF) {
        return (char*) 0;
    } else {
        diag("internal error in Gets()", "");
    }
    return (char*) 0;
}

/* two arguments so we can pass a name to construct an error message. */
void diag(const char* s1, const char* s2) {
    char* cp;
    Item *q1, *q2, *q;

    Fprintf(stderr, "%s", s1);
    if (s2) {
        Fprintf(stderr, "%s", s2);
    }
    if (lex_tok) {
        /*EMPTY*/
        for (q2 = lastok; q2->itemtype != NEWLINE && q2 != intoken; q2 = q2->next) {
            ;
        }
        /*EMPTY*/
        for (q1 = lastok->prev; q1->itemtype != NEWLINE && q1 != intoken; q1 = q1->prev) {
            ;
        }
        if (q2 == intoken) {
            Fprintf(stderr, " at end of file in file %s\n", finname);
        } else {
            Fprintf(stderr, " at line %d in file %s\n", q2->itemsubtype, finname);
        }
        assert(q1 != q2);
        for (q = q1->next; q != q2; q = q->next) {
            switch (q->itemtype) {
            case SYMBOL:
                Fprintf(stderr, "%s", SYM(q)->name);
                break;
            case STRING:
                Fprintf(stderr, "%s", STR(q));
                break;
            case NEWLINE:
                Fprintf(stderr, "\n");
                break;
            default:
                /*SUPPRESS 622*/
                assert(0);
            }
            if (q == lastok) {
                Fprintf(stderr, "<<ERROR>>");
            }
        }
    } else if (fin) {
        Fprintf(stderr, " at line %d in file %s\n", linenum, finname);
        Fprintf(stderr, "%s", inlinep);
        if (ctp >= inlinep) {
            for (cp = inlinep; cp < ctp - 1; cp++) {
                if (*cp == '\t') {
                    Fprintf(stderr, "\t");
                } else {
                    Fprintf(stderr, " ");
                }
            }
            Fprintf(stderr, "^");
        }
    }
    Fprintf(stderr, "\n");
    exit(1);
}

Symbol* _SYM(Item* q, char* file, int line) {
    if (!q || q->itemtype != SYMBOL) {
        internal_error(q, file, line);
    }
    return (Symbol*) ((q)->element);
}

char* _STR(Item* q, char* file, int line) {
    if (!q || q->itemtype != STRING) {
        internal_error(q, file, line);
    }
    return (char*) ((q)->element);
}

Item* _ITM(Item* q, char* file, int line) {
    if (!q || q->itemtype != ITEM) {
        internal_error(q, file, line);
    }
    return (Item*) ((q)->element);
}

Item** _ITMA(Item* q, char* file, int line) {
    if (!q || q->itemtype != ITEMARRAY) {
        internal_error(q, file, line);
    }
    return (Item**) ((q)->element);
}

List* _LST(Item* q, char* file, int line) {
    if (!q || q->itemtype != LIST) {
        internal_error(q, file, line);
    }
    return (List*) ((q)->element);
}

void internal_error(Item* q, char* file, int line) {
    Fprintf(stderr, "Internal error in file \"%s\", line %d\n", file, line);
    Fprintf(stderr, "The offending item has the structure:\n");
    debugitem(q);
    exit(1);
}


typedef struct FileStackItem {
    char* inlinep;
    char* ctp;
    int linenum;
    FILE* fp;
    char finname[NRN_BUFSIZE];
} FileStackItem;

static List* filestack;

static int getprefix(char* prefix, char* s) {
    char* cp;
    strcpy(prefix, s);
    for (cp = prefix + strlen(prefix); cp + 1 != prefix; --cp) {
        if (*cp == '/') {
            break;
        }
        *cp = '\0';
    }
    return (prefix[0] != '\0');
}

static FILE* include_open(char* fname, int err) {
    FILE* f = (FILE*) 0;
    FileStackItem* fsi;
    char *dirs, *colon;
    char buf2[200];
    if (fname[0] == '/') { /* highest precedence is complete filename */
        return fopen(fname, "r");
    }

    fsi = (FileStackItem*) (SYM(filestack->prev));
    if (getprefix(buf, fsi->finname)) {
        strcat(buf, fname);
        f = fopen(buf, "r"); /* first try in directory of last file */
        if (f) {
            strcpy(fname, buf);
            return f;
        }
        if (err)
            fprintf(stderr, "Couldn't open: %s\n", buf);
    }
    f = fopen(fname, "r"); /* next try current working directory */
    if (f) {
        return f;
    }
    Sprintf(buf, "../%s", fname); /* Next try next dir up. */
    if ((f = fopen(buf, "r")) != NULL)
        return f;

    if (err)
        fprintf(stderr, "Couldn't open: %s\n", fname);
    /* try all the directories in the environment variable */
    /* a colon separated list of directories */
    dirs = getenv("MODL_INCLUDE");
    if (dirs) {
        strcpy(buf, dirs);
        dirs = buf;
        colon = dirs;
        for (dirs = colon; *dirs; dirs = colon) {
            for (; *colon; ++colon) {
                if (*colon == ':') {
                    *colon = '\0';
                    ++colon;
                    break;
                }
            }
            strcpy(buf2, dirs);
            strcat(buf2, "/");
            strcat(buf2, fname);
            f = fopen(buf2, "r");
            if (f) {
                strcpy(fname, buf2);
                return f;
            }
            if (err)
                fprintf(stderr, "Couldn't open: %s\n", buf2);
        }
    }
    return f;
}

void include_file(Item* q) {
    char fname[200];
    FileStackItem* fsi;
    if (!filestack) {
        filestack = newlist();
    }
    strcpy(fname, STR(q) + 1);
    fname[strlen(fname) - 1] = '\0';
    fsi = (FileStackItem*) emalloc(sizeof(FileStackItem));
    lappendsym(filestack, (Symbol*) fsi);

    fsi->inlinep = inlinep;
    fsi->ctp = ctp;
    fsi->linenum = linenum;
    fsi->fp = fin;
    strcpy(fsi->finname, finname);

    if ((fin = include_open(fname, 0)) == (FILE*) 0) {
        include_open(fname, 1);
        diag("Couldn't open ", fname);
    }
    fprintf(stderr, "INCLUDEing %s\n", fname);
    strcpy(finname, fname);
    ctp = (char*) 0;
    linenum = 0;
}

void pop_file_stack() {
    FileStackItem* fsi;
    fsi = (FileStackItem*) (SYM(filestack->prev));
    remove(filestack->prev);
    linenum = fsi->linenum;
    inlinep = fsi->inlinep;
    fclose(fin);
    fin = fsi->fp;
    strcpy(finname, fsi->finname);
    free(fsi);
}

static int file_stack_empty() {
    if (!filestack) {
        return 1;
    }
    return (filestack->next == filestack);
}

/* io.c,v
 * Revision 1.2  1997/11/24  16:19:09  hines
 * modlunit port to mac (not complete)
 *
 * Revision 1.1.1.1  1994/10/12  17:22:48  hines
 * NEURON 3.0 distribution
 *
 * Revision 1.7  1994/09/20  14:43:18  hines
 * port to dec alpha
 *
 * Revision 1.6  1994/05/23  17:56:02  hines
 * error in handling MODL_INCLUDE when no file exists
 *
 * Revision 1.5  1994/05/18  18:08:13  hines
 * INCLUDE "file"
 * tries originalpath/file ./file MODL_INCLUDEpaths/file
 *
 * Revision 1.4  1993/02/01  15:15:45  hines
 * static functions should be declared before use
 *
 * Revision 1.3  92/02/17  12:30:55  hines
 * constant states with a compartment size didn't have space allocated
 * to store the compartment size.
 *
 * Revision 1.2  91/01/07  14:17:06  hines
 * in kinunit, wrong itemsubtype.  Fix lint messages
 *
 * Revision 1.1  90/11/13  16:10:17  hines
 * Initial revision
 *  */
