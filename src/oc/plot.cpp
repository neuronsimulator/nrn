#include <../../nrnconf.h>
#include "hoc.h"


/*LINTLIBRARY*/
#undef IGNORE
#if LINT
#define IGNORE(arg)	{if(arg);}
#else
#define IGNORE(arg)	arg
#endif

#if GRX
#include <libbcc.h>
#include "hoc.h"
#define __TURBOC__
#endif

#if defined(useNeXTstep) || defined(__TURBOC__) || defined(__linux__)
#ifndef NRNOC_X11
#define NRNOC_X11 0
#endif
#endif

#define	FIG 1 /* 12/8/88 add fig style output , replaces hpflag*/
#define TEK 1
#define HP 1
/* but not VT125 */
#define CODRAW 1

#if CODRAW
static void Codraw_plt(int, double, double);
static void Codraw_preamble(void);
#endif

#if HP
static void hplot(int, double, double);
#endif

#if FIG
static void Fig_preamble(void);
static void Fig_plt(int, double, double);
void Fig_file(const char*, int);
#endif

#if TEK
static void tplot(int, double, double);
#endif

#include <stdio.h>
#include <string.h>

#if defined(__MINGW32__)
extern char** _environ;
#else /*!__MINGW32__*/

#if HAVE_UNISTD_H
#include <unistd.h>
#if !defined(__APPLE__)
extern char** environ;
#else /* __APPLE */
#include <crt_externs.h>
#endif /* __APPLE__ */
#endif /* HAVE_UNISTD_H */

#endif /*!__MINGW32__*/

#if DOS
#include <graphics.h>
#include <dos.h>
#endif
#if defined(GRX)
#define DOS 1
#endif

static int console = 1;	/* 1 plotting to console graphics */
static int hardplot; /* 1 hp style  2 fig style  3 coplot*/
static int graphdev;
static int hpflag;	/* hp plotter switch */
int hoc_plttext;	/* text can be printed to hpdev */
#define text hoc_plttext
static FILE *gdev;	/* system call stdout not directed to here */
static FILE *hpdev;	/* hp, or fig style file */
static FILE *cdev;	/* console device */

#define SSUN 1
#define VT 2
#define SEL 3
#define TEK4014 4
#define ADM 5
#define NX 6

#define US 037
#define GS 035
#define CAN 030
#define EM 031
#define FS 034
#define ESC 033
#define ETX 03

static char	vt100[] = "TERM=vt125";
static char	adm3a[] = "TERM=adm3a";
static char	ssun[] = "TERM=sun";
static char	tek4014[] = "TERM=4014";
static char	ncsa[] = "NEURON=ncsa";
int hoc_color = 15;
static double xlast, ylast;

#if NRNOC_X11
extern void x11_put_text(const char*), x11_close_window(void), x11_setcolor(int);
extern void x11_coord(double, double), x11_vector(), x11_point(), x11_move(), x11flush();
extern void x11_clear(), x11_cleararea(), x11_open_window(), x11_fast(int);
static void hoc_x11plot(int, double, double);
#endif

#if NeXTstep
extern void NeXT_put_text(), NeXT_close_window(), NeXT_setcolor();
extern void NeXT_coord(), NeXT_vector(), NeXT_point(), NeXT_move(), NeXTflush();
extern void NeXT_clear(), NeXT_cleararea(), NeXT_open_window(), NeXT_fast();
#endif

static void hard_text_preamble();

#if defined(__TURBOC__)
int egagrph = 0;	/* global because need to erase on quit if in graphics
				mode */
static int graphmode = 0;
static double xres=640., yres=350.;
static tplt();

void *_graphgetmem(unsigned size) {
	char *p;
	p= hoc_Emalloc(size), hoc_malchk() ;
	return p;
}

