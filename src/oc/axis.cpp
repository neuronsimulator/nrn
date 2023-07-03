#include <../../nrnconf.h>
/* /local/src/master/nrn/src/oc/axis.cpp,v 1.2 1999/01/04 13:04:55 hines Exp */
/*
axis.cpp,v
 * Revision 1.2  1999/01/04  13:04:55  hines
 * fabs now from include math.h
 *
 * Revision 1.1.1.1  1994/10/12  17:22:06  hines
 * NEURON 3.0 distribution
 *
 * Revision 2.63  1993/11/04  15:55:48  hines
 * port to solaris2 (no more warnings)
 *
 * Revision 2.19  1993/02/02  10:34:03  hines
 * static functions declared before used
 *
 * Revision 1.9  92/10/29  09:19:17  hines
 * some errors in freeing objects fixed and replace usage of getarg for
 * non numbers.
 *
 * Revision 1.8  92/10/14  10:07:55  hines
 * move oc specific stuff out of axis.cpp and into code2.cpp
 * new argument function hoc_pgetarg checks for double pointer on stack
 * and returns it.
 * hoc_val_pointer(string) returns a pointer to the variable resulting
 * from parsing the string.
 *
 * Revision 1.7  92/10/09  12:13:51  hines
 * remove old style point process syntax
 * make hoc_run_expr(sym) much more general
 * added hoc_run_stmt(sym) as well
 * create them with hoc_parse_expr(char*, Symlist**) and
 * hoc_parse_stmt(char*, Symlist**)
 *
 * Revision 1.6  92/07/31  08:54:52  hines
 * following merged from hoc to oc
 * Stewart Jasloves contribution to axis labels. This can be invoked by
 * setting #define Jaslove 1. It is 0 by default. The 3rd and 6th arguments
 * of axis() may have a precision which specifies the number of digits
 * after the decimal point for axis labels. eg. 5.3 denotes 5 tic marks with
 * 3 digits after the decimal point for each tic label
 *
 *
 * Revision 1.5  92/04/15  11:22:16  hines
 * double hoc_run_expr(sym) returns value of expresssion in sym made by
 * hoc_parse_exper()
 *
 * Revision 1.4  92/04/09  12:39:52  hines
 * ready to add idplot usage with newgraph(), addgraph(), initgraph(), xgraph()
 * flushgraph().
 * A facilitating function exists called
 * Symbol* hoc_parse_expr(char* str, Symlist** psymlist) which
 * return a procedure symbol which can be used as
 * hoc_execute(sym->u.u_proc->defn.in);
 * val = hoc_xpop();
 *
 * Revision 1.3  92/03/19  08:57:01  hines
 * axis labels close to origin set to 0 so label not strange looking.
 *
 * Revision 1.2  91/10/18  14:39:40  hines
 * symbol tables now are type Symlist containing pointers to first and last
 * symbols.  New symbols get added onto the end.
 *
 * Revision 1.1  91/10/11  11:11:28  hines
 * Initial revision
 *
 * Revision 4.17  91/03/18  10:38:43  hines
 * some builtin functions take address of varname. eg. regraph(&name)
 * regraph saves list of addresses for fast plotting
 *
 * Revision 4.11  91/01/23  17:54:20  hines
 * use emalloc
 *
 * Revision 3.99  90/09/18  13:14:31  hines
 * plotx() and ploty() functions added so that we can hang absolute
 * location stuff around a scaled point.
 *
 * Revision 3.96  90/09/04  08:30:55  hines
 * try to get dos and unix together again in one RCS directory
 *
 * Revision 3.93  90/08/09  09:19:21  hines
 * axis.cpp placement of labels slightly lowered to look good with FIG
 *
 * Revision 3.83  90/07/25  10:39:56  hines
 * almost lint free on sparc 1+ under sunos 4.1
 *
 * Revision 3.82  90/07/25  08:51:10  hines
 * sun uses color in graph
 *
 * Revision 3.53  90/03/22  16:22:18  jamie
 * correct error with checkin
 *
 * Revision 3.47  90/03/22  15:22:18  jamie
 * Jamie's Additions	stopwatch and settext.
 *
 * Revision 3.46  90/01/24  06:35:48  mlh
 * emalloc() and ecalloc() are macros which return null if out of space
 * and then call execerror.  This ensures that pointers are set to null.
 * If more cleanup necessary then use hoc_Emalloc() followed by hoc_malchk()
 *
 * Revision 3.29  89/10/19  06:54:44  mlh
 * printing directed through plprint(string) instead
 * of detailed routing at each place
 *
 * Revision 3.7  89/07/13  08:20:25  mlh
 * stack functions involve specific types instead of Datum
 *
 * Revision 3.4  89/07/12  10:25:54  mlh
 * Lint free
 *
 * Revision 3.3  89/07/10  15:44:02  mlh
 * Lint pass1 is silent. Inst structure changed to union.
 *
 * Revision 2.0  89/07/07  16:01:42  mlh
 * ready to add cable stuff
 *
*/
/* version 7.1.1 12/8/88
   change graph to not interact with plot. Adding graphmode(mode).
   */
