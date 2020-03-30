#include <../../nrnconf.h>

#define FIG 1/* version 7.1.1 12/8/88
   added plots in fig format
   */
#include "hoc.h"
#include "gui-redirect.h"
extern Object** (*nrnpy_gui_helper_)(const char* name, Object* obj);
extern double (*nrnpy_object_to_double_)(Object*);

extern void Fig_file(const char*, int);

#if !defined(CYGWIN)

void Plt(void)
{
	TRY_GUI_REDIRECT_DOUBLE("plt", NULL);
	int mode;
	double x, y;
#ifndef WIN32
	mode = *getarg(1);
	if (mode >= 0 || ifarg(2))
	{
		if ((x = *getarg(2)) > 2047)
			x = 2047;
		else if (x < 0)
			x = 0;
		if ((y = *getarg(3)) > 2047)
			y = 2047;
		else if (y < 0)
			y = 0;
	}else{
		x=y=0.;
	}
	plt(mode, x, y);
#endif
	ret();
	pushx(1.);
}

void Setcolor(void)
{
	TRY_GUI_REDIRECT_DOUBLE("setcolor", NULL);
	double i;
	i = set_color((int)*getarg(1));
	ret();
	pushx(i);
}

void hoc_Lw(void)
{
	char *s;
	static int dev=2;
#ifndef WIN32
	if (ifarg(1)) {
		s = gargstr(1);
		if (ifarg(2)) {
			dev = *getarg(2);
		}
		if (s[0] != '\0') {
			Fig_file(s, dev);
		} else {
			Fig_file((char *)0, dev);
		}
	} else {
		Fig_file((char *) 0, dev);
	}
#endif
	ret();
	pushx(0.);
}

#endif /*!defined(CYGWIN)*/
