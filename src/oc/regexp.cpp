#include <../../nrnconf.h>
/* /local/src/master/nrn/src/oc/regexp.cpp,v 1.1.1.1 1994/10/12 17:22:13 hines Exp */
/*
regexp.cpp,v
 * Revision 1.1.1.1  1994/10/12  17:22:13  hines
 * NEURON 3.0 distribution
 *
 * Revision 2.19  93/02/02  10:34:37  hines
 * static functions declared before used
 *
 * Revision 1.3  92/07/31  12:11:31  hines
 * following merged from hoc
 * The regular expression has been augmented with
 * {istart-iend} where istart and iend are integers. The expression matches
 * any integer that falls in this range.
 *
 * Revision 1.2  92/01/30  08:17:19  hines
 * bug fixes found in hoc incorporated. if()return, no else, objectcenter
 * warnings.
 *
 * Revision 1.1  91/10/11  11:12:16  hines
 * Initial revision
 *
 * Revision 3.108  90/10/24  09:44:14  hines
 * saber warnings gone
 *
 * Revision 3.58  90/05/17  16:30:52  jamie
 * changed global functions to start with hoc_
 * moved regexp.cpp from project 'neuron' to 'hoc'
 *
 * Revision 1.25  89/08/31  10:28:46  mlh
 * regular expressions for issection()
 * differences between standard regular expressions are:
 * allways match from beginning to end of target (implicit ^ and $)
 * change [] to <>
 * eliminate \(
 *
 * Revision 1.2  89/08/31  09:22:17  mlh
 * works as in e.cpp and lint free
 *
 * Revision 1.1  89/08/31  08:24:59  mlh
 * Initial revision
 *
*/

/* regular expression match for section names
   grabbed prototype from e.cpp
   Use by first compiling the search string with hoc_regexp_compile(pattern)
   Then checking target strings one at a time with hoc_regexp_search(target)
*/

#include <stdio.h>
#include "hocdec.h"
#define CABLESECTION 1
/* Always match from beginning of string (implicit ^),
   Always match end of string (implicit $),
   change [] to <>,
   eliminate \(
*/

#define STAR  01
#define SUFF  '.'
#define TILDE '~'

#define EREGEXP     24
#define error(enum) hoc_execerror("search string format error", pattern)
#define CBRA        1
#define CCHR        2
#define CDOT        4
#define CCL         6
#define NCCL        8
#define CDOL        10
#define CEOF        11
#define CKET        12
#if CABLESECTION
#define INTRANGE 14
#endif
#define NBRA  5
#define ESIZE 256
#define eof   '\0'
static char expbuf[ESIZE + 4];
static const char* pattern = "";
static char* loc1;
static char* loc2;
static char* locs;
static char* braslist[NBRA];
static char* braelist[NBRA];
static int circfl;
#if CABLESECTION
static int int_range_start[NBRA];
static int int_range_stop[NBRA];
#endif

static int advance(char* lp, char* ep);
static int hoc_cclass(char* set, char c, int af);

void hoc_regexp_compile(const char* pat) {
    char* cp = (char*) pat;
    int c;
    char* ep;
    char* lastep = 0;
#if (!CABLESECTION)
    char bracket[NBRA], *bracketp;
    int nbra;
#else
    int int_range_index = 0;
#endif
    int cclcnt;
    int tempc;


    if (!cp) {
        pattern = "";
        error(EREGEXP);
    }
    if (pattern == cp && strcmp(pattern, cp)) {
        /* if previous pattern != cp then may have been freed */
        return;
    }
    pattern = cp;
    ep = expbuf;
#if (!CABLESECTION)
    bracketp = bracket;
    nbra = 0;
#endif
    if ((c = *cp++) == '\n') {
        cp--;
        c = eof;
    }
    if (c == eof) {
        if (*ep == 0)
            error(EREGEXP);
        return;
    }
#if CABLESECTION
    circfl = 1;
#else
    circfl = 0;
    if (c == '^') {
        c = *cp++;
        circfl++;
    }
#endif
    if (c == '*')
        goto cerror;
    cp--;
    for (;;) {
        if (ep >= &expbuf[ESIZE])
            goto cerror;
        c = *cp++;
        if (c == '\n') {
            cp--;
            c = eof;
        }
        if (c == eof) {
#if CABLESECTION
            *ep++ = CDOL;
#endif
            *ep++ = CEOF;
            return;
        }
        if (c != '*')
            lastep = ep;
        switch (c) {
        case '\\':
#if (!CABLESECTION)
            if ((c = *cp++) == '(') {
                if (nbra >= NBRA)
                    goto cerror;
                *bracketp++ = nbra;
                *ep++ = CBRA;
                *ep++ = nbra++;
                continue;
            }
            if (c == ')') {
                if (bracketp <= bracket)
                    goto cerror;
                *ep++ = CKET;
                *ep++ = *--bracketp;
                continue;
            }
#endif
            *ep++ = CCHR;
            if (c == '\n')
                goto cerror;
            *ep++ = c;
            continue;

        case '.':
            *ep++ = CDOT;
            continue;

        case '\n':
            goto cerror;

        case '*':
            if (*lastep == CBRA || *lastep == CKET)
                error(EREGEXP);
            *lastep |= STAR;
            continue;

#if (!CABLESECTION)
        case '$':
            tempc = *cp;
            if (tempc != eof && tempc != '\n')
                goto defchar;
            *ep++ = CDOL;
            continue;
#endif

#if CABLESECTION
        case '{': {
            char* cp1 = cp;
            if (int_range_index >= NBRA)
                goto cerror;
            *ep++ = INTRANGE;
            do {
                if (!(*cp >= '0' && *cp <= '9') && *cp != '-') {
                    error(EREGEXP);
                }
            } while (*(++cp) != '}');
            cp++;
            if (2 != sscanf(cp1,
                            "%d-%d",
                            int_range_start + int_range_index,
                            int_range_stop + int_range_index)) {
                error(EREGEXP);
            }
            *ep++ = int_range_index++;
        }
            continue;
#endif
#if CABLESECTION
        case '<':
#else
        case '[':
#endif
            *ep++ = CCL;
            *ep++ = 0;
            cclcnt = 1;
            if ((c = *cp++) == '^') {
                c = *cp++;
                ep[-2] = NCCL;
            }
            do {
                if (c == '\n')
                    goto cerror;
                /*
                 *	Handle the escaped '-'
                 */
                if (c == '-' && *(ep - 1) == '\\')
                    *(ep - 1) = '-';
                /*
                 *	Handle ranges of characters (e.g. a-z)
                 */
                else if ((tempc = *cp++) != ']' && c == '-' && cclcnt > 1 && tempc != '\n' &&
                         (c = *(ep - 1)) <= tempc) {
                    while (++c <= tempc) {
                        *ep++ = c;
                        cclcnt++;
                        if (ep >= &expbuf[ESIZE])
                            goto cerror;
                    }
                }
                /*
                 *	Normal case.  Add character to buffer
                 */
                else {
                    cp--;
                    *ep++ = c;
                    cclcnt++;
                    if (ep >= &expbuf[ESIZE])
                        goto cerror;
                }
#if CABLESECTION
            } while ((c = *cp++) != '>');
#else

            } while ((c = *cp++) != ']');
#endif
            lastep[1] = cclcnt;
            continue;

#if (!CABLESECTION)
        defchar:
#endif
        default:
            *ep++ = CCHR;
            *ep++ = c;
        }
    }