/* version 7.1.2 12/20/88
   ordinate labels not positioned correctly on laser writer because
   leading spaces not printed
   */
/* version 7.1.3 12/29/88
   sometimes doesn't return to last point after flushing
   because call to axis from setup can destroy xsav and ysav
   */
/* version 7.2.1 2-jan-89
   make sure graph(t) and graphmode dont try to do anything when graph list is
   empty. Replace onerr with static badgraph which is one when graph is empty.
   */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "hoc.h"
#include "gui-redirect.h"

#define CLIP 1e9
#define XS   500.
#define YS   400.
#define XO   100.
#define YO   100.
#define Ret(a) \
    hoc_ret(); \
    hoc_pushx(a);

/* This makes it easier to save and restore the 8 plot scale parameters*/
#define NPARAM 10
static double param[NPARAM] = {XO, YO, XS, YS, -1e9, -1e9, 1e9, 1e9, 0.0, 0.0};
#define xorg   param[0]
#define yorg   param[1]
#define xscale param[2]
#define yscale param[3]
#define xlow   param[4]
#define ylow   param[5]
#define xhigh  param[6]
#define yhigh  param[7]
#define xsav   param[8]
#define ysav   param[9]

static double XSIZE = XS, YSIZE = YS, XORG = XO, YORG = YO;
static double xstart = 0., xstop = 1., ystart = 0., ystop = 1.;
static double xinc = 1., yinc = 1.;
static double lastmode = 1;
static double clip = CLIP;
static int grphing = 0; /* flag true if multiple graphs */

static int SaveForRegraph = 0;
static int regraph_index;
static int max_regraph_index = 1000;
static int regraph_narg[1000];
static int regraph_mode[1000];
static int regraph_color[1000];
static double regraph_x[1000];
static double* regraph_y[1000];

static int PLOT(int, int, double, double);
static void free_graph(void);
static void plotstream(int, int, double);
static void plotflush(int);
static void do_setup(void);

void hoc_regraph(void) {
    TRY_GUI_REDIRECT_DOUBLE("regraph", NULL);

    if (regraph_index < max_regraph_index) {
        regraph_y[regraph_index] = hoc_pgetarg(1);
        regraph_index++;
    }
    Ret(1.)
}
static void open_regraph(void) {
    SaveForRegraph = 1;
    regraph_index = 0;
}
static void close_regraph(void) {
    SaveForRegraph = 0;
}
static void do_regraph(void) {
    int i;
    for (i = 0; i < regraph_index; i++) {
        if (regraph_color[i] != color) {
            set_color(regraph_color[i]);
        }
        PLOT(regraph_narg[i], regraph_mode[i], regraph_x[i], *regraph_y[i]);
    }
}
static void save_regraph_item(int narg, int mode, double x) {
    regraph_narg[regraph_index] = narg;
    regraph_mode[regraph_index] = mode;
    regraph_x[regraph_index] = x;
    regraph_color[regraph_index] = color;
    if (narg == 1 && regraph_index < max_regraph_index) {
        regraph_y[regraph_index] = &ystart;
        regraph_index++;
    }
}

