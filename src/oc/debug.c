#include <../../nrnconf.h>
/* /local/src/master/nrn/src/oc/debug.c,v 1.7 1996/04/09 16:39:14 hines Exp */
#include "hoc.h"
#include "code.h"
#include "equation.h"
#include <stdio.h>
int zzdebug;

#if	DOS
#define prcod(c1,c2)	else if (p->pf == c1) Printf("%p %p %s", p, p->pf, c2);
#else
#define prcod(c1,c2)	else if (p->pf == c1) Printf("%p %p %s", p, p->pf, c2);
#endif

void debug(void)		/* print the machine */
{
	if (zzdebug == 0)
		zzdebug = 1;
	else
		zzdebug = 0;
}

void debugzz(Inst* p)	/* running copy of calls to execute */
{
#if !OCSMALL
	{
		if(p->in == STOP) Printf("STOP\n");
		prcod(nopop, "POP\n")
		prcod(eval, "EVAL\n")
		prcod(add, "ADD\n")
		prcod(hoc_sub, "SUB\n")
		prcod(mul, "MUL\n")
		prcod(hoc_div, "DIV\n")
		prcod(negate, "NEGATE\n")
		prcod(power, "POWER\n")
		prcod(assign, "ASSIGN\n")
		prcod(bltin, "BLTIN\n")
		prcod(varpush, "VARPUSH\n")
		prcod(constpush, "CONSTPUSH\n")
		prcod(pushzero, "PUSHZERO\n")
		prcod(print, "PRINT\n")
		prcod(varread, "VARREAD\n")
		prcod(prexpr, "PREXPR\n")
		prcod(prstr,"PRSTR\n")
		prcod(gt, "GT\n")
		prcod(lt, "LT\n")
		prcod(eq, "EQ\n")
		prcod(ge, "GE\n")
		prcod(le, "LE\n")
		prcod(ne, "NE\n")
		prcod(hoc_and, "AND\n")
		prcod(hoc_or, "OR\n")
		prcod(hoc_not, "NOT\n")
		prcod(ifcode, "IFCODE\n")
		prcod(forcode, "FORCODE\n")
		prcod(shortfor, "SHORTFOR\n")
		prcod(call, "CALL\n")
		prcod(arg, "ARG\n")
		prcod(argassign, "ARGASSIGN\n")
		prcod(funcret, "FUNCRET\n")
		prcod(procret, "PROCRET\n")
		prcod(hocobjret, "HOCOBJRET\n")
#if DOS
/* no room for all this stuff */
#else
		prcod(hoc_iterator_stmt, "hoc_iterator_stmt\n")
		prcod(hoc_iterator, "hoc_iterator\n")
		prcod(hoc_argrefasgn, "ARGREFASSIGN\n")
		prcod(hoc_argref, "ARGREF\n")
		prcod(hoc_stringarg, "STRINGARG\n")
		prcod(hoc_push_string, "push_string\n")
		prcod(Break, "Break\n")
		prcod(Continue, "Continue\n")
		prcod(Stop, "Stop()\n")
		prcod(assstr, "assstr\n")
		prcod(hoc_evalpointer, "evalpointer\n")
		prcod(hoc_newline, "newline\n")
		prcod(hoc_delete_symbol, "delete_symbol\n")
		prcod(hoc_cyclic, "cyclic\n")
		prcod(hoc_parallel_begin, "parallel_begin\n")
		prcod(hoc_parallel_end, "parallel_end\n")

		prcod(dep_make, "DEPENDENT\n")
		prcod(eqn_name, "EQUATION\n")
		prcod(eqn_init, "eqn_init()\n")
		prcod(eqn_lhs, "eqn_lhs()\n")
		prcod(eqn_rhs, "eqn_rhs()\n")
/*OOP*/
		prcod(hoc_push_current_object, "hoc_push_current_object\n")
		prcod(hoc_objectvar, "objectvar\n")
		prcod(hoc_object_component, "objectcomponent()\n")
		prcod(hoc_object_eval, "objecteval\n")
		prcod(hoc_object_asgn, "objectasgn\n")
		prcod(hoc_objvardecl, "objvardecl\n")
		prcod(hoc_cmp_otype, "cmp_otype\n")
		prcod(hoc_newobj, "newobject\n")
		prcod(hoc_asgn_obj_to_str, "assignobj2str\n")
		prcod(hoc_known_type, "known_type\n")
		prcod(hoc_push_string, "push_string\n")
		prcod(hoc_objectarg, "hoc_objectarg\n")
		prcod(hoc_ob_pointer, "hoc_ob_pointer\n")
		prcod(hoc_constobject, "hoc_constobject\n")

/*NEWCABLE*/
		prcod(connect_obsec_syntax, "connect_obsec_syntax()\n")
		prcod(connectsection, "connectsection()\n")
		prcod(simpleconnectsection, "simpleconnectsection()\n")
		prcod(connectpointer, "connectpointer()\n")
		prcod(add_section, "add_section()\n")
		prcod(range_const, "range_const()\n")
		prcod(range_interpolate, "range_interpolate()\n")
		prcod(range_interpolate_single, "range_interpolate_single()\n")
		prcod(rangevareval, "rangevareval()\n")
		prcod(rangepoint, "rangepoint()\n")
		prcod(sec_access, "sec_access()\n")
		prcod(ob_sec_access, "ob_sec_access()\n")
		prcod(mech_access, "mech_access()\n")
		prcod(for_segment, "forsegment()\n")
		prcod(sec_access_push, "sec_access_push()\n")
		prcod(sec_access_pop, "sec_access_pop()\n")
		prcod(forall_section, "forall_section()\n")
		prcod(hoc_ifsec, "hoc_ifsec()\n")
		prcod(hoc_ifseclist, "hocifseclist()\n")
		prcod(forall_sectionlist, "forall_sectionlist()\n")
		prcod(connect_point_process_pointer, "connect_point_process_pointer\n")
		prcod(nrn_cppp, "nrn_cppp()\n")
		prcod(rangevarevalpointer, "rangevarevalpointer\n")
		prcod(sec_access_object, "sec_access_object\n")
		prcod(mech_uninsert, "mech_uninsert\n")
#endif
		else
		{
			size_t offset = (size_t)p->in;
			if (offset < 1000)
				Printf("relative %d\n", p->i);
			else {
			offset = (size_t)(p->in) - (size_t)p;
			if (offset > (size_t)prog - (size_t)p
			&& offset < (size_t)(&prog[2000]) - (size_t)p )
				Printf("relative %ld\n", p->in - p);
			else if (p->sym->name != (char *) 0)
				{
				if (p->sym->name[0] == '\0') {
					Printf("constant or string pointer\n");
/*Printf("value=%g\n", p->sym->u.val);*/
				}
				else
					Printf("%s\n", p->sym->name);
				}
			else
				Printf("symbol without name\n");
			}
		}
		p++;
	}
#endif /*OCSMALL*/
}
