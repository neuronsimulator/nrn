#include <../../nrnconf.h>
/* /local/src/master/nrn/src/oc/hocusr.c,v 1.2 1996/04/09 13:44:29 hines Exp */
/*
hocusr.c,v
 * Revision 1.2  1996/04/09  13:44:29  hines
 * Changed error message with user defined names already exist.
 * Reset errno when ropen/wopen fail.
 *
 * Revision 1.1.1.1  1994/10/12  17:22:10  hines
 * NEURON 3.0 distribution
 *
 * Revision 2.63  1993/11/04  15:55:48  hines
 * port to solaris2 (no more warnings)
 *
 * Revision 2.58  1993/08/15  12:54:30  hines
 * fix warning of function declared external and later static
 *
 * Revision 1.8  92/08/12  16:16:12  hines
 * uninsert mechanism_name
 * 
 * Revision 1.7  92/08/12  11:56:31  hines
 * hoc_fake_ret() allows calls to functions that do a ret(). These functions
 * can have no arguments and the caller must pop the stack and deal with
 * hoc_returning.
 * This was done so init... could be called both from hoc_spinit and from
 * hoc.
 * last function called by hoc_spinit in hocusr.c is hoc_last_init()
 * 
 * Revision 1.6  92/07/07  09:50:02  hines
 * chkarg moved to code2.c so always available
 * 
 * Revision 1.5  91/11/13  07:57:08  hines
 * userdouble uses old style sym->u.pval
 * 
 * Revision 1.4  91/11/05  11:24:11  hines
 * all neuron/examples produce same results with nrnoc as with neuron.
 * Found quite a few bugs this way.
 * 
 * Revision 1.3  91/10/18  14:40:44  hines
 * symbol tables now are type Symlist containing pointers to first and last
 * symbols.  New symbols get added onto the end.
 * 
 * Revision 1.2  91/10/17  15:01:34  hines
 * VAR, STRING now handled with pointer to value located in object data space
 * to allow future multiple objects. Ie symbol for var, string, objectvar
 * has offset into a pointer data space.
 * 
 * Revision 1.1  91/10/11  11:12:09  hines
 * Initial revision
 * 
 * Revision 4.28  91/05/31  09:13:04  hines
 * forgot to change arrayinstal args at one place.
 * 
 * Revision 4.24  91/05/09  09:34:13  hines
 * minor mods to interface hoc to NEMO. USERFLOAT, NEMONODE, NEMOAREA
 * 
 * Revision 3.106  90/10/24  07:02:12  hines
 * Now any user function whose name begins with init is executed on startup.
 * 
 * Revision 3.91  90/08/06  17:27:37  hines
 * do modl registration after init
 * 
 * Revision 3.90  90/08/06  16:12:26  hines
 * hocusr calls modl_reg
 * This means that hocusr is now independent of modl
 * 
 * Revision 3.87  90/07/31  14:11:44  hines
 * allow args ot hoc_register_var to be 0
 * 
 * Revision 3.85  90/07/31  14:02:36  hines
 * allow user files to directly register hoc variables via
 * hoc_register_var(DoubScal, DoubVec, IntFunc)
 * 
 * Revision 3.55  90/05/08  09:31:30  mlh
 * removed last lint message
 * 
 * Revision 3.46  90/01/24  06:37:55  mlh
 * emalloc() and ecalloc() are macros which return null if out of space
 * and then call execerror.  This ensures that pointers are set to null.
 * If more cleanup necessary then use hoc_Emalloc() followed by hoc_malchk()
 * 
 * Revision 3.23  89/09/14  14:34:04  mlh
 * for turboc 2.0 void the signal and malloc, and remove calloc
 * 
 * Revision 3.19  89/08/12  14:36:17  mlh
 * so newcable can be lint free
 * 
 * Revision 3.9  89/08/12  13:44:08  mlh
 * IGNORE defined in hocdec.h
 * 
 * Revision 3.8  89/07/14  14:11:06  mlh
 * pretty much lint free but having problems with declarations in
 * hocusr.h and use in hocusr.c
 * 
 * Revision 3.5  89/07/12  13:24:53  mlh
 * use the new Inst structure for defn
 * 
 * Revision 2.0  89/07/07  11:32:00  mlh
 * Preparation for newcable
 * 
 * Revision 1.1  89/07/07  11:16:30  mlh
 * Initial revision
 * 
*/

/* version 7.2.1 2-jan-89 */
#include <stdio.h>
#include <stdlib.h>
#include "hocdec.h"
#include "parse.h"
#if 1
#include "hocusr.h"
#endif

# define	CHECK(name)	if (hoc_lookup(name) != (Symbol *)0){\
		IGNORE(fprintf(stderr, CHKmes, name));\
		nrn_exit(1);}
static char	CHKmes[] = "The user defined name, %s, already exists.\n";