void hoc_Plot(void) {
    TRY_GUI_REDIRECT_DOUBLE("plot", NULL);

    double ok;
    int narg, mode;

    if (ifarg(3)) {
        narg = 3;
        mode = (int) *getarg(1);
        xsav = *getarg(2);
        ysav = *getarg(3);
    } else if (ifarg(2)) {
        narg = 2;
        mode = 0;
        xsav = *getarg(1);
        ysav = *getarg(2);
    } else if (ifarg(1)) {
        narg = 1;
        mode = (int) *getarg(1);
        if (mode == -10) {
            open_regraph();
            Ret(1.);
            return;
        } else if (mode == -11) {
            close_regraph();
            Ret(1.);
            return;
        } else if (mode == -12) {
            do_regraph();
            Ret(1.);
            return;
        }
    } else {
        Printf("plot(mode)\nplot(x, y)\nplot(mode, x, y\n");
        Printf("axis()\naxis(clip)\naxis(xorg, xsize, yorg, ysize)\n");
        Printf("axis(xstart, xstop, ntic, ystart, ystop, ntic)\n");
        Ret(1.);
        return;
    }
    ok = PLOT(narg, mode, xsav, ysav);
    Ret(ok);
}

static int PLOT(int narg, int mode, double x, double y) {
    int ok;

    ok = 1;
    if (SaveForRegraph) {
        save_regraph_item(narg, mode, x);
    }
    if (narg == 1) {
        lastmode = mode;
        return ok;
    } else if (narg == 2) {
        if ((mode = lastmode) == 1)
            lastmode = 2;
    } else {
        lastmode = mode;
        if ((mode = lastmode) == 1)
            lastmode = 2;
    }
    x = xorg + xscale * x;
    y = yorg + yscale * y;
    if (x <= xhigh && x >= xlow && y <= yhigh && y >= ylow) {
        plt(mode, x, y);
    } else if (mode > 0) {
        lastmode = 1;
        ok = 0;
    }
    return ok;
}
void hoc_plotx(void) {
    double val;
    val = xorg + *getarg(1) * xscale;
    Ret(val);
}
void hoc_ploty(void) {
    double val;
    val = yorg + *getarg(1) * yscale;
    Ret(val);
}

