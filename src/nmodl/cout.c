#include <../../nmodlconf.h>
/* /local/src/master/nrn/src/nmodl/cout.c,v 4.1 1997/08/30 20:45:16 hines Exp */
/*
cout.c,v
 * Revision 4.1  1997/08/30  20:45:16  hines
 * cvs problem with branches. Latest nmodl stuff should now be a top level
 *
 * Revision 4.0.1.1  1997/08/08  17:23:39  hines
 * nocmodl version 4.0.1
 *
 * Revision 4.0  1997/08/08  17:06:04  hines
 * proper nocmodl version number
 *
 * Revision 1.1.1.1  1994/10/12  17:21:34  hines
 * NEURON 3.0 distribution
 *
 * Revision 9.170  93/05/17  11:06:36  hines
 * PI defined by some <math.h> so undefing it
 * 
 * Revision 9.158  93/02/02  10:32:17  hines
 * create static func before usage
 * 
 * Revision 9.157  93/02/01  15:17:09  hines
 * static functions should be declared before use.
 * inline is keyword for some compilers.
 * 
 * Revision 9.154  92/11/10  11:52:16  hines
 * when compiling a translated model on the CRAY (with _CRAY defined as 1)
 * the check table functions are taken out of the loops. This is
 * not entirely safe if tables are used (and dependencies have changed)
 * in direct calls to functions that call functions with tables. It is
 * reasonably safe for finitialize(), fadvance(), fcurrent(), and
 * calling the hoc_function itself.
 * 
 * Revision 9.147  92/09/27  17:45:43  hines
 * vectorized channel densities with _method3 fill thisnode.GC,EC instead
 * of rhs,d
 * 
 * Revision 9.144  92/08/06  09:57:51  hines
 * put cray pragmas at appropriate places
 * get rid of initmodel references to saveing t and incrementing ninit
 * 
 * Revision 9.142  92/08/05  16:22:43  hines
 * can vectorize hh. need work on tables though.
 * 
 * Revision 9.140  92/07/27  11:33:06  hines
 * some bugs fixed. no vectorizing except for SOLVE procedure (and that not done yet)
 * 
 * Revision 9.139  92/07/27  10:11:21  hines
 * Can do some limited vectorization. Much remains. Often fails
 * 
 * Revision 9.133  92/03/19  15:14:20  hines
 * creates a nrn_initialize function that will be called from finitialize().
 * 
 * Revision 9.128  92/02/05  14:47:46  hines
 * saber warning free
 * FUNCTION made global for use by other models. #define GLOBFUNC 1
 * 
 * Revision 9.122  91/10/28  09:09:24  hines
 * wathey's improvements. different binary architectures in different
 * directories.
 * 
 * Revision 9.92  91/01/03  07:54:43  hines
 * nparout.c  version comment moved to beginning of file
 * 
 * Revision 9.89  90/12/14  15:43:33  hines
 * nmodl: point process working with proper current dimensions. tested with
 * stim.mod and pascab1.hoc
 * 
 * Revision 9.83  90/12/12  09:01:51  hines
 * nmodl: nparout allocates the p-array
 * 
 * Revision 9.73  90/12/04  11:59:50  hines
 * model version displayed only as comment in generated c file
 * format of plot lines for scalar in .var file is
 * name nl vindex pindex 1 nl
 * for vector with specific index:
 * name[index] vindex pindex 1
 * for vector without index
 * name[size] vindex pindex size
 * 
 * Revision 9.72  90/11/30  13:09:35  hines
 * dcurdv calculated for ionic currents.
 * 
 * Revision 9.71  90/11/30  08:22:04  hines
 * modl_set_dt should only be created for time dependent models
 * 
 * Revision 9.70  90/11/28  15:34:53  hines
 * much work on case when ion is a state
 * 
 * Revision 9.61  90/11/23  10:01:14  hines
 * STEADYSTATE of kinetic and derivative blocks
 * 
 * Revision 9.53  90/11/13  13:09:08  hines
 * nmodl: cachan works pretty well. ions generating current works.
 * 
 * Revision 9.52  90/11/10  15:44:50  hines
 * nmodl: uses new NEURON { USEION ... format
 * passive.c works
 * 
 * Revision 9.46  90/10/30  14:27:25  hines
 * calc_dt is gone. Not needed because due to scopfit, dt needs to be
 * calculated at every break point.
 * _reset no longer used to say that dt has changed. The integrator will
 * have to check that itself.
 * 
 * Revision 9.45  90/10/30  13:55:45  hines
 * derivative blocks (this impacts kinetic and sens as well) now return
 * _reset which can be set with RESET statement.  _reset is static in the
 * file and set to 0 on entry to a derivative or kinetic block.
 * 
 * Revision 9.41  90/10/30  08:36:37  hines
 * saber warning free except for ytab.c and lex.c
 * 
 * Revision 9.40  90/10/30  08:06:00  hines
 * nmodl: Passive.mod working with index vectors. No longer copying
 * doubles. Just copying two pointers.
 * 
 * Revision 9.35  90/10/15  12:12:53  hines
 * mistake with checkin
 * 
 * Revision 9.33  90/10/11  15:44:34  hines
 * bugs fixed with respect to conversion from pointer vector to index vector.
 * 
 * Revision 9.32  90/10/08  14:12:15  hines
 * index vector instead of pointer vector for slist and dlist
 * 
 * Revision 9.31  90/10/08  11:33:45  hines
 * simsys prototype
 * 
 * Revision 9.18  90/08/07  15:35:44  hines
 * computation of rhs was bad
 * 
 * Revision 9.16  90/08/02  08:58:02  hines
 * NMODL can use more than one mechanism. Integrators that use ninits and
 * reset will not work, though.
 * 
 * Revision 9.14  90/07/31  17:03:03  hines
 * NMODL getting close. Compiles but multiple .mod files cause
 * multiple definitions.
 * 
 * Revision 9.13  90/07/30  14:36:40  hines
 * NMODL looks pretty good. Ready to start testing. Have not yet tried
 * to compile the .c file.
 * 
 * Revision 9.11  90/07/30  11:50:43  hines
 * NMODL getting better, almost done
 * 
 * Revision 9.5  90/07/18  07:58:00  hines
 * define for arrays now (p + n) instead of &p[n]. This allows the c file
 * to have arrays that look like a[i] instead of *(a + i).
 * 
 * Revision 8.9  90/04/03  07:44:29  mlh
 * for turbo-c defs.h stuff must appear after inclusion of scoplib.h
 * This is done by now saving defs.h stuff in defs_list and printing
 * it in cout.c.  Note that each item is type VERBATIM so that there
 * is no prepended space.
 * 
 * Revision 8.8  90/02/15  10:09:26  mlh
 * defs.h removed. Those defines now come from parout()
 * 
 * Revision 8.7  90/01/16  11:05:43  mlh
 * error checking and cleanup after error and call to abort_run()
 * 
 * Revision 8.6  89/11/21  07:36:19  mlh
 * _calc_dt used in scopcore and therefore must be declared even for
 * time independent models.
 * 
 * Revision 8.5  89/11/17  16:07:45  mlh
 * _ninits tells how many times initmodel() is called. Used by
 * scopmath routines that need to know when to self_initialize.
 * 
 * Revision 8.4  89/11/17  14:53:57  nfh
 * Changed modl_version string printed by SCoP to read: "Language version..."
 * 
 * Revision 8.3  89/11/14  16:37:20  mlh
 * _reset set to true whenever initmodel is called and whenever _calc_dt
 * is true
 * 
 * Revision 8.2  89/10/11  08:42:33  mlh
 * _reset apparently being declared elsewhere
 * 
 * Revision 8.1  89/10/11  08:34:19  mlh
 * generate modl_version string in .c file
 * declare _reset=1
 * 
 * Revision 8.0  89/09/22  17:26:00  nfh
 * Freezing
 * 
 * Revision 7.1  89/09/07  07:44:51  mlh
 * was failing to initialize time after match
 * many bugs in handling exact for loop of t to the break point
 * 
 * Revision 7.0  89/08/30  13:31:25  nfh
 * Rev 7 is now Experimental; Rev 6 is Testing
 * 
 * Revision 6.0  89/08/14  16:26:11  nfh
 * Rev 6.0 is latest of 4.x; now the Experimental version
 * 
 * Revision 4.0  89/07/24  17:02:39  nfh
 * Freezing rev 3.  Rev 4 is now Experimental
 * 
 * Revision 3.2  89/07/18  11:54:02  mlh
 * first_time removed and MODEL_LEVEL used for declaration precedence
 * 
 * Revision 1.2  89/07/18  11:21:54  mlh
 * eliminate first_time, etc.
 * 
 * Revision 1.1  89/07/06  14:47:30  mlh
 * Initial revision
 * 
*/

