#include <../../nrnconf.h>
/* /local/src/master/nrn/src/oc/fmenu.c,v 1.4 1996/02/16 16:19:25 hines Exp */
/*
fmenu.c,v
 * Revision 1.4  1996/02/16  16:19:25  hines
 * OCSMALL used to throw out things not needed by teaching programs
 *
 * Revision 1.3  1995/07/22  13:01:47  hines
 * avoid unhandled exceptions in mswindows due to some function stubs
 *
 * Revision 1.2  1994/11/23  19:52:57  hines
 * all nrnoc works in dos with go32
 *
 * Revision 1.1.1.1  1994/10/12  17:22:08  hines
 * NEURON 3.0 distribution
 *
 * Revision 2.19  93/02/02  10:34:25  hines
 * static functions declared before used
 * 
 * Revision 1.3  92/08/18  07:31:36  hines
 * arrays in different objects can have different sizes.
 * Now one uses araypt(symbol, SYMBOL) or araypt(symbol, OBJECTVAR) to
 * return index of an array variable.
 * 
 * Revision 1.2  91/10/14  17:36:08  hines
 * scaffolding for oop in place. Syntax about right. No action yet.
 * 
 * Revision 1.1  91/10/11  11:12:01  hines
 * Initial revision
 * 
 * Revision 4.20  91/04/03  16:01:33  hines
 * mistyped ||
 * 
 * Revision 4.9  91/01/04  09:51:08  hines
 * turboc++ sometimes fails with __TURBOC__ but succeeds with
 * #if defined(__TURBOC__)
 * 
 * Revision 3.77  90/07/20  09:45:49  hines
 * case 3 allows actions to be executed when variable is changed
 * 
 * Revision 3.50  90/02/17  10:12:25  mlh
 * lint free on sparc and makfile good for sparc
 * 
 * Revision 3.44  90/01/05  14:57:24  mlh
 * min and max along with Jamie's changes that allow person to match
 * upper char by typing lower char (works only with turboc)
 * 
 * Revision 3.41  89/12/08  15:34:47  mlh
 * infinite loop when searching for non-existent character starting
 * at first menu item.
 * Corrected with do{}while control structure.
 * 
 * Revision 3.20  89/08/15  08:29:42  mlh
 * compiles under turbo-c 1.5 -- some significant bugs found
 * 
 * Revision 3.7  89/07/13  08:21:26  mlh
 * stack functions involve specific types instead of Datum
 * 
 * Revision 3.4  89/07/12  10:26:55  mlh
 * Lint free
 * 
 * Revision 3.3  89/07/10  15:45:56  mlh
 * Lint pass1 is silent. Inst structure changed to union.
 * 
 * Revision 2.0  89/07/07  11:36:43  mlh
 * *** empty log message ***
 * 
 * Revision 1.1  89/07/07  11:15:57  mlh
 * Initial revision
 * 
*/

/* Copyright 1989,88,87- M.L. Hines, Neurobiology Dept.,DUMC, Durham, NC
*
* REVISION HISTORY:
*
* 5-19-89 let return start entry of number
* 5-04-89 Get it going on the SUN.
* 4-17-89 Added if HOC if FOCEXT statements to distinguish between FOCAL
* and HOC versions.
*
* Synopsis	fmenu is a FOCAL menu management function
*
*			x fmenu(args)	where the following options are available:
*
*			fmenu(nmenu,-1) allocates space for nmenu number of menus
*					Menu identifier numbers start at 0,1,...nmenu-1
*			fmenu(imenu,0)	erase previous menu identified by imenu.
*			
*			fmenu(imenu,1,var list) add variables specified in list
*						to imenu. The variable names will 
*						be added sequentially in the order
*						specified.
*
*			fmenu(imenu,2,"prompt","command") add the executable command
*						specified by a prompt,command pair
*						to imenu.
*
*			fmenu(imenu)		executes menu imenu, displays, 
*						navigates through imenu.
*
*
*			Version 1.0 written by M.V. Evans and M.L. Hines 4-12-89
*/

#include <stdlib.h>

#if defined(WITHOUT_MEMACS)
#define OCSMALL 1
#endif

#if OCSMALL
void hoc_fmenu(void) {
	hoc_ret(); hoc_pushx(0.);
}
hoc_menu_cleanup() {
}
#else
#if defined(__GO32__)
#define G32 1
#include <dos.h>
#include <gppconio.h>
extern int egagrph; /* detect if in graphics mode */
#endif

