#include <../../nmodlconf.h>

/* file.mod input routines */
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <errno.h>
#include "modl.h"
#include <ctype.h>
#undef METHOD
#include "parse1.hpp"
#if defined(_WIN32)
#include <direct.h>
#endif

int isend(char*, char*);
static void pop_file_stack();
static int file_stack_empty();
int in_comment_;

char* inputline() {
    /* and removes comment, newline, beginning and trailing blanks */
    /* used to get the TITLE line */
#if SYSV || defined(MINGW)
#define index strchr
#endif
    char* cp;
    int i;

    buf[0] = '\0';
    cp = Gets(buf);
    i = strlen(buf);
    if (i)
        buf[i - 1] = '\0';
    if ((cp = index(buf, '!')) != (char*) 0) {
        *cp-- = '\0';
    }
    while (cp >= buf && isspace(*cp)) {
        *cp-- = '\0';
    }
    /*EMPTY*/
    for (cp = buf; *cp != '\0' && isspace(*cp); cp++) {
        ;
    }
    return stralloc(cp, (char*) 0);
}

static int linenum = 0;

void inblock(char* s) { /* copy input verbatim to intoken up to END*s
                         * error if we get the whole input */
    char* cp;
    int l;
    Item* q;

    l = linenum;
    for (;;) {
        cp = Gets(buf);
        if (cp == (char*) 0) {
            linenum = l;
            diag(s, "block goes to end of file");
        }
        if (isend(s, buf)) {
            break;
        }
        q = putintoken(buf, STRING);
        q->itemtype = VERBATIM;
    }
}

int isend(char* s, char* buf) {
    /* if first chars in buf form a keyword return 1 */
    char *cp, word[256], *wp, test[256];
    int yesno = 0;

    cp = buf;
    Sprintf(test, "END%s", s);
    while (*cp == ' ' || *cp == '\t')
        cp++;
    if (isalpha(*cp)) {
        for (wp = word; isalpha(*cp);) {
            *wp++ = *cp++;
        }
        *wp = '\0';
        if (strcmp(test, word) == 0) {
            yesno = 1;
        }
    }
    return yesno;
}

/*
 * We painfully constuct our own input buffer so that when user errors occur
 * we can print the whole line. Often, even this is not enough if the error
 * is at the end of a line.  We also count lines so the user can go right to
 * the error in most cases
 */