/* print the .c file from the lists */

#include "modl.h"
#include "parse1.h"
#include "symbol.h"

#define P(arg) fputs(arg, fcout)
List           *procfunc, *initfunc, *modelfunc, *termfunc, *initlist, *firstlist;
/* firstlist gets statements that must go before anything else */

#if NMODL
List		*nrnstate;
extern List	*currents, *set_ion_variables(), *get_ion_variables();
extern List	*begin_dion_stmt(), *end_dion_stmt();
#endif

extern Symbol  *indepsym;
extern List    *indeplist;
extern List	*match_bound;
extern List	*defs_list;
extern char	*saveindep;
extern char	*RCS_version;
extern char	*RCS_date;
char           *modelline;

#if VECTORIZE
extern int vectorize;
static List	*vectorize_replacements; /* pairs of item pointer, strings */
extern char* cray_pragma();
#endif

#if __TURBOC__ || SYSV || VMS
#define index strchr
#endif

static initstates();
static funcdec();

void c_out()
{
#if NMODL
	Item *q;
	extern int point_process;
#endif
	
#if VECTORIZE
	if (vectorize) {
		vectorize_do_substitute();
		c_out_vectorize();
		return;
	}
#endif
	Fprintf(fcout, "/* Created by Language version: %s  of %s */\n",
		RCS_version, RCS_date);
#if VECTORIZE
	P("/* NOT VECTORIZED */\n");
#endif
	Fflush(fcout);
	/* things which must go first and most declarations */
#if SIMSYS
	P("#include <stdio.h>\n#include <math.h>\n#include \"mathlib.h\"\n");
	P("#include \"common.h\"\n#include \"softbus.h\"\n");
	P("#include \"sbtypes.h\"\n#include \"Solver.h\"\n");
#else
	P("#include <stdio.h>\n#include <math.h>\n#include \"scoplib.h\"\n");
	P("#undef PI\n");
#endif
	printlist(defs_list);
	printlist(firstlist);
	RCS_version[strlen(RCS_version) - 2] = '\0';
	RCS_date[strlen(RCS_date) - 11] = '\0';
	RCS_version = index(RCS_version, ':') + 2;
	RCS_date = index(RCS_date, ':') + 2;
	P("static int _reset;\n");
#if NMODL
	P("static ");
#endif	
	if (modelline) {
		Fprintf(fcout, "char *modelname = \"%s\";\n\n", modelline);
	} else {
		Fprintf(fcout, "char *modelname = \"\";\n\n");
	}
	Fflush(fcout);		/* on certain internal errors partial output
				 * is helpful */
	P("static int error;\n");
#if NMODL
	P("static ");
#endif	
	P("int _ninits = 0;\n");
	P("static int _match_recurse=1;\n");
#if NMODL
	P("static ");
#endif	
	P("_modl_cleanup(){ _match_recurse=1;}\n");
	/*
	 * many machinations are required to make the infinite number of
	 * definitions involving _p in defs.h to be invisible to the user 
	 */
	/*
	 * This one allows scop variables in functions which do not have the
	 * p array as an argument 
	 */
#if SIMSYS || HMODL || NMODL
#else
	P("static double *_p;\n\n");
#endif
	funcdec();
	Fflush(fcout);

	/*
	 * translations of named blocks into functions, procedures, etc. Also
	 * some special declarations used by some blocks 
	 */
	printlist(procfunc);
	Fflush(fcout);

	/* Initialization function must always be present */
#if NMODL
	P("\nstatic initmodel() {\n  int _i; double _save;");
#endif
#if SIMSYS || HMODL
	P("\ninitmodel() {\n  int _i; double _save;");
#endif
#if (!(SIMSYS || HMODL || NMODL))
	P("\ninitmodel(_pp) double _pp[]; {\n int _i; double _save; _p = _pp;");
#endif
#if !NMODL
	P("_initlists();\n");
#endif
	P("_ninits++;\n");
	P(saveindep); /*see solve.c; blank if not a time dependent process*/
	P("{\n");
	initstates();
	printlist(initfunc);
	if (match_bound) {
		P("\n_init_match(_save);");
	}
	P("\n}\n}\n");
	Fflush(fcout);

#if NMODL
	/* generation of initmodel interface */
#if VECTORIZE
	P("\nstatic nrn_init(_pp, _ppd, _v) double *_pp, _v; Datum* _ppd; {\n");
	P(" _p = _pp; _ppvar = _ppd;\n");
#else
	P("\nstatic nrn_init(_prop, _v) Prop *_prop; double _v; {\n");
	P(" _p = _prop->param; _ppvar = _prop->dparam;\n");
#endif
	P(" v = _v;\n");
	printlist(get_ion_variables(1));
	P(" initmodel();\n");
	printlist(set_ion_variables(1));
	P("}\n");

	/* standard modl EQUATION without solve computes current */
	P("\nstatic double _nrn_current(_v) double _v;{double _current=0.;v=_v;{");
	if (currents->next != currents) {
		printlist(modelfunc);
	}
	ITERATE(q, currents) {
		Sprintf(buf, " _current += %s;\n", SYM(q)->name);
		P(buf);
	}
	P("\n} return _current;\n}\n");

	/* the neuron current also has to compute the dcurrent/dv as well
	   as make sure all currents accumulated properly (currents list) */
#if VECTORIZE
	P("\nstatic double nrn_cur(_pp, _ppd, _pdiag, _v) double *_pp, *_pdiag, _v; Datum* _ppd; {\n");
#else
	P("\nstatic double nrn_cur(_prop, _pdiag, _v) Prop *_prop; double *_pdiag, _v;{\n");
#endif
	if (currents->next != currents) {
	  P(" double _g, _rhs;\n");
#if VECTORIZE
	  P(" _p = _pp; _ppvar = _ppd;\n");
#else
	  P(" _p = _prop->param;  _ppvar = _prop->dparam;\n");
#endif
	  printlist(get_ion_variables(0));
	  P(" _g = _nrn_current(_v + .001);\n");
	  printlist(begin_dion_stmt());
	  P(" _rhs = _nrn_current(_v);\n");
	  printlist(end_dion_stmt(".001"));
	  P(" _g = (_g - _rhs)/.001;\n");
	  /* set the ion variable values */
	  printlist(set_ion_variables(0));
	  if (point_process) {
		P(" *_pdiag += _g * 1.e2/(_nd_area);\n return (_rhs - _g*_v) * 1.e2/(_nd_area);\n}\n");
	  }else{
		P(" *_pdiag += _g;\n return _rhs - _g*_v;\n}\n");
	  }
	}else{
	  P(" return 0.;\n}\n");
	}

	/* nrnstate list contains the EQUATION solve statement so this
	   advances states by dt */
#if VECTORIZE
	P("\nstatic nrn_state(_pp, _ppd, _v) double *_pp, _v; Datum* _ppd; {\n");
#else
	P("\nstatic nrn_state(_prop, _v) Prop *_prop; double _v; {\n");
#endif
	if (nrnstate || currents->next == currents) {
	  P(" double _break, _save;\n");
#if VECTORIZE
	  P(" _p = _pp; _ppvar = _ppd;\n");
#else
	  P(" _p = _prop->param;  _ppvar = _prop->dparam;\n");
#endif
	  P(" _break = t + .5*dt; _save = t; delta_t = dt;\n");
	  P(" v=_v;\n{\n");
	  printlist(get_ion_variables(1));
	  if (nrnstate) {
		  printlist(nrnstate);
	  }
	  if (currents->next == currents) {
	  	printlist(modelfunc);
	  }
	  printlist(set_ion_variables(1));
	  P("\n}");
	}
	P("\n}\n");
#else
	/* Model function must always be present */
#if SIMSYS
	P("\nmodel() {\n");
	P("double _break, _save;\n{\n");
#else
	P("\nmodel(_pp, _indepindex) double _pp[]; int _indepindex; {\n");
	P("double _break, _save;");
#if HMODL
	P("\n{\n");
#else
	P("_p = _pp;\n{\n");
#endif
#endif
	printlist(modelfunc);
	P("\n}\n}\n");
	Fflush(fcout);
#endif

#if NMODL
	P("\nstatic terminal(){}\n");
#else
	/* Terminal function must always be present */
#if SIMSYS || HMODL
	P("\nterminal() {");
	P("\n{\n");
#else
	P("\nterminal(_pp) double _pp[];{");
	P("_p = _pp;\n{\n");
#endif
	printlist(termfunc);
	P("\n}\n}\n");
	Fflush(fcout);
#endif

	/* initlists() is called once to setup slist and dlist pointers */
#if NMODL || SIMSYS || HMODL
	P("\nstatic _initlists() {\n");
#else
	P("\n_initlists() {\n");
#endif
	P(" int _i; static int _first = 1;\n");
	P("  if (!_first) return;\n");
	printlist(initlist);
	P("_first = 0;\n}\n");
}