cerror:
    expbuf[0] = 0;
    error(EREGEXP);
}

int hoc_regexp_search(const char* tar) { /*return true if target matches pattern*/
    char* target = (char*) tar;
    char *p1, *p2, c;

#if 1
    if (target == (char*) 0) {
        return (0);
    }
    p1 = target;
    locs = (char*) 0;
#else /* in e, apparently for searches within or at begining of string */
    if (gf) {
        if (circfl)
            return (0);
        p1 = linebuf;
        p2 = genbuf;
        while (*p1++ = *p2++)
            ;
        locs = p1 = loc2;
    } else {
        if (addr == zero)
            return (0);
        p1 = getline(*addr);
        locs = NULL;
    }
#endif
    p2 = expbuf;
    if (circfl) {
        loc1 = p1;
        return (advance(p1, p2));
    }
    /* fast check for first character */
    if (*p2 == CCHR) {
        c = p2[1];
        do {
            if (*p1 != c)
                continue;
            if (advance(p1, p2)) {
                loc1 = p1;
                return (1);
            }
        } while (*p1++);
        return (0);
    }
    /* regular algorithm */
    do {
        if (advance(p1, p2)) {
            loc1 = p1;
            return (1);
        }
    } while (*p1++);
    return (0);
}

static int advance(char* lp, char* ep) {
    char* curlp;

    for (;;)
        switch (*ep++) {
        case CCHR:
            if (*ep++ == *lp++)
                continue;
            return (0);

        case CDOT:
            if (*lp++)
                continue;
            return (0);

        case CDOL:
            if (*lp == 0)
                continue;
            return (0);

        case CEOF:
            loc2 = lp;
            return (1);

#if CABLESECTION
        case INTRANGE: {
            int start, stop, num;
            start = int_range_start[*ep];
            stop = int_range_stop[*ep++];
            num = *lp++ - '0';
            if (num < 0 || num > 9) {
                return (0);
            }
            while (*lp >= '0' && *lp <= '9') {
                num = 10 * num + *lp - '0';
                ++lp;
            }
            if (num >= start && num <= stop) {
                continue;
            }
        }
            return (0);
#endif

        case CCL:
            if (hoc_cclass(ep, *lp++, 1)) {
                ep += *ep;
                continue;
            }
            return (0);

        case NCCL:
            if (hoc_cclass(ep, *lp++, 0)) {
                ep += *ep;
                continue;
            }
            return (0);

        case CBRA:
            braslist[*ep++] = lp;
            continue;

        case CKET:
            braelist[*ep++] = lp;
            continue;

        case CDOT | STAR:
            curlp = lp;
            /*EMPTY*/
            while (*lp++)
                ;
            goto star;

        case CCHR | STAR:
            curlp = lp;
            /*EMPTY*/
            while (*lp++ == *ep)
                ;
            ep++;
            goto star;

        case CCL | STAR:
        case NCCL | STAR:
            curlp = lp;
            /*EMPTY*/
            while (hoc_cclass(ep, *lp++, ep[-1] == (CCL | STAR)))
                ;
            ep += *ep;
            goto star;

        star:
            do {
                lp--;
                if (lp == locs)
                    break;
                if (advance(lp, ep))
                    return (1);
            } while (lp > curlp);
            return (0);

        default:
            error(EREGEXP);
        }
}

static int hoc_cclass(char* set, char c, int af) {
    int n;

    if (c == 0)
        return (0);
    n = *set++;
    while (--n)
        if (*set++ == c)
            return (af);
    return (!af);
}