static void Initplot(void) {
#if !defined(__GO32__)
	registerfarbgidriver(EGAVGA_driver_far);
	registerfarbgidriver(CGA_driver_far);
	registerfarbgidriver(Herc_driver_far);
	registerfarbgifont(triplex_font_far);
#endif

	graphdev = DETECT;
	initgraph(&graphdev, &graphmode,"c:\\bc\\bgi");
	{int err;
		graphdev=1;
		err = graphresult();
		if (err != grOk) {
hoc_execerror("Error in initializing graphics adaptor\n", (char *)0);
		}
		xres = (double)(getmaxx()+1);
		yres = (double)(getmaxy()+1);
		grx_txt_clear();
		return;
	}
	if (graphdev > 0) {
		xres = (double)(getmaxx()+1);
		yres = (double)(getmaxy()+1);
		restorecrtmode();
	} else {
hoc_execerror("Error in initializing graphics adaptor\n", (char *)0);
	}
}
#endif

void plprint(const char* s)
{
#if DOS
	extern int newstyle;
	extern unsigned text_style, text_size, text_orient;
#endif
	char buf[128];

	if (text && s[strlen(s) - 1] == '\n') {
		IGNORE(strcpy(buf, s));
		s = buf;
		buf[strlen(s)-1] = '\0';
	}

	if (console && text) {
#if	DOS
		if (egagrph == 2) {
			if (newstyle) {
				settextstyle(text_style,text_orient,text_size);
				newstyle = 0;
			}
			outtext(s);
		}else{
			IGNORE(fprintf(cdev, "%s", s));
		}
#else
#if SUNCORE
		hoc_pl_sunplot(s);
#else
#if NRNOC_X11
		x11_put_text(s);
#else
#if NeXTstep
		if (graphdev == NX) 
			NeXT_put_text(s);
#else

		IGNORE(fprintf(cdev, "%s", s));
		IGNORE(fflush(cdev));
#endif
#endif
#endif
#endif

	} else if (!text) {
#if GRX
		if (egagrph) {
			hoc_outtext(s);
		}else
#endif
		{
#if 0
			IGNORE(fprintf(stdout, "%s", s));
#else
			nrnpy_pr("%s", s);
#endif
		}
	}
	if (hardplot && hpdev && text && strlen(s)) {
		hard_text_preamble();
		IGNORE(fprintf(hpdev, "%s", s));
		IGNORE(fflush(hpdev));
	}
	if (text && s == buf) {
		plt(1, xlast, ylast-20);
		plt(-2, 0.,0.);
	}
}

#if GRX
static int trcur;
static GrTextRegion* gtr;
#define GRXCOL 80
#define GRXROW 5
void grx_move(int r, int c){
	r = r%GRXROW;
	c = c%GRXCOL;
	trcur = c + r*GRXCOL;
}
void grx_rel_move(int new){
	trcur = ((int)(trcur/GRXCOL))*GRXCOL + new;
}
void grx_output_some_chars(const char* string, int count) {
	if (string[count] == '\0') {
		hoc_outtext(string);
	}else{
		hoc_outtext("non-terminated string\n");
	}
}
void grx_delete_chars(int count) {
	int i;
	int j = trcur;
	for (i=0; i < count; ++i) {
		gtr->txr_buffer[j++] = ' ';
	}
	GrDumpTextRegion(gtr);
}
grx_insert_some_chars(string, count) char* string; int count; {
	int i, j;
	i = trcur + count;
	j = trcur + count;
	while(i >= trcur) {
		gtr->txr_buffer[j--] = gtr->txr_buffer[i--];
	}
	i = trcur;
	grx_output_some_chars(string, count);
	trcur = i;
}
grx_backspace(count) int count; {
	while (count--) {
		trcur--;
	}
}
grx_clear_to_eol() {
	int j = trcur;
	do {
		gtr->txr_buffer[j] = ' ';
	}while ((++j)%GRXCOL);
	GrDumpTextRegion(gtr);
}

grx_force_text() {
	int i;
	for (i=0; i < GRXCOL*GRXROW; ++i) {
		gtr->txr_backup[i] = '\0';
	}
	GrDumpTextRegion(gtr);
}