/*
 * One of the things initmodel() must do is initialize all states to the
 * value of state0.  This generated code goes before any explicit initialize
 * code written by the user. 
 */
static void initstates()
{
	int             i;
	Item           *qs;
	Symbol         *s;

	SYMITER_STAT {
		Sprintf(buf, "%s0", s->name);
		if (lookup(buf)) {	/* if no constant associated
					 * with a state such as the
					 * ones automattically
					 * generated by SENS then
					 * there is no initialization
					 * line */
			if (s->subtype & ARRAY) {
				Fprintf(fcout,
					" for (_i=0; _i<%d; _i++) %s[_i] = %s0;\n",
					      s->araydim, s->name, s->name);
			} else {
				Fprintf(fcout, "  %s = %s0;\n",
					s->name, s->name);
			}
		}
	}
}

/*
 * here is the only place as of 18-apr-89 where we don't explicitly know the
 * type of a list element 
 */

void printlist(s)
	List           *s;
{
	Item           *q;
	int             newline = 0, indent = 0, i;

	/*
	 * most of this is merely to decide where newlines and indentation
	 * goes so that the .c file can be read if something goes wrong 
	 */
	if (!s) {
		return;
	}
	ITERATE(q, s) {
		if (q->itemtype == SYMBOL) {
			if (SYM(q)->type == SPECIAL) {
				switch (SYM(q)->subtype) {

				case SEMI:
					newline = 1;
					break;
				case BEGINBLK:
					newline = 1;
					indent++;
					break;
				case ENDBLK:
					newline = 1;
					indent--;
					break;
				}
			}
			Fprintf(fcout, " %s", SYM(q)->name);
		} else if (q->itemtype == VERBATIM) {
			Fprintf(fcout, "%s", STR(q));
		}else {
			Fprintf(fcout, " %s", STR(q));
		}
		if (newline) {
			newline = 0;
			Fprintf(fcout, "\n");
			for (i = 0; i < indent; i++) {
				Fprintf(fcout, "  ");
			}
		}
	}
}