#if DOS
#include <dos.h>
#include <io.h>
union REGS regs;
extern int egagrph; /* detect if in graphics mode */
#else
#if !G32
static int egagrph = 0;
#include "estruct.h"
extern TERM emacs_term;
#endif
#endif
#include "hoc.h"
#include <ctype.h>
#if HOC | OOP
#define Ret(a)   hoc_ret(); hoc_pushx(a)
#endif
#define NUL	0
#define SPACE	'\040'
#if DOS || G32
#define BEEP	Printf("\007")
#else
#define BEEP	(*emacs_term.t_beep)()
#endif
/* structure and functions from getsym.c */
#include "hocgetsym.h"

/* Structure for single menu list */
typedef struct Menuitem{
		struct Menuitem *pprevious; /* Pointer to a previous item */
		short row, col; /* Coordinates of each menu item */
		short type;
		char	*prompt;  /* prompt, command used for actions */
		char	*command;
		Psym	*psym;
		double	symmin;	/* min and max value for sym */
		double	symmax;
		struct Menuitem *nextitem; /* Pointer to next menu item */
} Menuitem;

/* menu types */
#define MENU_VAR	1
#define MENU_ACTION	2

/* Summary of menu functions :
	insert_menu(int r, int c, sym *sp ) - Stores row and col coordinates, and the symbol pointer  specified in fcursor() as an item
in a structure list. Successive items are appended at the end of the list. This function returns the pointer for the beginning of the list.
	display_menu(Menuitem *menu) - Displays the whole menu as specified by successive calls to fcursor. Recovers information stored for each menu item and prints the corresponding item variable name and value at its specified position.
	destroy_menu(Menuitem *menu) - Frees the space allocated for the whole menu list.
	navigate_menu(Menuitem *menu) - Allows user to move around the displayed menu by making use of the arrow keys.
	erase_item(Menuitem *pnow) - Erases second line contents in menu
*/


static int cexecute(const char*);
static char *navigate(int);
static Menuitem *append();	/*common code for appendsym,appendaction*/
static void appendvar(int, const char*, const char*);
static void appendaction(int, const char*, const char*);
static void destroy(int);
static double enter(int, int, double, int, Menuitem*);
static void prval(int, int, int, double);
static void prs(int, int, int, const char*);
static void undisplay(int);

/* Structure pointer summary:
	*pprev - pointer to previous item structure
	*pnow - pointer to current item structure
	*nextitem - pointer to next item - forward link
	*pprevious - pointer to previous item - reverse link
*/

static int current_menu = -1;	/* current menu number */
static int maxmenus;
static Menuitem **menusfirst; /* pointers to first menuitem in list*/
static Menuitem **menuslast; /* pointers to last menuitem in list*/
static Menuitem **menuscurrent; /* pointers to where navigate starts*/
static int first = 1;		/* emacs_term has not been opened */

#define diag(s) hoc_execerror(s, (char*)0);
#define chk(i)	{if (i < 0 || i >= maxmenus) diag("menu number out of range");}
static void menu_manager(int nmenu) {
	int previous;
	char *command;
	previous = current_menu;
	current_menu = nmenu;
#if DOS || G32
#else
	if (first) {
		(*emacs_term.t_open)();
		(*emacs_term.t_close)();
		first = 0;
	}
#endif
	if (previous >= 0) {
		undisplay(previous);
	}else{
		undisplay(current_menu);
	}
	while((command = navigate(current_menu)) != (char *)0) {
		if (cexecute(command) == 4) { /* 4 means stop was executed */
			break;
		}
	}
	if (previous >= 0) {
		undisplay(current_menu);
	}
	current_menu = previous;
}