grx_txt_clear() {
	int i;
	if (!gtr) {
		gtr = (GrTextRegion*)emalloc(sizeof(GrTextRegion));
		gtr->txr_font = (GrFont*)0;
		gtr->txr_buffer = ecalloc(GRXCOL*GRXROW, sizeof(char));
		gtr->txr_backup = ecalloc(GRXCOL*GRXROW, sizeof(char));
		gtr->txr_xpos = 0;
		gtr->txr_ypos = 0;
		gtr->txr_width = GRXCOL;
		gtr->txr_height = GRXROW;
		gtr->txr_lineoffset = GRXCOL;
		gtr->txr_fgcolor.v = 7;
		gtr->txr_bgcolor.v = 0;
		gtr->txr_chrtype = GR_BYTE_TEXT;
	}
	for (i=0; i < GRXCOL*GRXROW; ++i) {
		gtr->txr_buffer[i] = ' ';
	}
	trcur = 0;
	GrDumpTextRegion(gtr);
}

hoc_outtext(s) char* s; {
	int i;
	char* cp;
	int ch;
	if (!egagrph) {
		return;
	}
	if (! gtr) {
		grx_txt_clear();
	}
	
	for (cp = s; *cp; ++cp) {
		ch = *cp;
		if (*cp == '\n' || trcur >= GRXROW*GRXCOL) {
			char* c1, *c2;
			int j;
			for (i=1; i < GRXROW; ++i) {
				c1 = gtr->txr_buffer + (i-1)*GRXCOL;
				c2 = gtr->txr_buffer + (i)*GRXCOL;
				for (j =0; j < GRXCOL; ++j) {
					c1[j] = c2[j];
				}
			}
			for (i=0; i < GRXCOL; ++i) {
				c2[i] = ' ';
			}
			trcur = (GRXROW-1)*GRXCOL;
			continue;
		}
		if (ch == '\t') {
			ch = ' ';
		}
		gtr->txr_buffer[trcur++] = ch;
	}
	GrDumpTextRegion(gtr);
}
#endif