static void funcdec()
{
	int             i;
	Symbol         *s;
	List           *qs;

	SYMITER(NAME) {
		/*EMPTY*/	/*maybe*/
		if (s->subtype & FUNCT) {
#define GLOBFUNCT 1
#if GLOBFUNCT && NMODL
#else
			Fprintf(fcout, "static double %s();\n", s->name);
#endif
		}
		if (s->subtype & PROCED) {
			Fprintf(fcout, "static %s();\n", s->name);
		}
	}
}

#if VECTORIZE
void c_out_vectorize()
{
	Item *q;
	extern int point_process;
	
	Fprintf(fcout, "/* Created by Language version: %s  of %s */\n",
		RCS_version, RCS_date);
	Fflush(fcout);
	/* things which must go first and most declarations */
	P("/* VECTORIZED */\n");
	P("#include <stdio.h>\n#include <math.h>\n#include \"scoplib.h\"\n");
	P("#undef PI\n");
	printlist(defs_list);
	printlist(firstlist);
	RCS_version[strlen(RCS_version) - 2] = '\0';
	RCS_date[strlen(RCS_date) - 11] = '\0';
	RCS_version = index(RCS_version, ':') + 2;
	RCS_date = index(RCS_date, ':') + 2;
	P("static int _reset;\n");
	if (modelline) {
		Fprintf(fcout, "static char *modelname = \"%s\";\n\n", modelline);
	} else {
		Fprintf(fcout, "static char *modelname = \"\";\n\n");
	}
	Fflush(fcout);		/* on certain internal errors partial output
				 * is helpful */
	P("static int error;\n");
	P("static int _ninits = 0;\n");
	P("static int _match_recurse=1;\n");
	P("static _modl_cleanup(){ _match_recurse=1;}\n");

	funcdec();
	Fflush(fcout);

	/*
	 * translations of named blocks into functions, procedures, etc. Also
	 * some special declarations used by some blocks 
	 */
	printlist(procfunc);
	Fflush(fcout);

	/* Initialization function must always be present */

	P("\nstatic initmodel(_ix) int _ix; {\n  int _i; double _save;");

#if 0
	P("_initlists(_ix);\n");
	P("_ninits++;\n");
	P(saveindep); /*see solve.c; blank if not a time dependent process*/
#endif
	P("{\n");
	initstates();
	printlist(initfunc);
	if (match_bound) {
		assert(!vectorize);
		P("\n_init_match(_save);");
	}
	P("\n}\n}\n");
	Fflush(fcout);

	/* generation of initmodel interface */
	P("\nstatic nrn_init(_count, _nodes, _data, _pdata)\n");
	P("	int _count; Node** _nodes; double** _data; Datum** _pdata;\n");
	P("{ int _ix;\n");
	P(" _p = _data; _ppvar = _pdata;\n");
	check_tables();
	P(cray_pragma());
	P("	for (_ix = 0; _ix < _count; ++_ix) {\n");
	P("		v = _nodes[_ix]->_v;\n");
	printlist(get_ion_variables(1));
	P("		initmodel(_ix);\n");
	printlist(set_ion_variables(1));
	P("	}\n");
	P("}\n");

	/* standard modl EQUATION without solve computes current */
	P("\nstatic double _nrn_current(_ix, _v) int _ix; double _v;{\n double _current=0.;v=_v;\n  {");
	if (currents->next != currents) {
		printlist(modelfunc);
	}
	ITERATE(q, currents) {
		Sprintf(buf, " _current += %s;\n", SYM(q)->name);
		P(buf);
	}
	P("\n} return _current;\n}\n");


	/* the neuron current also has to compute the dcurrent/dv as well
	   as make sure all currents accumulated properly (currents list) */
	P("\nstatic double nrn_cur(_count, _nodes, _data, _pdata)\n");
	P("	int _count; Node** _nodes; double** _data; Datum** _pdata;\n");
	P("{\n");
    if (currents->next != currents) {
	P("int _ix;\n");
	P(" _p = _data; _ppvar = _pdata;\n");
	check_tables();
	
	P("\n#if METHOD3\n   if (_method3) {\n");
	/*--- reprduction of normal with minor changes ---*/
	P(cray_pragma());
	P("	for (_ix = 0; _ix < _count; ++_ix) {\n");
	P("		double _g, _rhs, _v;\n");
	P("		_v = _nodes[_ix]->_v;\n");
	printlist(get_ion_variables(0));
	P("		_g = _nrn_current(_ix, _v + .001);\n");
	printlist(begin_dion_stmt());
	P("\n		_rhs = _nrn_current(_ix, _v);\n");
	printlist(end_dion_stmt(".001"));
	P("		_g = (_g - _rhs)/.001;\n");
	/* set the ion variable values */
	printlist(set_ion_variables(0));
	if (point_process) {
		P("		_nodes[_ix]->_d += _g * 1.e2/(_nd_area);\n");
		P("		_nodes[_ix]->_rhs += (_g*_v - _rhs) * 1.e2/(_nd_area);\n");
	}else{
		P("		_nodes[_ix]->_thisnode._GC += _g;\n");
		P("		_nodes[_ix]->_thisnode._EC += (_g*_v - _rhs);\n");
	}
	P("	}\n");
	/*-------------*/
	P("\n   }else\n#endif\n   {\n");

	/*--- normal ---*/
	P(cray_pragma());
	P("	for (_ix = 0; _ix < _count; ++_ix) {\n");
	P("		double _g, _rhs, _v;\n");
	P("		_v = _nodes[_ix]->_v;\n");
	printlist(get_ion_variables(0));
	P("		_g = _nrn_current(_ix, _v + .001);\n");
	printlist(begin_dion_stmt());
	P("\n		_rhs = _nrn_current(_ix, _v);\n");
	printlist(end_dion_stmt(".001"));
	P("		_g = (_g - _rhs)/.001;\n");
	/* set the ion variable values */
	printlist(set_ion_variables(0));
	if (point_process) {
		P("		_nodes[_ix]->_d += _g * 1.e2/(_nd_area);\n");
		P("		_nodes[_ix]->_rhs += (_g*_v - _rhs) * 1.e2/(_nd_area);\n");
	}else{
		P("		_nodes[_ix]->_d += _g;\n");
		P("		_nodes[_ix]->_rhs += (_g*_v - _rhs);\n");
	}
	P("	}\n");
	/*------------*/

	P("   }\n");

    }
	P("	return 0.;\n}\n");

	/* nrnstate list contains the EQUATION solve statement so this
	   advances states by dt */

	P("\nstatic nrn_state(_count, _nodes, _data, _pdata)\n");
	P("	int _count; Node** _nodes; double** _data; Datum** _pdata;\n");
	P("{\n");
    if (nrnstate || currents->next == currents) {
	P(" int _ix;\n");
	P(" double _break, _save;\n");
	P(" _p = _data; _ppvar = _pdata;\n");
	check_tables();
	P(" _break = t + .5*dt; _save = t; delta_t = dt;\n");

	P(cray_pragma());
	P("	for (_ix = 0; _ix < _count; ++_ix) {\n");
	P("		v = _nodes[_ix]->_v;\n");
	printlist(get_ion_variables(1));
	P("	}\n");

	P("\n{\n");
	if (nrnstate) {
		printlist(nrnstate);
	}
	P("}\n");
	P(cray_pragma());
	P("	for (_ix = 0; _ix < _count; ++_ix) {\n");
	if (currents->next == currents) {
		printlist(modelfunc);
	}
	printlist(set_ion_variables(1));
	P("	}\n");
    }
	P("}\n");
	Fflush(fcout);

	P("\nstatic terminal(){}\n");

	/* vectorized: data must have own copies of slist and dlist
	   for now we don't vectorize if slist or dlist exists. Eventually
	   must separate initialization of static things from vectorized
	   things.
	*/
	/* initlists() is called once to setup slist and dlist pointers */
	P("\nstatic _initlists(){\n");
	P(" int _i; static int _first = 1;\n");
	P("  if (!_first) return;\n");
	printlist(initlist);
	P("_first = 0;\n}\n");
}

void vectorize_substitute(q, str)
	Item* q;
	char* str;
{
	if (!vectorize_replacements) {
		vectorize_replacements = newlist();
	}
	lappenditem(vectorize_replacements, q);
	lappendstr(vectorize_replacements, str);
}

void vectorize_do_substitute() {
	Item *q, *q1;
	if (vectorize_replacements) {
		ITERATE(q, vectorize_replacements) {
			q1 = ITM(q);
			q = q->next;
			replacstr(q1, STR(q));
		}
	}
}

char* cray_pragma() {
	static char buf[] = "\
\n#if _CRAY\
\n#pragma _CRI ivdep\
\n#endif\
\n";
	return buf;
}

#endif /*VECTORIZE*/