extern Symlist	*hoc_symlist;

static arayinstal();

hoc_spinit()	/* install user variables and functions */
{
	int i;
	Symbol *s;

	hoc_register_var(scdoub, vdoub, function);
	for (i = 0; scint[i].name; i++)
	{
		CHECK(scint[i].name);
		s = hoc_install(scint[i].name, UNDEF, 0.0, &hoc_symlist);
		s->type = VAR;
		s->u.pvalint = scint[i].pint;
		s->subtype = USERINT;
	}
	for (i = 0; scfloat[i].name; i++)
	{
		CHECK(scfloat[i].name);
		s = hoc_install(scfloat[i].name, UNDEF, 0.0, &hoc_symlist);
		s->type = VAR;
		s->u.pvalfloat = scfloat[i].pfloat;
		s->subtype = USERFLOAT;
	}
	for (i = 0; vint[i].name; i++)
	{
		CHECK(vint[i].name);
		s = hoc_install(vint[i].name, UNDEF, 0.0, &hoc_symlist);
		s->type = VAR;
		arayinstal(s, 1, vint[i].index1, 0, 0);
		s->u.pvalint = vint[i].pint;
		s->subtype = USERINT;
	}
	for (i = 0; vfloat[i].name; i++)
	{
		CHECK(vfloat[i].name);
		s = hoc_install(vfloat[i].name, UNDEF, 0.0, &hoc_symlist);
		s->type = VAR;
		arayinstal(s, 1, vfloat[i].index1, 0, 0);
		s->u.pvalfloat = vfloat[i].pfloat;
		s->subtype = USERFLOAT;
	}
	for (i = 0; ardoub[i].name; i++)
	{
		CHECK(ardoub[i].name);
		s = hoc_install(ardoub[i].name, UNDEF, 0.0, &hoc_symlist);
		s->type = VAR;
		arayinstal(s, 2, ardoub[i].index1, ardoub[i].index2, 0);
		s->u.pval = ardoub[i].pdoub;
		s->subtype = USERDOUBLE;
	}
	for (i = 0; thredim[i].name; i++)
	{
		CHECK(thredim[i].name);
		s = hoc_install(thredim[i].name, UNDEF, 0.0, &hoc_symlist);
		s->type = VAR;
		arayinstal(s, 3, thredim[i].index1, thredim[i].index2,
			thredim[i].index3);
		s->u.pval = thredim[i].pdoub;
		s->subtype = USERDOUBLE;
	}
	for (i = 0; function[i].name; i++)
	{
		if (!strncmp(function[i].name, "init", 4))
		{
			hoc_fake_call(hoc_lookup(function[i].name));
			(*function[i].func)();
			continue;
		}
	}
	hoc_last_init();
}

hoc_register_var(scdoub, vdoub, function)
	DoubScal *scdoub;
	DoubVec *vdoub;
	IntFunc *function;
{
	int i;
	Symbol *s;
	
	if (scdoub) for (i = 0; scdoub[i].name; i++)
	{
		CHECK(scdoub[i].name);
		s = hoc_install(scdoub[i].name, UNDEF, 0.0, &hoc_symlist);
		s->type = VAR;
		s->u.pval = scdoub[i].pdoub;
		s->subtype = USERDOUBLE;
	}
	if (vdoub) for (i = 0; vdoub[i].name; i++)
	{
		CHECK(vdoub[i].name);
		s = hoc_install(vdoub[i].name, UNDEF, 0.0, &hoc_symlist);
		s->type = VAR;
		arayinstal(s, 1, vdoub[i].index1, 0, 0);
		s->u.pval = vdoub[i].pdoub;
		s->subtype = USERDOUBLE;
	}
	if (function) for (i = 0; function[i].name; i++)
	{
		CHECK(function[i].name);
		s=hoc_install(function[i].name, FUN_BLTIN, 0.0, &hoc_symlist);
		s->u.u_proc->defn.pf = function[i].func;
		s->u.u_proc->nauto = 0;
		s->u.u_proc->nobjauto = 0;
	}
}

static
arayinstal(sp, nsub, sub1, sub2, sub3) /* set up arayinfo */
	Symbol *sp;
	int nsub, sub1, sub2, sub3;
{

	sp->type = VAR;
	sp->s_varn = 0;
	sp->arayinfo = (Arrayinfo *) emalloc(
		(unsigned)(sizeof(Arrayinfo)+nsub*sizeof(int)));
	sp->arayinfo->a_varn = (unsigned *) 0;
	sp->arayinfo->nsub = nsub;
	sp->arayinfo->sub[0] = sub1;
	if (nsub > 1)
		sp->arayinfo->sub[1] = sub2;
	if (nsub > 2)
		sp->arayinfo->sub[2] = sub3;
}

ret(x)		/* utility return for user functions */
	double x;
{
	hoc_ret();
	hoc_pushx(x);
}