void initplot(void)
{
#if !defined(__MINGW32__) /* to end of function */
	int i;
#if defined (__APPLE__)
	char** environ=(*_NSGetEnviron());
#endif
#if defined(__TURBOC__)
	graphdev = 0;
#else
#if NeXTstep
	graphdev = NX;
#else
	graphdev = SSUN;
	for (i = 0; environ[i] != NULL; i++)
	{
		if (strcmp(environ[i], vt100) == 0)
			graphdev = VT;
		if (strcmp(environ[i], ssun) == 0)
			graphdev = SSUN;
		if (strcmp(environ[i], adm3a) == 0)
			graphdev = ADM;
		if (strcmp(environ[i], tek4014) == 0)
			graphdev = TEK4014;
		if (strcmp(environ[i], ncsa) == 0)
			graphdev = TEK4014;
	}
#endif /*!NeXTstep*/
#endif /*!__TURBOC__*/
	hpdev = (FILE *)0;
	cdev = gdev = stdout;
#if SUNCORE
	if (graphdev == SSUN) {
		hoc_open_sunplot(environ);
#else
#if NeXTstep
	/*EMPTY*/
	if (graphdev == SSUN) {
#else
#if NRNOC_X11
    /*EMPTY*/
    if (graphdev == SSUN) {
#else
    if (graphdev == SSUN) {
/*
	if (graphdev == SSUN &&(cdev = fopen("/dev/ttyp4", "w")) == (FILE *)0) {
		IGNORE(fprintf(stderr, "Can't open /dev/ttyp4 for TEK\n"));
		cdev = stdout;
*/
#endif /*NRNOC_X11*/
#endif /*NeXTstep*/
#endif /*SUNCORE*/
    } else {
        if (graphdev == TEK4014) {
            cdev = gdev = stdout;
        }
    }
#endif /*!__MINGW32__*/
}

void hoc_close_plot(void) {
#if    DOS
    if (egagrph) plt(-3,0.,0.);
#else
#if    SUNCORE
    hoc_close_sunplot();
#else
#if    NRNOC_X11
    x11_close_window();
#else
#if    NeXTstep
    NeXT_close_window();
#endif
#endif
#endif
#endif
}

void plt(int mode, double x, double y) {
    if (x < 0.) x = 0.;
    if (x > 1000.) x = 1000.;
    if (y < 0.) y = 0.;
    if (y > 780.) y = 780.;
    if (mode >= 0) {
        xlast = x;
        ylast = y;
    }
    if (console) {
#if defined(__TURBOC__)
        if(graphdev > 0) {
            tplt(mode, x, y);
        } else if (graphdev == 0) {
            Initplot();
            tplt(mode, x, y);
        }
#else
        switch (graphdev) {
            case SSUN:
#if SUNCORE
                hoc_sunplot(&text, mode, x, y);
                break;
#else
#if NRNOC_X11
                hoc_x11plot(mode,x,y);
                break;
#else
#if NeXTstep
                break;
            case NX:
                hoc_NeXTplot(mode,x,y);
                break;
#endif
#endif
#endif
#if TEK
            case ADM:
            case SEL:
            case TEK4014:
                tplot(mode, x, y);
                break;
#endif
#if VT125
                case VT:
                    vtplot(mode, x, y);
                    break;
#endif
        }
#endif
    }
#if HP
    if (hardplot == 1) {
        hplot(mode, x, y);
    }
#endif
#if FIG
    if (hardplot == 2) {
        Fig_plt(mode, x, y);
    }
#endif
#if CODRAW
    if (hardplot == 3) {
        Codraw_plt(mode, x, y);
    }
#endif
    if (hardplot && hpdev) {
        IGNORE(fflush(hpdev));
    }
    if (console && cdev) {
        IGNORE(fflush(cdev));
    }
}

#if TEK
#define XHOME 0
#define YHOME 770

static void tplot(int mode, double x, double y) {
    unsigned ix, iy;
    int hx, hy, ly, lx;

    if (graphdev == SEL) {
        IGNORE(putc(ESC, cdev));
        IGNORE(putc('1', cdev));
    }
    if (mode < 0) {
        switch (mode) {
            default:
            case -1:            /* home cursor */
                switch (graphdev) {
                    case ADM:    /* to adm3a alpha mode */
                        IGNORE(putc(US, cdev));
                        IGNORE(putc(CAN, cdev));
                        break;
                    case TEK4014:
                    default:
                        IGNORE(putc(GS, cdev));
                        hy = (((YHOME & 01777) >> 5) + 32);
                        ly = ((YHOME & 037) + 96);
                        hx = (((XHOME & 01777) >> 5) + 32);
                        lx = ((XHOME & 037) + 64);
                        IGNORE(fprintf(cdev, "%c%c%c%c", hy, ly, hx, lx));
                        IGNORE(putc(US, cdev));
                        break;
                }
                text = 0;
                return;

            case -2:        /* to 4010 alpha mode */
                IGNORE(putc(GS, cdev));
                IGNORE(putc(US, cdev));
                text = 1;
                return;

            case -3:        /* erase, and go to alpha on ADM */
                switch (graphdev) {
                    case ADM:
                        IGNORE(putc(GS, cdev));
                        IGNORE(putc(EM, cdev));
                        IGNORE(putc(US, cdev));
                        IGNORE(putc(CAN, cdev));
                        break;
                    case TEK4014:
                    default:
                        IGNORE(putc(ESC, cdev));
                        IGNORE(putc(014, cdev));
                        break;
                }
                text = 0;
                return;
        }
    }
    switch (mode) {
        case 0:        /* enter point mode */
            /*IGNORE(putc(FS, cdev));
            break;*/ /* no point mode on pure 4014 so plot vector of 0 length*/
        case 1:        /* enter vector mode */
            IGNORE(putc(GS, cdev));
            break;
    }
    iy = y;
    ix = x;
    hy = (((iy & 01777) >> 5) + 32);
    ly = ((iy & 037) + 96);
    hx = (((ix & 01777) >> 5) + 32);
    lx = ((ix & 037) + 64);
    IGNORE(fprintf(cdev, "%c%c%c%c", hy, ly, hx, lx));
    if (mode == 0) {
        IGNORE(fprintf(cdev, "%c%c%c%c", hy, ly, hx, lx));
    }
    return;
}

#endif /*TEK*/

#if FIG || HP || CODRAW

static char hardplot_filename[100];

void hardplot_file(const char *s) {
    if (hpdev) {
        IGNORE(fclose(hpdev));
    }
    hpdev = (FILE *) 0;
    hpflag = 0;
    hardplot = 0;
    gdev = stdout;
    if (s) {
        hpdev = fopen(s, "w");
        if (hpdev) {
            strncpy(hardplot_filename, s, 99);
            hpflag = 1;
            gdev = hpdev;
        } else {
            IGNORE(fprintf(stderr, "Can't open %s for hardplot output\n", s));
        }
    } else {
        hardplot_filename[0] = '\0';
    }
}

void Fig_file(const char *s, int dev) {
    plt(-1, 0., 0.);
    hardplot_file(s);
    if (!hpdev) return;
    hardplot = dev;
#if FIG
    if (hardplot == 2) {
        Fig_preamble();
    }
#endif
#if CODRAW
    if (hardplot == 3) {
        Codraw_preamble();
    }
#endif
}

#endif

static char fig_text_preamble[100];

static void hard_text_preamble(void) {
    if (hardplot == 2) {
        IGNORE(fprintf(hpdev, "%s", fig_text_preamble));
        fig_text_preamble[0] = '\0';
    }
}

#if FIG

static void Fig_preamble(void) {
    static char fig_preamble[] = "#FIG 1.4\n80 2\n";

    if (!hpdev) return;
    IGNORE(fprintf(hpdev, "%s", fig_preamble));
}

#define HIRES 1

void Fig_plt(int mode, double x, double y) {
#define SCX(x) ((int)(x*.8))
#define SCY(y) (600-(int)(y*.8))
#if HIRES
#define SCXD(x) ((x*.8))
#define SCYD(y) (7.5*80.-(y*.8))
#endif
#undef TEXT
#define TEXT 1
#define LINE1 2
#define LINE2 3
    static short state = 0;
    static double oldx, oldy;
    static char
            text_preamble[] = "4 0 0 16 0 0 0 0.000 1 16 40 ",
            text_postamble[] = "\1\n",
#if HIRES
            line_preamble[] = "7 1 0 1 0 0 0 0 0.000 0 0\n",
#else
    line_preamble[]="2 1 0 1 0 0 0 0 0.000 0 0\n",
#endif
            line_postamble[] = " 9999 9999\n";


    if (!hpdev) return;

    if (state == TEXT) {
        if (!fig_text_preamble[0]) {
            IGNORE(fprintf(hpdev, "%s", text_postamble));
        }
        state = 0;
        text = 0;
    }

    if (mode < 0) {
        if (state == LINE2) {
            IGNORE(fprintf(hpdev, "%s", line_postamble));
        }
        text = 0;
        state = 0;
        if (mode == -2) {
            IGNORE(sprintf(fig_text_preamble, "%s %d %d ",
                           text_preamble, SCX(oldx), SCY(oldy)));
            state = TEXT;
            text = 1;
            return;
        }
        if (mode == -3) {
#if 0
            IGNORE(fseek(hpdev, 0L, 0));
            Fig_preamble();
#else
            Fig_file(hardplot_filename, 2);
#endif
        }
    } else {
        switch (mode) {
            case 0:
                break;
            case 1:
                if (state == LINE2) {
                    IGNORE(fprintf(hpdev, "%s", line_postamble));
                }
                state = LINE1;
                break;
            default:
                if (state == LINE1) {
#if HIRES
                    IGNORE(fprintf(hpdev, "%s %.1f %.1f\n", line_preamble,
                                   SCXD(oldx), SCYD(oldy)));
#else
                    IGNORE(fprintf(hpdev, "%s %d %d\n", line_preamble,
                        SCX(oldx), SCY(oldy)));
#endif
                    state = LINE2;
                }
#if HIRES
                IGNORE(fprintf(hpdev, " %.1f %.1f\n", SCXD(x), SCYD(y)));
#else
                IGNORE(fprintf(hpdev, " %d %d\n", SCX(x), SCY(y)));
#endif
                break;
        }
        oldx = x;
        oldy = y;
    }
}

#endif /*FIG*/

#if HP

void hplot(int mode, double x, double y) {
    static short hpflag = 0;
    static short txt = 0;

    if (!hpdev) return;
    if (hpflag == 0) {
        hpflag = 1;
        IGNORE(fprintf(hpdev, "%c.Y%c.I81;;17:%c.N;19:SC 0,1023,0,780;SP 1;",
                       ESC, ESC, ESC));
    }

    if (txt == 1) {
        IGNORE(fprintf(hpdev, "%c;", ETX));
        txt = 0;
        text = 0;
    }
    if (mode < 0)
        switch (mode) {
            case -2:
                IGNORE(fprintf(hpdev, "LB"));
                txt = 1;
                text = 1;
                return;

            case -3:
                txt = 0;
                text = 0;
                hpflag = 0;
                IGNORE(fseek(hpdev, 0L, 0));
                return;
            default:
                IGNORE(fprintf(hpdev, "PU;SP;%c.Z", ESC));
                txt = 0;
                text = 0;
                hpflag = 0;
                return;
        }
    switch (mode) {
        case 0:
            IGNORE(fprintf(hpdev, "PU %8.2f,%8.2f;PD;", x, y));
            return;
        case 1:
            IGNORE(fprintf(hpdev, "PU %8.2f,%8.2f;", x, y));
            return;
        default:
            IGNORE(fprintf(hpdev, "PD %8.2f,%8.2f;", x, y));
            return;
    }
}

#endif /*HP*/

/* not modified for new method */
#if VT125
void vtplot(int mode, double x, double y)
{
    static short vtgrph = 0;
    int vtx, vty;

    if (mode < 0)
    {
        IGNORE(fprintf(gdev, "%c\\%c", ESC, ESC));
        switch (mode)
        {
        default:
        case -1:		/* vt125 alpha mode */
            break;

        case -2:		/* graphics text mode */
            IGNORE(fprintf(gdev, "P1pt\'"));
            vtgrph = 2;
            return;

        case -3:		/* erase graphics */
            IGNORE(fprintf(gdev, "P1pS(E)"));
            break;

        case -4:		/* erase text */
            IGNORE(fprintf(gdev, "[2J"));
            break;

        case -5:		/* switch to HP plotter */
            IGNORE(fprintf(gdev, "%c\\%c[5i", ESC, ESC)); /* esc and open port */
            vtgrph = 0;
            hplot(-5, 0., 0.);
            return;
        }
        IGNORE(fprintf(gdev, "%c\\", ESC));
        vtgrph = 0;
        return;
    }

    if (vtgrph == 2)
    {
        IGNORE(fprintf(gdev, "%c\\", ESC));
        vtgrph = 0;
    }
    if (vtgrph == 0)
    {
        IGNORE(fprintf(gdev, "%cP1p", ESC));
        vtgrph = 1;
    }
    vtx = (int) ((767./1023.)*x);
    vty = (int) (479. - (479/779.)*y);
    if (mode >= 2)
    {
        IGNORE(fprintf(gdev, "v[%d,%d]", vtx, vty));
        return;
    }
    IGNORE(fprintf(gdev, "p[%d,%d]", vtx, vty));
    if (mode == 0)
        IGNORE(fprintf(gdev, "v[]"));
    return;
}
#endif /*VT*/

int set_color(int c) {
    if (c >= 0 || c < 128) {
        hoc_color = c;
    }
#if defined(__TURBOC__)
    if (egagrph) {
        setcolor(hoc_color);
    }
#else
#if SUNCORE
    set_line_index(c);
    set_text_index(c);
#else
#if NRNOC_X11
    x11_setcolor(c);
#else
#if NeXTstep
    NeXT_setcolor(c);
#endif
#endif
#endif
#endif
    return (int) hoc_color;
}

#if defined(__TURBOC__)
#define UN		unsigned int

static void tplt(int mode, double x, double y)
{
#if DOS
    extern int newstyle;
#endif
    int ix, iy;

    if (egagrph == 0)
    {
        setgraphmode(graphmode);
        setcolor(hoc_color);
        egagrph = 1;
#if DOS
        newstyle = 1;
#endif
    }

    if (mode < 0)
    {
        text = 0;
        switch (mode)
        {
        default:
        case -1:
            if (egagrph) {
                egagrph=1;
            }
                        cursor(0,0);
            break;

        case -2:		/* graphics text mode */
            egagrph=2;
            text = 1;
            return;

        case -3:		/* erase graphics */
            restorecrtmode();
            egagrph = 0;
            break;
#if GRX
        case -4:
            grx_txt_clear();
            break;
        case -5:
            GrClearScreen(0);
            grx_force_text();
            break;
        }
#endif
        return;
    }

    ix = ((xres -1.)/1023.)*x;
    iy = (yres-1.) - ((yres-1.)/779.)*y;

    if (mode >= 2)
    {
        lineto(ix, iy);
    }
    if (mode == 1) {
        moveto(ix, iy);
    }
    if (mode == 0)
    {
        moveto(ix, iy);
        lineto(ix, iy);
    }
    return;
}

void cursor(int r, int c) {
#if !defined(__GO32__)
    int ax, dx;
    ax = 2*256;
    dx = (r&255)*256 + c&255;
    _AX = ax;
    _BX = 0;
    _DX = dx;
    geninterrupt(0x10);
#endif
}
#endif

#if CODRAW

#define CODRAW_MAXPOINT 200
static int codraw_npoint = 0;
static float *codraw_pointx, *codraw_pointy;

static void codraw_line();

void Codraw_preamble() {
    static char codraw_preamble[] = "SW(1,0,8,0,8);\nST(1);\nSG(0.1);\n\
SF(1,'HELVET-L');\nSF(2,'HELVET');\nSF(3,'CENTURY');\nSF(4,'SCRIPT');\n\
SF(5,'GREEK');\nSP(1);\nLT(1);LW(1);LC(15);LD(50);\nTF(1);TW(1);TS(0);TC(15);\
TL(1);TV(4);TA(0);TH(0.2);\n";

    if (!hpdev) return;
    IGNORE(fprintf(hpdev, "%s", codraw_preamble));
    codraw_npoint = 0;
    if (!codraw_pointy) {
        codraw_pointx = (float *) hoc_Emalloc(CODRAW_MAXPOINT * sizeof(float));
        codraw_pointy = (float *) hoc_Emalloc(CODRAW_MAXPOINT * sizeof(float));
        hoc_malchk();
    }
}

void Codraw_plt(int mode, double x, double y) {
#undef SCXD
#undef SCYD
#define SCXD(x) ((x*.008))
#define SCYD(y) ((y*.008))
#undef TEXT
#undef LINE1
#undef LINE2
#define TEXT 1
#define LINE1 2
#define LINE2 3
    static short state = 0;
    static double oldx, oldy;

    if (!hpdev) return;

    if (state == TEXT) {
        IGNORE(fprintf(hpdev, "');\n"));
        state = 0;
        text = 0;
    }

    if (mode < 0) {
        if (state == LINE2) {
            codraw_line();
        }
        text = 0;
        state = 0;
        if (mode == -2) {
            IGNORE(fprintf(hpdev, "TT(%.2f,%.2f,'",
                           SCXD(oldx), SCYD(oldy)));
            state = TEXT;
            text = 1;
            return;
        }
        if (mode == -3) {
            IGNORE(fseek(hpdev, 0L, 0));
            Codraw_preamble();
        }
    } else {
        switch (mode) {
            case 0:
                break;
            case 1:
                if (state == LINE2) {
                    codraw_line();
                }
                state = LINE1;
                break;
            default:
                if (state == LINE1) {
                    codraw_npoint = 1;
                    codraw_pointx[0] = oldx;
                    codraw_pointy[0] = oldy;
                    state = LINE2;
                }
                codraw_pointx[codraw_npoint] = x;
                codraw_pointy[codraw_npoint] = y;
                if (++codraw_npoint == CODRAW_MAXPOINT) {
                    codraw_line();
                }
                break;
        }
        oldx = x;
        oldy = y;
    }
}

static void codraw_line() {
    int i;

    if (codraw_npoint < 2) {
        codraw_npoint = 0;
        return;
    }
    IGNORE(fprintf(hpdev, "LL(%d", codraw_npoint));
    for (i = 0; i < codraw_npoint; i++) {
        if (((i + 1) % 8) == 0) {
            IGNORE(fprintf(hpdev, "\n"));
        }
        IGNORE(fprintf(hpdev, ",%.2f,%.2f", SCXD(codraw_pointx[i]),
                       SCYD(codraw_pointy[i])));
    }
    IGNORE(fprintf(hpdev, ");\n"));
    if (codraw_npoint == CODRAW_MAXPOINT) {
        codraw_npoint = 1;
        codraw_pointx[0] = codraw_pointx[CODRAW_MAXPOINT - 1];
        codraw_pointy[0] = codraw_pointy[CODRAW_MAXPOINT - 1];
    } else {
        codraw_npoint = 0;
    }
}

#endif

#if NRNOC_X11
extern char *getenv();
static void hoc_x11plot(int mode, double x, double y)
{
    extern int x11_init_done;

    if (!x11_init_done) {
        x11_open_window();
    }

    if (mode >= 0) {
        x11_coord(x, y);
    }
    if (mode > 1) {
        x11_vector();
    }else {
        switch (mode) {
        case 0:
            x11_point();
            break;
        case 1:
            x11_move();
            break;
        case -1:
            text = 0;
            x11flush();
            break;
        case -2:
            text = 1;
            break;
        case -3:
            x11_clear();
            break;
        case -4:
            x11_coord(x, y);
            x11_cleararea();
            break;
        case -5:
            x11_fast(1);
            break;
        case -6:
            x11_fast(0);
        }
    }
}
#endif /*NRNOC_X11*/

#if NeXTstep
/* extern char *getenv(); */
static
hoc_NeXTplot(mode, x, y)
    int mode;
    double x, y;
{
    extern int NeXT_init_done;

    if (!NeXT_init_done) {
        NeXT_open_window();
    }

    if (mode >= 0) {
        NeXT_coord(x, y);
    }
    if (mode > 1) {
        NeXT_vector();
    }else {
        switch (mode) {
        case 0:
            NeXT_point();
            break;
        case 1:
            NeXT_move();
            break;
        case -1:
            text = 0;
            NeXTflush();
            break;
        case -2:
            text = 1;
            break;
        case -3:
            NeXT_clear();
            break;
        case -4:
            NeXT_coord(x, y);
            NeXT_cleararea();
            break;
        case -5:
            NeXT_fast(1);
            break;
        case -6:
            NeXT_fast(0);
            break;
        case -7:
            NeXT_fast(-1);
            break;
        }
    }
}
#endif /*NeXTstep*/