void hoc_fmenu(void){
	int imenu, flag, i, narg;
#ifdef WIN32
	hoc_execerror("fmenu not available under mswindows.", "Use xpanel series");
#endif
	imenu = *getarg(1);
	if (!ifarg(2)) { /* navigate the menu */
		chk(imenu);
		menu_manager(imenu);
		Ret(0.);
		return ;
	}
	flag = *getarg(2);
	narg = 2;
	switch(flag)
	{
		case -1:
			if (current_menu != -1) {
				diag("can't destroy current menu");
			}
			if (maxmenus) {
				for (i = 0; i<maxmenus; i++) {
					destroy(i);
				}
				free((char *)menusfirst);
				free((char *)menuslast);
			}
			maxmenus = 0;
menusfirst = (Menuitem **)emalloc((unsigned)(imenu*sizeof(Menuitem *)));
menuslast = (Menuitem **)emalloc((unsigned)(imenu*sizeof(Menuitem *)));
menuscurrent = (Menuitem **)emalloc((unsigned)(imenu*sizeof(Menuitem *)));

			maxmenus = imenu;
			for (i=0; i<maxmenus; i++) {
				menusfirst[i] = menuslast[i] = menuscurrent[i]
				 = (Menuitem *)0;
			}
			break;
		case 0:
			chk(imenu);
			if (current_menu == imenu) {
				diag(" can't destroy current menu");
			}
			destroy(imenu);
			appendaction(imenu, "Exit", "stop");
			break;
		case 1:
			while (ifarg(narg=narg+1)) {
				appendvar(imenu, gargstr(narg), (char *)0);
				menuslast[imenu]->symmin = *getarg(narg=narg+1);
				menuslast[imenu]->symmax = *getarg(narg=narg+1);
			}
			break;
		case 2:
			while (ifarg(narg=narg+1)) {
				char *prompt, *command;
				prompt = gargstr(narg);
				command = gargstr(narg=narg+1);
				appendaction(imenu, prompt, command);
			}
			break;
		case 3:
			while (ifarg(narg=narg+1)) {
				appendvar(imenu, gargstr(narg), gargstr(narg+1));
				menuslast[imenu]->symmin = *getarg(narg=narg+2);
				menuslast[imenu]->symmax = *getarg(narg=narg+1);
			}
			break;
		default:
			diag("illegal argument flag");
			break;
	}
	Ret (0.);
}

static void xcursor(int r, int c){
#if DOS
	_BH = 0;
	_DH = r;
	_DL = c;
	_AH = 2;
	geninterrupt(0x10);
#else
#if G32
	union REGS regs;
	regs.h.ah = 0x02;
	regs.h.bh = 0;
	regs.h.dh = r;
	regs.h.dl = c;
	if (egagrph) {
		grx_move(r, c);
	}else{
		int86(0x10, &regs, &regs);
	}
#else
	(*emacs_term.t_move)(r, c);
#endif
#endif
}

static int ibmgetc(void){       /* Copied from ibm.c file in memacs */
#if DOS
	regs.h.ah = 7;
	intdos(&regs,&regs);
	return (int)regs.h.al;
#else
#if G32
	union REGS regs;
	regs.h.ah = 7;
	intdos(&regs,&regs);
	return (int)regs.h.al;
#else
	return (*emacs_term.t_getchar)();
#endif
#endif
}

static Menuitem *append(int imenu){
	Menuitem *last, *pnow;

	if (imenu < 0 || imenu >= maxmenus) {
		diag("menu number out of range");
	}
	last = menuslast[imenu];
	pnow = (Menuitem *)emalloc(sizeof(Menuitem));

	pnow->pprevious = last;
	pnow->nextitem = (Menuitem *)0;
	menuslast[imenu] = pnow;

	if (last) {
		int col = last->col, row = last->row;
		last->nextitem = pnow;
		col += 13;
		if (col > 77) {
			row += 2;
			col = 0;
		}
		pnow->row = row;
		pnow->col = col;
	}else{
		menusfirst[imenu] = pnow;
		pnow->row = 0;
		pnow->col = 0;
		menuscurrent[imenu] = pnow;
	}

	pnow->type = 0;
	pnow->prompt = (char *)0;
	pnow->command = (char *)0;
	pnow->psym = (Psym *)0;
	return (pnow);
}

static void appendvar(int imenu, const char* variable, const char* command)
{
	Menuitem *item;
	int i, len;
	char buf[256];
	Psym *p;

	item = append(imenu);
	item->type = MENU_VAR;
	item->psym = p  = hoc_getsym(variable);
	if (command) {
		item->command = (char *)emalloc((unsigned)(strlen(command) + 1));
		Strcpy(item->command, command);
	}else{
		item->command = (char *)0;
	}
	Sprintf(buf, "%s", p->sym->name);
	len = strlen(buf);
	for (i = 0; i < p->nsub; i++) {
		Sprintf(buf+len, "[%d]", p->sub[i]);
		len = strlen(buf);
	}
	item->prompt = (char *)emalloc((unsigned)(len+1));
	Strcpy(item->prompt, buf);
}

