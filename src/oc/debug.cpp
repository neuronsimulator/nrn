#include <../../nrnconf.h>
/* /local/src/master/nrn/src/oc/debug.cpp,v 1.7 1996/04/09 16:39:14 hines Exp */
#include "hocdec.h"
#include "code.h"
#include "equation.h"
#include <stdio.h>

#include "utils/logger.hpp"

int hoc_zzdebug;

#define prcod(c1, c2) else if (p->pf == c1) Printf("%p %p %s", fmt::ptr(p), fmt::ptr(p->pf), c2)

void hoc_debug(void) /* print the machine */
{
    if (hoc_zzdebug == 0)
        hoc_zzdebug = 1;
    else
        hoc_zzdebug = 0;
}

/* running copy of calls to execute */
void debugzz(Inst* p) {
#if !OCSMALL
    {
        if (p->in == STOP)
            Printf("STOP\n");
        prcod(hoc_nopop, "POP\n");
        prcod(hoc_eval, "EVAL\n");
        prcod(hoc_add, "ADD\n");
        prcod(hoc_sub, "SUB\n");
        prcod(hoc_mul, "MUL\n");
        prcod(hoc_div, "DIV\n");
        prcod(hoc_negate, "NEGATE\n");
        prcod(hoc_power, "POWER\n");
        prcod(hoc_assign, "ASSIGN\n");
        prcod(hoc_bltin, "BLTIN\n");
        prcod(hoc_varpush, "VARPUSH\n");
        prcod(hoc_constpush, "CONSTPUSH\n");
        prcod(hoc_pushzero, "PUSHZERO\n");
        prcod(hoc_print, "PRINT\n");
        prcod(hoc_varread, "VARREAD\n");
        prcod(hoc_prexpr, "PREXPR\n");
        prcod(hoc_prstr, "PRSTR\n");
        prcod(hoc_gt, "GT\n");
        prcod(hoc_lt, "LT\n");
        prcod(hoc_eq, "EQ\n");
        prcod(hoc_ge, "GE\n");
        prcod(hoc_le, "LE\n");
        prcod(hoc_ne, "NE\n");
        prcod(hoc_and, "AND\n");
        prcod(hoc_or, "OR\n");
        prcod(hoc_not, "NOT\n");
        prcod(hoc_ifcode, "IFCODE\n");
        prcod(hoc_forcode, "FORCODE\n");
        prcod(hoc_shortfor, "SHORTFOR\n");
        prcod(hoc_call, "CALL\n");
        prcod(hoc_arg, "ARG\n");
        prcod(hoc_argassign, "ARGASSIGN\n");
        prcod(hoc_funcret, "FUNCRET\n");
        prcod(hoc_procret, "PROCRET\n");
        prcod(hocobjret, "HOCOBJRET\n");
        prcod(hoc_iterator_stmt, "hoc_iterator_stmt\n");
        prcod(hoc_iterator, "hoc_iterator\n");
        prcod(hoc_argrefasgn, "ARGREFASSIGN\n");
        prcod(hoc_argref, "ARGREF\n");
        prcod(hoc_stringarg, "STRINGARG\n");
        prcod(hoc_Break, "Break\n");
        prcod(hoc_Continue, "Continue\n");
        prcod(hoc_Stop, "Stop()\n");
        prcod(hoc_assstr, "assstr\n");
        prcod(hoc_evalpointer, "evalpointer\n");
        prcod(hoc_newline, "newline\n");
        prcod(hoc_delete_symbol, "delete_symbol\n");
        prcod(hoc_cyclic, "cyclic\n");

        prcod(hoc_dep_make, "DEPENDENT\n");
        prcod(hoc_eqn_name, "EQUATION\n");
        prcod(hoc_eqn_init, "eqn_init()\n");
        prcod(hoc_eqn_lhs, "eqn_lhs()\n");
        prcod(hoc_eqn_rhs, "eqn_rhs()\n");
        /*OOP*/
        prcod(hoc_push_current_object, "hoc_push_current_object\n");
        prcod(hoc_objectvar, "objectvar\n");
        prcod(hoc_object_component, "objectcomponent()\n");
        prcod(hoc_object_eval, "objecteval\n");
        prcod(hoc_object_asgn, "objectasgn\n");
        prcod(hoc_objvardecl, "objvardecl\n");
        prcod(hoc_cmp_otype, "cmp_otype\n");
        prcod(hoc_newobj, "newobject\n");
        prcod(hoc_asgn_obj_to_str, "assignobj2str\n");
        prcod(hoc_known_type, "known_type\n");
        prcod(hoc_push_string, "push_string\n");
        prcod(hoc_objectarg, "hoc_objectarg\n");
        prcod(hoc_ob_pointer, "hoc_ob_pointer\n");
        prcod(hoc_constobject, "hoc_constobject\n");

        /*NEWCABLE*/
        prcod(connect_obsec_syntax, "connect_obsec_syntax()\n");
        prcod(connectsection, "connectsection()\n");
        prcod(simpleconnectsection, "simpleconnectsection()\n");
        prcod(connectpointer, "connectpointer()\n");
        prcod(add_section, "add_section()\n");
        prcod(range_const, "range_const()\n");
        prcod(range_interpolate, "range_interpolate()\n");
        prcod(range_interpolate_single, "range_interpolate_single()\n");
        prcod(rangevareval, "rangevareval()\n");
        prcod(rangepoint, "rangepoint()\n");
        prcod(sec_access, "sec_access()\n");
        prcod(ob_sec_access, "ob_sec_access()\n");
        prcod(mech_access, "mech_access()\n");
        prcod(for_segment, "forsegment()\n");
        prcod(sec_access_push, "sec_access_push()\n");
        prcod(sec_access_pop, "sec_access_pop()\n");
        prcod(forall_section, "forall_section()\n");
        prcod(hoc_ifsec, "hoc_ifsec()\n");
        prcod(hoc_ifseclist, "hocifseclist()\n");
        prcod(forall_sectionlist, "forall_sectionlist()\n");
        prcod(connect_point_process_pointer, "connect_point_process_pointer\n");
        prcod(nrn_cppp, "nrn_cppp()\n");
        prcod(rangevarevalpointer, "rangevarevalpointer\n");
        prcod(sec_access_object, "sec_access_object\n");
        prcod(mech_uninsert, "mech_uninsert\n");
        else {
            size_t offset = (size_t) p->in;
            if (offset < 1000)
                Printf("relative %d\n", p->i);
            else {
                offset = (size_t) (p->in) - (size_t) p;
                if (offset > (size_t) hoc_prog - (size_t) p &&
                    offset < (size_t) (&hoc_prog[2000]) - (size_t) p)
                    Printf("relative %ld\n", p->in - p);
                else if (p->sym->name != (char*) 0) {
                    if (p->sym->name[0] == '\0') {
                        Printf("constant or string pointer\n");
                        /*Printf("value=%g\n", p->sym->u.val);*/
                    } else
                        Printf("%s\n", p->sym->name);
                } else
                    Printf("symbol without name\n");
            }
        }
        p++;
    }
#endif /*OCSMALL*/
}
