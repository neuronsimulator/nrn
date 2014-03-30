#include <../../nrnconf.h>
/* Copyright 1987,1988- Michael Hines, Neurobiology Dept., DUMC, NC
*
* REVISION HISTORY:
*
* 11-88 This file was fixed so that the memacs editor would work with
the editing keys. Made use of dos function call 7 which returns the
extended ASCII codes for the editing keys.
*
*/
/*mlh /7/20/87
==================================================
ibm.c
==================================================
*/
/*
 * The routines in this file provide support for PC compatible terminals
 * The serial I/O services are provided by routines in
 * "termio.c". It compiles into nothing if not an IBM device.
 */

#define		termdef		1
#include	<stdio.h>
#include	"estruct.h"
#include	"edef.h"

#if	_Windows || MSDOS
#if _Windows
#undef IGNORE
#include <windows.h>
#include <winio.h>
static HWND hwnd;
#undef IGNORE
#define IGNORE(arg) arg
#else
#include	<dos.h>	/* works with microsoft c */
#endif
#if __GO32__
#ifndef _Windows
#include	<gppconio.h>
#endif
#else
#ifndef _Windows
#include	<conio.h>
#endif
#endif

#ifdef _Windows
#define NROW	16			/* Screen size. 		*/
#define NCOL	50			/* Edit if you want to. 	*/
#else
#define NROW	25			/* Screen size. 		*/
#define NCOL	80			/* Edit if you want to. 	*/
#endif
#define MARGIN	8
#define	SCRSIZ	64
#define BEL	0x07			/* BEL character.		*/
#define ESC	0x1B			/* ESC character.		*/

extern	int	ttopen();		/* Forward references.		*/
static	int	ibmgetc();
static	int	ibmputc();
static	int	ibmflush();
#ifdef _Windows
static	int	ibmclose();
#else
extern	int	ttclose();
#endif
static	int	ibmmove();
static	int	ibmeeol();
static	int	ibmeeop();
static	int	ibmbeep();
static	int	ibmrev();
static	int	ibmopen();


#ifdef MEMACS_DLL
/* Most of memacs not part of neuron.exe but in a dll */
/* This is the interface to the dll. This file is part of neuron.exe */

#include "../windll/dll.h"

extern char* neuron_home;
typedef int (*Pfri)();
static Pfri main_, vtinit_, refresh_, ttputc_, ttflush_, quit_;

BUFFER* emacs_curbp;
WINDOW* emacs_curwp;

static int load_memacs() {
	struct DLL* dll;
	char buf[256];
	sprintf(buf, "%s\\lib\\memacs.dll", neuron_home);
	dll = dll_load(buf);
	if (dll) {
		main_ = (Pfri)dll_lookup(dll, "_emacs_main");
		vtinit_ = (Pfri)dll_lookup(dll, "_emacs_vtinit");
		refresh_ = (Pfri)dll_lookup(dll, "_emacs_refresh");
		ttputc_ = (Pfri)dll_lookup(dll, "_emacs_ttputc");
		ttflush_ = (Pfri)dll_lookup(dll, "_emacs_ttflush");
		quit_ = (Pfri)dll_lookup(dll, "_emacs_quit");
		return 1;
	}else{
		return 0;
	}
}

int emacs_main(argc, argv) int argc; char** argv; {
	static int first = 1;
	if (first) {
		first = 0;
		if (!load_memacs())
			hoc_execerror("Couldn't load $NEURONHOME/lib/memacs.dll", (char*)0);
		}
	if (main_) {
		(*main_)(argc, argv);
	}
	return 0;
}

int emacs_vtinit() {
	if (vtinit_) {
		(*vtinit_)();
	}
	return 0;
}

int emacs_refresh(f, n) int f, n; {
	if (refresh_) {
		(*refresh_)(f, n);
	}
	return 0;
}

int emacs_ttputc(c) int c; {
	if (ttputc_) {
		(*ttputc_)(c);
	}
	return 0;
}

int emacs_ttflush() {
	if (ttflush_) {
		(*ttflush_)();
	}
	return 0;
}