static void appendaction(int imenu, const char* prompt, const char* command)
{
	Menuitem *item;
	item = append(imenu);
	item->type = MENU_ACTION;
	item->prompt = (char *)emalloc((unsigned)(strlen(prompt) + 1));
	Strcpy(item->prompt, prompt);
	item->command = (char *)emalloc((unsigned)(strlen(command) + 1));
	Strcpy(item->command, command);
}

static void display(int imenu) {

	Menuitem *menu, *pnow;
	int row, col;

	chk(imenu);
	menu = menusfirst[imenu];
	for(pnow=menu;pnow;pnow=pnow->nextitem){
		row = pnow->row;
		col = pnow->col;
		prs(0, row, col, pnow->prompt);
		switch (pnow->type) {
		case MENU_VAR:
			prval(0, row+1, col, hoc_getsymval(pnow->psym));
			break;
		}
	}
	xcursor(menuslast[imenu]->row+2, 0);
}


static void destroy(int imenu) {
	Menuitem *menu;
	Menuitem *pnow, *nextitem;

	menu = menusfirst[imenu];
	menusfirst[imenu] = (Menuitem *)0;
	menuslast[imenu] = (Menuitem *)0;
	menuscurrent[imenu] = (Menuitem *)0;
	for(pnow = menu;pnow;pnow = nextitem){
		nextitem = pnow->nextitem;
		if (pnow->prompt) {
			free(pnow->prompt);
		}
		if (pnow->command) {
			free((char *)pnow->command);
		}
		if (pnow->psym) {
			free_arrayinfo(pnow->psym->arayinfo);
			free((char *)pnow->psym);
		}
		free((char *)pnow);
	}
}

static char *navigate(int imenu) {
	Menuitem *menu;
	Menuitem *pcur, *pnow;
	int row, col, key, current_col;
	double val;

	menu = menusfirst[imenu];
	if (menu == (Menuitem *)0) {
		return (char *)0;
	}
#if DOS || G32
#else
	(*emacs_term.t_open)(); /* before return be sure to close */
#endif
	display(imenu);
	pcur = menuscurrent[imenu];
	key = 0;
	while (key != 27) {
		pnow = pcur;	/* pnow is fixed hereafter */
		row = pnow->row + 1;
		col = pnow->col;
		switch (pnow->type) {
		case MENU_VAR:
			val = hoc_getsymval(pnow->psym);
			prval(1, row, col, val);
			break;
		case MENU_ACTION:
			prs(1, row, col, "execute");
			break;
		}

		key = ibmgetc();
        		if(key == 27 || key == 3)
				goto label;
			if (key == 5) {
#if DOS || G32
				return "plt(-5)";
#else
				(*emacs_term.t_close)();
				return "plt(-3)";
#endif
			}
		if(key == 0) { /* arrow key pressed */
			key = ibmgetc();
			switch(key) {
			case 77: /* Right arrow key */
				pcur = pcur->nextitem;
                                if(pcur == NUL)
					pcur = menu;
				break;
			case 75: /* Left arrow key */
				pcur = pcur->pprevious;
				if (pcur == (Menuitem *)0) {
					pcur = menuslast[imenu];
				}
				break;
			case 80: /* Down key */
				current_col = pnow->col;
				do{
					pcur=pcur->nextitem;
					if(pcur == (Menuitem *)0) {
						pcur = menu;
					}
				}while(pcur->col != current_col);
				break;
			case 72: /* Up key*/
				current_col = pnow->col;
				do{
					pcur=pcur->pprevious;
                                        if (pcur == (Menuitem *)0) {
						pcur = menuslast[imenu];
					}
				}while(pcur->col != current_col);
				break;
			default:
				BEEP;
				break;
			}
		} else 	if( pnow->type == MENU_VAR && (
		 (isdigit(key) || key == '-' || key == '+'
		 || key == 015 || key =='.')) ){
			prs(0, row, col, "");
			val = enter(row, col,val,key, pnow);
			hoc_assignsym(pnow->psym, val);
			if (pnow->command) {
				prs(0, pnow->row+1, pnow->col, "executing");
				xcursor(menuslast[imenu]->row+2, 0);
#if DOS || G32
#else
				(*emacs_term.t_close)();
#endif
		
				return pnow->command;
			}
			prval(1, row, col, val);
		} else if (key == 015 && pnow->type == MENU_ACTION) {
			prs(0, pnow->row+1, pnow->col, "executing");
			xcursor(menuslast[imenu]->row+2, 0);
#if DOS || G32
#else
			(*emacs_term.t_close)();
#endif
		
			return pnow->command;
		} else if (isalpha(key)) {
			pcur = pnow;
			do {
				pcur = pcur->nextitem;
				if (pcur == (Menuitem *)0) {
					pcur = menusfirst[imenu];
				}
				if ( toupper(pcur->prompt[0]) == toupper(key) )
					break;
			} while (pcur!=pnow);
		} else {
			BEEP;
		}
		menuscurrent[imenu] = pcur;

		switch (pnow->type) { /* the old one */
		case MENU_VAR:
			prval(0, row, col, val);
			break;
		case MENU_ACTION:
			prs(0, row, col, "");
			break;
		}
	}
label:	xcursor(menuslast[imenu]->row+2, 0);
#if DOS || G32
#else
	(*emacs_term.t_close)();
#endif
		
	return (char *)0;
}

