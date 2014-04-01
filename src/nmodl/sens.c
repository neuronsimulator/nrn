#include <../../nmodlconf.h>
/* /local/src/master/nrn/src/nmodl/sens.c,v 4.2 1997/11/28 15:11:43 hines Exp */
/*
sens.c,v
 * Revision 4.2  1997/11/28  15:11:43  hines
 * absolute tolerance for CVODE on a per state basis.
 * Interface is a spec of absolute tolerance within a .mod file for the
 * declaration of a STATE as in
 * 	state (units) <tolerance>
 * Within nrniv, one specifies tolerance via
 * tol = cvode.abstol(&var, tolerance) where var is any variable whose address
 * can be taken (although only STATEs make use of a tolerance).
 * The address aspect of the above is misleading since tolerances are the
 * same for any single name, eg cvode.abstol(&v(.5)) changes tolerances for
 * ALL membrane potentials globally. The only purpose of the address is
 * to unambiguously identify the Symbol for the name. Perhaps string spec
 * such as "TrigKSyn.G" will be incorporated in the future.
 * when an absolute tolerance is changed, cvode will re-initialize the
 * tolerances next time Cvode.re_init is called. The tolerance actually
 * used for a STATE is the  minimum between the value specified in the second
 * arg of cvode.accuracy and the tolerance stored in the Symbol.
 *
 * Revision 4.1  1997/08/30  20:45:34  hines
 * cvs problem with branches. Latest nmodl stuff should now be a top level
 *
 * Revision 4.0.1.1  1997/08/08  17:24:01  hines
 * nocmodl version 4.0.1
 *
 * Revision 4.0  1997/08/08  17:06:28  hines
 * proper nocmodl version number
 *
 * Revision 1.2  1995/09/05  17:57:58  hines
 * allow domain limit in parameter spec. the syntax is of the form
 * name '=' number units '[' number ',' number ']'
 * The brackets may be changed to <...> if the syntax for arrays is ambiguous
 *
 * Revision 1.1.1.1  1994/10/12  17:21:37  hines
 * NEURON 3.0 distribution
 *
 * Revision 9.76  90/12/07  09:27:25  hines
 * new list structure that uses unions instead of void *element
 * 
 * Revision 9.58  90/11/20  17:24:17  hines
 * CONSTANT changed to PARAMETER
 * CONSTANT now refers to variables that don't get put in .var file
 * 
 * Revision 9.45  90/10/30  13:56:56  hines
 * derivative blocks (this impacts kinetic and sens as well) now return
 * _reset which can be set with RESET statement.  _reset is static in the
 * file and set to 0 on entry to a derivative or kinetic block.
 * 
 * Revision 9.32  90/10/08  14:12:55  hines
 * index vector instead of pointer vector for slist and dlist
 * 
 * Revision 8.2  90/02/07  10:23:23  mlh
 * It is important that blocks for derivative and sensitivity also
 * be declared static before their possible use as arguments to other
 * functions and that their body also be static to avoid multiple
 * declaration errors.
 * 
 * Revision 8.1  90/01/16  11:06:16  mlh
 * error checking and cleanup after error and call to abort_run()
 * 
 * Revision 8.0  89/09/22  17:26:57  nfh
 * Freezing
 * 
 * Revision 7.0  89/08/30  13:32:33  nfh
 * Rev 7 is now Experimental; Rev 6 is Testing
 * 
 * Revision 6.0  89/08/14  16:27:13  nfh
 * Rev 6.0 is latest of 4.x; now the Experimental version
 * 
 * Revision 4.1  89/08/07  15:35:26  mlh
 * freelist now takes pointer to list pointer and 0's the list pointer.
 * Not doing this is a bug for multiple sens blocks, etc.
 * 
 * Revision 4.0  89/07/24  17:03:43  nfh
 * Freezing rev 3.  Rev 4 is now Experimental
 * 
 * Revision 3.2  89/07/18  11:55:19  mlh
 * first_time removed and MODEL_LEVEL used for declaration precedence
 * 
 * Revision 1.2  89/07/18  11:22:19  mlh
 * eliminate first_time, etc.
 * 
 * Revision 1.1  89/07/06  14:50:34  mlh
 * Initial revision
 * 
*/

#include "modl.h"
#include "parse1.h"

static List *sensinfo; /* list of pairs: first is the block symbol where
				the SENS statement appeared.  The second is
				a list of statements which goes after the
				SOLVE statement. Used for NONLINEAR blocks.
				So far sensmassage gets control when
				massageblock is almost finished
			*/
static List *statelist;
static List *parmlist;
extern Symbol *indepsym;
int sens_parm = 0;

void sensparm(qparm)
	Item *qparm;
{
	if (!parmlist)
		parmlist = newlist();
	Lappendsym(parmlist, SYM(qparm));
	sens_parm++;
}

void add_sens_statelist(s)
	Symbol *s;
{
	if (!statelist)
		statelist = newlist();
	Lappendsym(statelist, s);
}