int emacs_quit(f, n) int f, n; {
	if (quit_) {
		(*quit_)();
	}
	return 0;
}

#endif

/*
 * Standard terminal interface dispatch table. Most of the fields point into
 * "termio" code.
 */
TERM	term	= {
	NROW-1,
	NCOL,
	MARGIN,
	SCRSIZ,
	ibmopen,
#ifdef _Windows
	ibmclose,
#else
	ttclose,
#endif
	ibmgetc,
	ibmputc,
	ibmflush,
	ibmmove,
	ibmeeol,
	ibmeeop,
	ibmbeep,
	ibmrev
};


static int ibmmove(row, col)
{
#ifndef _Windows
	union REGS regs;
	regs.h.ah = 0x02;
	regs.h.bh = 0;
	regs.h.dh = row;
	regs.h.dl = col;
	int86(0x10, &regs, &regs);
#else
	em_goto(hwnd, row, col);
#endif
	return 0;
}

static ibmeeol()
{
#ifdef _Windows
	em_clr_eol(hwnd);
#else
	union REGS regs;
	regs.h.ah = 0x03;
	regs.h.bh = 0;
	int86(0x10, &regs, &regs);

	regs.h.ah = 6;
	regs.h.al = 0;
	regs.h.bh = 7;
	regs.h.ch = regs.h.dh;
	regs.h.cl = regs.h.dl;
	regs.h.dl = NCOL - 1;
	int86(0x10, &regs, &regs);
#endif
}
#ifdef _Windows
int clreol() {printf("clreol\n"); return 0;}
#endif

static int ibmeeop()
{
#ifdef _Windows
	em_clear(hwnd);
#else
	union REGS regs;
	regs.h.ah = 6;
	regs.h.al = 0;
	regs.h.bh = 7;
	regs.h.ch = 0;
	regs.h.cl = 0;
	regs.h.dh = NROW - 1;
	regs.h.dl = NCOL - 1;
	int86(0x10, &regs, &regs);
	ibmmove(0, 0);
#endif
	return 0;
}

static int ibmgetc()
{
#ifdef _Windows
	return getch();
#else
	union REGS regs;
	regs.h.ah = 7;
	intdos(&regs,&regs);
	return (int)regs.h.al;
#endif
}
#ifdef _Windows
int getch() { return fgetchar(); return 0;}
#endif


static int ibmputc(c)
{
#if 1
#ifdef _Windows
	  em_putchar(hwnd, c);
#else
	cprintf("%c",c);
#endif
#else
	union REGS regs;
	regs.h.ah = 14;
	regs.h.bl = 7;
	regs.h.al = c;
	int86(0x10, &regs, &regs);
#endif
	return 0;
}

static ibmflush()
{
}

static int ttputc(int a) {
	ibmputc(a);
	return 0;
}

static int ibmbeep()
{
	ttputc(BEL);
	ibmflush();
	return 0;
}

static int ibmparm(n)
register int	n;
{

	register int	q;

	q = n/10;
	if (q != 0)
		ibmparm(q);
	ttputc((n%10) + '0');
	return 0;
}

#endif

int ibmopen()
{
#ifndef _Windows
	textmode(3);
	textcolor(2);
#endif

	ttopen();
	return 0;
}
#ifdef _Windows
void destroy_func(HWND hwnd) {
	winio_onclose(hwnd, 0);
	quit(0,0);
	winio_onclose(hwnd, destroy_func);
}

int ttopen() {
	int row, col;
	hwnd = winio_current();
	winio_setecho(hwnd, FALSE);
	em_open(hwnd, destroy_func);
	em_size(hwnd, &row, &col);
	term.t_nrow = row;
	term.t_ncol = col;
	term.t_nrow;
	return 0;
}
int ibmclose() {
	em_close(hwnd, destroy_func);
	return 0;
}
#endif

static int ibmrev(state)		/* change reverse video state */

int state;	/* TRUE = reverse, FALSE = normal */

{
/*ANSI	ttputc(ESC);
	ttputc('[');
	ttputc(state ? '7': '0');
	ttputc('m');
*/
	return 0;
}