static char inlinebuf[2][NRN_BUFSIZE], *inlinep = inlinebuf[0] + 30, *ctp = inlinebuf[0] + 30;
static int whichbuf;

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
        if (c < 0 || c > 127) {
            *p++ = '\n';
            *p = '\0';
            if (!in_comment_) {
                diag("Non-Ascii character in file:", buf);
            }
            lappendstr(filetxtlist, buf);
            return buf;
        }
        *p++ = c;
        if (c == '\n') {
            *p = '\0';
            lappendstr(filetxtlist, buf);
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
        whichbuf = (whichbuf ? 0 : 1);
        inlinep = inlinebuf[whichbuf] + 30;
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
    if (ctp > inlinebuf[whichbuf]) {
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

#if 0 /* not currently used */
void unGets(char* buf)		/* all this because we don't have an ENDBLOCK
				 * keyword */
{
	if (ctp != '\0') {	/* can only be called after successful Gets */
		Strcpy(inlinep, buf);
		ctp = inlinep;
	} else {
		diag("internal error in unGets()", "");
	}
}
#endif

char* current_line() { /* assumes we actually want the previous line */
    static char buf[NRN_BUFSIZE];
    char* p;
    Sprintf(
        buf, "at line %d in file %s:\\n%s", linenum - 1, finname, inlinebuf[whichbuf ? 0 : 1] + 30);
    for (p = buf; *p; ++p) {
        if (*p == '\n') {
            *p = '\0';
        }
        if (*p == '"') {
            *p = '\047';
        }
    }
    return buf;
}

/* two arguments so we can pass a name to construct an error message. */
void diag(const char* s1, const char* s2) {
    char* cp;
    Fprintf(stderr, "Error: %s", s1);
    if (s2) {
        Fprintf(stderr, "%s", s2);
    }
    if (fin) {
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

#if 0
static Symbol  *symq[20], **symhead = symq, **symtail = symq;

/*
 * the following is a nonsensical implementation of heirarchical model
 * building. Disregard. It assumes .mod files can be concatenated to produce
 * meaningful models.  It was this insanity which prompted us to allow use of
 * variables before declaration 
 */
void enquextern(Symbol* sym)
{
	*symtail++ = sym;
}

FILE *dequextern()
{
	char            fname[256];
	FILE           *f;
	Symbol         *s;

	if (symhead >= symtail)
		return (FILE *) 0;
	s = *symhead++;
	Sprintf(fname, "%s.mod", s->name);
	f = fopen(fname, "r");
	if (f == (FILE *) 0) {
		diag("Can't open", fname);
	}
	Fclose(fin);
	linenum = 0;
	Strcpy(finname, fname);
	return f;
}
#endif

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
    /* since dirs is a ':' separated list of paths, there is no
       limit to the size and so allocate from size of dirs and free
    */
    char *buf, *buf2;
    if (fname[0] == '/') { /* highest precedence is complete filename */
        return fopen(fname, "r");
    }

    fsi = (FileStackItem*) (SYM(filestack->prev));
    buf = static_cast<char*>(emalloc(NRN_BUFSIZE));
    if (getprefix(buf, fsi->finname)) {
        strcat(buf, fname);
        f = fopen(buf, "r"); /* first try in directory of last file */
        if (f) {
            strcpy(fname, buf);
            free(buf);
            return f;
        }
        if (err)
            fprintf(stderr, "Couldn't open: %s\n", buf);
    }
    f = fopen(fname, "r"); /* next try current working directory */
    if (f) {
        free(buf);
        return f;
    }
    std::snprintf(buf, NRN_BUFSIZE, "../%s", fname); /* Next try next dir up. */
    if ((f = fopen(buf, "r")) != NULL) {
        strcpy(fname, buf);
        free(buf);
        return f;
    }

    if (err)
        fprintf(stderr, "Couldn't open: %s\n", fname);
    /* try all the directories in the environment variable */
    /* a colon separated list of directories */
    dirs = getenv("MODL_INCLUDE");
    if (dirs) {
        buf = stralloc(dirs, buf); /* frees old buf and allocates */
        dirs = buf;
        colon = dirs;
        for (dirs = colon; *dirs; dirs = colon) {
            buf2 = NULL;
            for (; *colon; ++colon) {
                if (*colon == ':') {
                    *colon = '\0';
                    ++colon;
                    break;
                }
            }
            buf2 = static_cast<char*>(emalloc(strlen(dirs) + 2 + strlen(fname)));
            strcpy(buf2, dirs);
            strcat(buf2, "/");
            strcat(buf2, fname);
            f = fopen(buf2, "r");
            if (f) {
                strcpy(fname, buf2);
                free(buf);
                free(buf2);
                return f;
            }
            if (err)
                fprintf(stderr, "Couldn't open: %s\n", buf2);
            free(buf2);
        }
        free(buf);
    }
    return f;
}

void include_file(Item* q) {
    char* pf = NULL;
    char fname[NRN_BUFSIZE];
    Item* qinc;
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

    strcpy(finname, fname);
    if ((fin = include_open(fname, 0)) == (FILE*) 0) {
        include_open(fname, 1);
        diag("Couldn't open ", fname);
    }
    fprintf(stderr, "INCLUDEing %s\n", fname);
    ctp = (char*) 0;
    linenum = 0;

    qinc = filetxtlist->prev;
    Sprintf(buf, ":::%s", STR(qinc));
    replacstr(qinc, buf);
#if HAVE_REALPATH
    pf = realpath(fname, NULL);
#endif
    if (pf) {
        Sprintf(buf, ":::realpath %s\n", pf);
        free(pf);
        lappendstr(filetxtlist, buf);
    }
}

static void pop_file_stack() {
    Sprintf(buf, ":::end INCLUDE %s\n", finname);
    lappendstr(filetxtlist, buf);
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