#define WIDTH  10.
#define HEIGHT 10.
void hoc_axis(void) {
    TRY_GUI_REDIRECT_DOUBLE("axis", NULL);

#if DOS
    extern int newstyle;
    extern unsigned text_style, text_orient, text_size;
#endif
    int width, height;
    double x, y;
    double i, j, offset;
    char s[200];
    double x0, y0;
    if (ifarg(6)) {
        xstart = *getarg(1);
        xstop = *getarg(2);
        xinc = *getarg(3);
        ystart = *getarg(4);
        ystop = *getarg(5);
        yinc = *getarg(6);
        xinc = floor(xinc);
        yinc = floor(yinc);
    } else if (ifarg(4)) {
        XORG = *getarg(1);
        XSIZE = *getarg(2);
        YORG = *getarg(3);
        YSIZE = *getarg(4);
    } else if (ifarg(1)) {
        clip = *getarg(1);
    }
    xscale = XSIZE / (xstop - xstart);
    yscale = YSIZE / (ystop - ystart);
    xorg = XORG - xstart * xscale;
    yorg = YORG - ystart * yscale;
    xlow = xstop - clip * (xstop - xstart);
    xlow = xorg + xscale * xlow;
    xhigh = xstart + clip * (xstop - xstart);
    xhigh = xorg + xscale * xhigh;
    ylow = ystop - clip * (ystop - ystart);
    ylow = yorg + yscale * ylow;
    yhigh = ystart + clip * (ystop - ystart);
    yhigh = yorg + yscale * yhigh;
    if (xorg < XORG || xorg > XORG + XSIZE)
        x0 = XORG;
    else
        x0 = xorg;
    if (yorg < YORG || yorg > YORG + YSIZE)
        y0 = YORG;
    else
        y0 = yorg;

    if (!ifarg(1)) {
        plt(1, XORG, y0);
        for (x = xstart; x <= xstop + 1e-10; x = x + (xstop - xstart) / xinc) {
            i = xorg + xscale * x;
            plt(2, i, y0);
            plt(2, i, y0 + 10.);
            plt(1, i, y0);
        }
        plt(1, x0, YORG);
        for (y = ystart; y <= ystop + 1e-10; y = y + (ystop - ystart) / yinc) {
            j = yorg + yscale * y;
            plt(2, x0, j);
            plt(2, x0 + 10., j);
            plt(1, x0, j);
        }

#if DOS
        if (newstyle) {
            settextstyle(text_style, text_orient, text_size);
            newstyle = 0;
        }
        width = textwidth("O") * 1.5;
        height = textheight("O");
#else
        width = WIDTH;
        height = HEIGHT;
#endif


        for (x = xstart; x <= xstop + 1e-10; x = x + (xstop - xstart) / xinc) {
            i = xorg + xscale * x;
            if (fabs(x) < 1e-10) {
                x = 0.;
            }
            Sprintf(s, "%g", x);
            offset = width * (int) strlen(s) / 2;
            if (i == x0 && y0 != YORG)
                offset = -width / 2;
#if DOS
            plt(1, i - offset, y0 - height);
            plt(-2, 0., 0.);
#else
            plt(1, i - offset, y0 - 1.5 * height);
            plt(-2, 0., 0.);
#endif
            plprint(s);
        }

        for (y = ystart; y <= ystop + 1e-10; y = y + (ystop - ystart) / yinc) {
            if (fabs(y) < 1e-10) {
                y = 0.;
            }
            Sprintf(s, "%g", y);
            offset = width * (int) strlen(s) + width;
            j = yorg + yscale * y;
            if (j == y0 && x0 != XORG)
#if DOS
                plt(1, x0 - offset, j + height);
#else
                plt(1, x0 - offset, j + 2.);
#endif
            else
#if DOS
                plt(1, x0 - offset, j + height / 2);
#else
                plt(1, x0 - offset, j - 6.);
#endif

            plt(-2, 0., 0.);
            plprint(s);
        }
        plt(-1, 0., 0.);
    }
    Ret(1.);
}

/* prior to version 7.1.1
graph connects with the plot functions in such a way that multiple
line graphs can be generated by using the plot function in the same way
that one creates a single plot.

graph() starts a new list of expressions and setup statements
graph("expr", "setup") adds this spec to the list
graph(1) executes the setup statements
graph(-1) flushes remaining part of graph
  version 7.1.1
  graph(t) accumulates a point with this abscissa for all expr in list.
  graphmode(1) executes setup statements
  graphmode(-1) flushes remaining part of graph, next graphs with start
      new lines
  graphmode(2...) flushes, but next graphs don't start new lines
*/

/* local info for graph */
static int initialized;         /* true if the setup statements have been executed*/
static Symlist* graph_sym_list; /*list of expressions and setup statements*/
#define MAXCNT 50
static int pcnt;         /* points per plot already stored */
static int badgraph = 1; /* graph data structure is no good or empty */
static double* lx;       /* points to array which holds abscissa values */

typedef struct Grph {    /* holds info for graphing */
    struct Grph* g_next; /*next one in the queue*/
    Symbol* g_sexp;      /* symbol for the expression */
    Symbol* g_setup;     /* symbol for the setup statement */
    int g_color;
    double g_param[NPARAM]; /* holds plot scale factors, etc. */
    double g_val[MAXCNT];   /* y value buffer to plot */
} Grph;

static Grph *glist_head, *glist_tail; /* access for the queue */

void hoc_Graph(void) {
    TRY_GUI_REDIRECT_DOUBLE("graph", NULL);

    Grph* g;

    if (ifarg(2)) {
        if (badgraph) {
            free_graph();
        }
        badgraph = 1; /* if this is not reset at end then we had an execerror*/
        initialized = pcnt = 0;
        if (glist_head == (Grph*) 0) {
            lx = (double*) emalloc(sizeof(double) * MAXCNT);
        }
        g = (Grph*) emalloc(sizeof(Grph));
        g->g_next = (Grph*) 0;
        g->g_sexp = g->g_setup = (Symbol*) 0;
        if (glist_tail != (Grph*) 0) {
            glist_tail->g_next = g;
        } else {
            glist_head = g;
        }
        glist_tail = g;

        g->g_sexp = hoc_parse_expr(gargstr(1), &graph_sym_list);
        g->g_setup = hoc_parse_stmt(gargstr(2), &graph_sym_list);
        badgraph = 0; /* successful */
        grphing = 1;

    } else if (ifarg(1) && !badgraph) {
        plotstream(2, 2, *getarg(1));
    } else {
        free_graph();
        badgraph = 1;
    }
    Ret(0.);
}