static double enter(int row, int col, double defalt, int frstch, Menuitem* pnow)
{
        char istr[80],*istrptr; int key;
	double input;
	char buf[10];

	istrptr = &istr[0];
	xcursor(row,++col);
	if(frstch !=13){
		*istrptr++ = frstch;
		sprintf(buf, "%c",istr[0]);
		plprint(buf);
	}
	for(;;) {
		key = ibmgetc();
		if(isdigit(key)|| key =='.' || key == 'e'
		  || key == '-' || key == '+'){
			sprintf(buf, "%c",key);
			plprint(buf);
			*istrptr++ = key;
			continue;
		} else if(key == 27){
			return (defalt);
		}else if (key == '\b') {
			if (istrptr > istr) {
#if G32
	if (egagrph) {
		grx_backspace(1);
		hoc_outtext(" ");
		grx_backspace(1);
	}else
#endif
				Printf("\b \b");
				*(--istrptr) = '\0';
			}
		}else if (key == 13){ /*return*/
			*istrptr = '\0';
			if(sscanf(istr, "%lf",&input) == 1)
				if (input >= pnow->symmin && input <= pnow->symmax) {
					return (input);
				} else { /* input out of range */
BEEP;
Sprintf(istr, "Range %-5g", pnow->symmin);
prs(0, pnow->row, pnow->col, istr);
Sprintf(istr, "To %-5g ", pnow->symmax);
prs(0, pnow->row + 1, pnow->col, istr);
key = ibmgetc();
BEEP;
prs(0, pnow->row, pnow->col, pnow->prompt);

					return (defalt);
				}
			else{
				return (defalt);
			}
		}else{
			BEEP;
			continue;
		}
	}
}/* end of function enter */

static void prval(int oldnew, int row, int col, double val)
{
        char string[100];
	Sprintf(string,"%g",val);
	prs(oldnew, row, col, string);
}

static void prs(int oldnew, int row, int col, const char* string)
{
	char buf[100];
	xcursor(row, col);
	if(oldnew == 0){
		sprintf(buf, "%-13s",string);
		plprint(buf);
	}else{
		sprintf(buf, "%13c", SPACE);
		plprint(buf);
		xcursor(row, col);
		sprintf(buf,"<%s>",string);
		plprint(buf);
	}
}

static int cexecute(const char* command) {
	int i;
	hoc_returning = 0;
	hoc_execstr(command);
	i = hoc_returning;
	hoc_returning = 0;
	return i;
}

#if DOS || G32
#else
static void clrscr(void){
	(*emacs_term.t_move)(0, 0);
	(*emacs_term.t_eeop)();
}
#endif

static void undisplay(int imenu) {
	int i;
	if(egagrph !=0){
#if GRX
		grx_txt_clear();
#else
		xcursor(menusfirst[imenu]->row, 0);
		for (i = menuslast[imenu]->row - menusfirst[imenu]->row + 2; i; i--) {
			Printf("%80c\n", SPACE);
		}
#endif
	}else{
		clrscr();
	}
}

void hoc_menu_cleanup(void) {
	current_menu = -1;
#if DOS || G32
#else
	if (!first) {
		(*emacs_term.t_close)();
	}
#endif
}

#endif /*OCSMALL*/