void sensmassage(type, qfun, fn)
	int type;
	Item *qfun;
{
/*qfun is the list symbol for the name of the derivative block. It has
a count of the number of state variables used.  A copy of this symbol
is made but with the name S_name and qfun is made to point to the new symbol
The old function name is then the name of a new created function which
contains the calls to trajecsens followed by a call to S_name to compute the
Note that trajecsens itself contains calls to S_name.
qfun->sym->used is then multiplied by the (1 + #sens parms used) so that
the solve code doesn't need to be changed.

The slist needs to be augmented and that is done here also. However
since it is declared in solve.c it is necessary to access the number
of parameters being used.
*/

/* extending to nonlinear blocks. Differences:
Number of states remains the same. Statements built here but saved for output
when SOLVE is translated. No derivative variables constructed.
However, the full dlist is constructed since we need the last part
for use by EM variables
This has gotten much more difficult to understand since the nonlinear method
is merged into the code to take advantage of common code.  It would be
conceptually simpler to merely repeat the whole process in a separate file*/
/* extending to linear blocks. Same as nonlinear except that
there is a linearsens call and we must be sure to keep proper state order */
	int nstate, i, j, newjac;
	char sname[100], dname[100];
	Item *q, *q1;
	List *senstmt;	/* nonlinear sens statements (saved in sensinfo) */
	Symbol *oldfun, *newfun, *s;
	
	oldfun = SYM(qfun);
	Sprintf(buf, "S_%s", oldfun->name);
	if (lookup(buf)) {
		diag(buf, " is user defined and cant be used for SENS");
	}
	/*this is a time bomb*/
	newfun = install(buf, oldfun->type);
	newfun->subtype = oldfun->subtype;
	newfun->u.i = oldfun->u.i; /*the listnum*/
	newfun->used = oldfun->used; /*number of states*/
	nstate = oldfun->used;
	if (type == DERIVATIVE) {
		oldfun->used *= (1 + sens_parm);
	}
/* even derivatives need sensinfo since envelope statements go after the
SOLVE for statement */
	if (!sensinfo) {
		sensinfo = newlist();
	}
	Lappendsym(sensinfo, oldfun); /* newton will call oldfun
		which will merely call newfun */
	q = lappendsym(sensinfo, SYM0); /* the second element is a list
		of statements to be constructed below */
	senstmt = newlist();
	LST(q) = senstmt;

	SYM(qfun) = newfun; /* the derivative equations alone are now
				called newfun->name; oldfun will contain
				the trajecsens calls */

	/* build the oldfun function */
	/* In the nonlinear case all statements except call to newfun get
		sent to senstmt */
	/* in the derivative case envelope statements get sent to senstmt */
	Sprintf(buf, "\nstatic int %s() {\n", oldfun->name);
	Lappendstr(procfunc, buf);
	Sprintf(buf, "\nstatic int %s();\n", newfun->name);
	Linsertstr(procfunc, buf);
	newjac = 1;
	i=1;
	ITERATE(q, parmlist) {

if (type == DERIVATIVE) {
		Sprintf(buf, "error=trajecsens(%d, _slist%d, _dlist%d,\
_p, &%s, %s, %s, %d, _slist%d+%d, _dlist%d+%d);\n if(error){abort_run(error);}\n",
nstate,
fn, fn,
SYM(q)->name, newfun->name, indepsym->name, newjac,
fn, nstate*i,
fn, nstate*i);
		Lappendstr(procfunc, buf);
}else if (type == NONLINEAR) {
		Sprintf(buf, "error=steadysens(%d, _slist%d, _p, &%s, %s,\
 _dlist%d, %d, _slist%d+%d);\n if(error){abort_run(error);}\n",
			nstate,
			fn,
			SYM(q)->name, newfun->name, fn, newjac,
			fn, nstate*i);
		Lappendstr(senstmt, buf);
}else if (type == LINEAR) {
		Sprintf(buf, "error=linearsens(%d, _slist%d, _p, &%s, %s,\
 _coef%d, %d, _slist%d+%d);\n if(error){abort_run(error);}\n",
			nstate,
			fn,
			SYM(q)->name, newfun->name, fn, newjac,
			fn, nstate*i);
		Lappendstr(senstmt, buf);
}
		newjac = 0;
		/* define S_state_parm and DS_state_parm */
		ITERATE(q1, statelist) {
			j = SYM(q1)->varnum;
			Sprintf(sname, "S_%s_%s", SYM(q1)->name, SYM(q)->name);
			Sprintf(dname, "D%s", sname);
			if ((s = lookup(sname)) == SYM0) {
				s = install(sname, NAME);
			}
			if (SYM(q1)->subtype & ARRAY) {
depinstall(1, s, SYM(q1)->araydim, "0", "1", "", ITEM0, 0, "");
			}else{
				depinstall(1, s, 0, "0", "1", "", ITEM0, 0, "");
			}
			s->usage |= DEP;
if (type == DERIVATIVE) {
			s = lookup(dname);
			assert (s);
			s->usage |= DEP;
			/* initialize augmented _slist and _dlist */
			if (SYM(q1)->subtype & ARRAY) {
Sprintf(buf, "for (_i=0;_i<%d;_i++){\
_slist%d[%d+_i] = (%s + _i) - _p; _dlist%d[%d+_i] = (%s + _i) - _p;}\n", SYM(q1)->araydim,
fn, j + nstate*i, sname, fn, j + nstate*i, dname);
			}else{
Sprintf(buf, "_slist%d[%d] = &(%s) - _p; _dlist%d[%d] = &(%s) - _p;\n",
fn, j + nstate*i, sname, fn, j + nstate*i, dname);
			}
}else if (type == NONLINEAR || type == LINEAR) {
			if (SYM(q1)->subtype & ARRAY) {
				Sprintf(buf, "for (_i=0;_i<%d;_i++){\
_slist%d[%d+_i] = (%s + _i) - _p;}\n", SYM(q1)->araydim,
					fn, j + nstate*i, sname);
			}else{
				Sprintf(buf, "_slist%d[%d] = &(%s) - _p;\n",
					fn, j + nstate*i, sname);
			}
}
			Lappendstr(initlist, buf);
		}
		i++;
	}

/* addition of envelope calls by modifying copy of above code
 create EP_state_parm, EM_state_parm using a new eplist and emlist
 respectively.  Also create U_parm with default value of .05 if it
 doesn't already exist as a constant.
 */
 
Sprintf(buf, "static int _eplist%d[%d], _emlist%d[%d];\n",
   fn, nstate*sens_parm, fn, nstate*sens_parm);
   	Linsertstr(procfunc, buf);
   	i = 0;
	ITERATE(q, parmlist) {

		Sprintf(buf, "envelope(_p, _slist%d, %d, %s, U_%s,\
_slist%d+%d, _eplist%d + %d, _emlist%d + %d);\n",
fn, nstate,
SYM(q)->name, SYM(q)->name,
fn, nstate*(1 + i),
fn, nstate*i,
fn, nstate*i);
		Lappendstr(senstmt, buf);

		/* define the uncertainty variables */
		Sprintf(buf, "U_%s", SYM(q)->name);
		IGNORE(ifnew_parminstall(buf, "0.05", "", ""));
		/* define EP_state_parm and EM_state_parm */
		ITERATE(q1, statelist) {
			j = SYM(q1)->varnum;
			Sprintf(sname, "EP_%s_%s", SYM(q1)->name, SYM(q)->name);
			Sprintf(dname, "EM_%s_%s", SYM(q1)->name, SYM(q)->name);
			if ((s = lookup(sname)) == SYM0) {
				s = install(sname, NAME);
			}
			if (SYM(q1)->subtype & ARRAY) {
depinstall(0, s, SYM(q1)->araydim, "0", "1", "", ITEM0, 0, "");
			}else{
				depinstall(0, s, 0, "0", "1", "", ITEM0, 0, "");
			}
			s->usage |= DEP;

			if ((s = lookup(dname)) == SYM0) {
				s = install(dname, NAME);
			}
			if (SYM(q1)->subtype & ARRAY) {
depinstall(0, s, SYM(q1)->araydim, "0", "1", "", ITEM0, 0, "");
			}else{
				depinstall(0, s, 0, "0", "1", "", ITEM0, 0, "");
			}
			s->usage |= DEP;

			/* initialize augmented _slist and _dlist */
			if (SYM(q1)->subtype & ARRAY) {
Sprintf(buf, "for (_i=0;_i<%d;_i++){\
_eplist%d[%d+_i] = (%s + _i) - _p; _emlist%d[%d+_i] = (%s + _i) - _p;}\n", SYM(q1)->araydim,
fn, j + nstate*i, sname, fn, j + nstate*i, dname);
			}else{
Sprintf(buf, "_eplist%d[%d] = &(%s) - _p; _emlist%d[%d] = &(%s) - _p;\n",
fn, j + nstate*i, sname, fn, j + nstate*i, dname);
			}
			Lappendstr(initlist, buf);
		}
		i++;
	}
	if (type == DERIVATIVE) {
		Sprintf(buf, "return %s();\n}\n", newfun->name);
	}else{
		Sprintf(buf, "%s();\n}\n", newfun->name);
	}
	Lappendstr(procfunc, buf);
	freelist(&parmlist);
	freelist(&statelist);
	sens_parm = 0;
}

void sens_nonlin_out(q, fun)
	Item *q;
	Symbol *fun;
{
	/* find fun in the sensinfo list and insert its statements before q */
	
	Item *q1, *q2, *q3;
	
	if (!sensinfo) return;
	ITERATE(q1, sensinfo) {
		q2 = q1->next;
		if (SYM(q1) == fun) {
			ITERATE(q3, LST(q2)) {
				Insertstr(q, STR(q3));
			}
		}
		q1 = q2;
	}
}