void hoc_Graphmode(void) {
    TRY_GUI_REDIRECT_DOUBLE("graphmode", NULL);
    int mode;
    if (!badgraph) {
        mode = *getarg(1);
        if (mode == 1) {
            do_setup();
        }
        if (mode == -1) {
            plotflush(1);
        }
        if (mode > 1 && pcnt > 0) {
            plotflush(2);
        }
    }
    Ret(0.);
}

static void free_graph(void) {
    Grph *g, *gnext;
    /* free all graph space and reinitialize */
    hoc_free_list(&graph_sym_list);
    for (g = glist_head; g != (Grph*) 0; g = gnext) {
        gnext = g->g_next;
        free((char*) g);
    }
    if (lx) {
        free((char*) lx);
        lx = (double*) 0;
    }
    glist_head = glist_tail = (Grph*) 0;
    grphing = initialized = pcnt = 0;
}

static void plotstream(int narg, int mode, double x) {
    Grph* g;

    if (narg == 1 || narg == 3) {
        plotflush(1);
        IGNORE(PLOT(narg, mode, xsav, ysav));
    }
    if (pcnt >= MAXCNT) {
        plotflush(2);
    }
    if (narg == 2 || narg == 3) {
        lx[pcnt] = x;
        for (g = glist_head; g != (Grph*) 0; g = g->g_next) {
            g->g_val[pcnt] = hoc_run_expr(g->g_sexp);
        }
        pcnt++;
    }
}

static void plotflush(int contin) {
    /* contin = 1 then flush after this will start with new points
       contin = 2 then it will start with last points of this flush
       */
    int savcolor;
    int i, savmode;
    double parsav[NPARAM];
    Grph* g;

    savmode = lastmode;
    if (!initialized) {
        do_setup();
    }
    for (i = 0; i < NPARAM; i++) {
        parsav[i] = param[i];
    }
    savcolor = color;
    for (g = glist_head; g != (Grph*) 0; g = g->g_next) {
        for (i = 0; i < NPARAM; i++) {
            param[i] = g->g_param[i];
        }
        if (color != g->g_color) {
            IGNORE(set_color(g->g_color));
        }
        IGNORE(PLOT(1, 1, 0., 0.));
        for (i = 0; i < pcnt; i++) {
            IGNORE(PLOT(2, 0, lx[i], g->g_val[i]));
        }
        if (contin == 2) {
            g->g_val[0] = g->g_val[pcnt - 1];
        }
    }
    for (i = 0; i < NPARAM; i++) {
        param[i] = parsav[i];
    }
    if (savcolor != color) {
        IGNORE(set_color(savcolor));
    }
    if (contin == 2 && pcnt > 0) {
        lx[0] = lx[pcnt - 1];
        pcnt = 1;
        IGNORE(PLOT(3, 1, xsav, ysav)); /* last point of explicit plot */
    }
    lastmode = savmode;
    if (contin == 1) {
        pcnt = 0;
    }
}

static void do_setup(void) {
    Grph* g;
    double parsav[NPARAM];
    int i;
    int savcolor;

    for (i = 0; i < NPARAM; i++) {
        parsav[i] = param[i];
    }
    savcolor = color;
    for (g = glist_head; g != (Grph*) 0; g = g->g_next) {
        hoc_run_stmt(g->g_setup);
        for (i = 0; i < NPARAM; i++) {
            g->g_param[i] = param[i];
        }
        g->g_color = color;
    }
    for (i = 0; i < NPARAM; i++) {
        param[i] = parsav[i];
    }
    if (savcolor != color) {
        IGNORE(set_color(savcolor));
    }
    initialized = 1;
}
