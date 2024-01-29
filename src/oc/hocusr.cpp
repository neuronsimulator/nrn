#include <../../nrnconf.h>

/* version 7.2.1 2-jan-89 */
#include <stdio.h>
#include <stdlib.h>
#include "hocdec.h"
#include "parse.hpp"
#if 1
#include "hocusr.h"
#endif

#define CHECK(name) nrn_load_name_check(name)


static char CHKmes[] = "The user defined name, %s, already exists.\n";

Symlist* nrn_load_dll_called_;

/* Return 0 if nrn_load_dll_called_ == NULL, otherwise recover and return 1.
   If 1 is returned, then can recover with a hoc_execerror.
*/
int nrn_load_dll_recover_error() {
    if (nrn_load_dll_called_) {
        /* recoverable error for nrn_load_dll interpreter call */
        hoc_built_in_symlist = hoc_symlist;
        hoc_symlist = nrn_load_dll_called_;
        nrn_load_dll_called_ = (Symlist*) 0;
        return 1;
    }
    return 0;
}

void nrn_load_name_check(const char* name) {
    if (hoc_lookup(name) != (Symbol*) 0) {
        if (nrn_load_dll_recover_error()) {
            hoc_execerror("The user defined name already exists:", name);
        } else {
            fprintf(stderr, CHKmes, name);
            nrn_exit(1);
        }
    }
}


static void arayinstal(Symbol* sp, int nsub, int sub1, int sub2, int sub3);

/* install user variables and functions */
void hoc_spinit() {
    int i;
    Symbol* s;

    hoc_register_var(scdoub, vdoub, functions);
    for (i = 0; scint[i].name; i++) {
        CHECK(scint[i].name);
        s = hoc_install(scint[i].name, UNDEF, 0.0, &hoc_symlist);
        s->type = VAR;
        s->u.pvalint = scint[i].pint;
        s->subtype = USERINT;
    }
    for (i = 0; scfloat[i].name; i++) {
        CHECK(scfloat[i].name);
        s = hoc_install(scfloat[i].name, UNDEF, 0.0, &hoc_symlist);
        s->type = VAR;
        s->u.pvalfloat = scfloat[i].pfloat;
        s->subtype = USERFLOAT;
    }
    for (i = 0; vint[i].name; i++) {
        CHECK(vint[i].name);
        s = hoc_install(vint[i].name, UNDEF, 0.0, &hoc_symlist);
        s->type = VAR;
        arayinstal(s, 1, vint[i].index1, 0, 0);
        s->u.pvalint = vint[i].pint;
        s->subtype = USERINT;
    }
    for (i = 0; vfloat[i].name; i++) {
        CHECK(vfloat[i].name);
        s = hoc_install(vfloat[i].name, UNDEF, 0.0, &hoc_symlist);
        s->type = VAR;
        arayinstal(s, 1, vfloat[i].index1, 0, 0);
        s->u.pvalfloat = vfloat[i].pfloat;
        s->subtype = USERFLOAT;
    }
    for (i = 0; ardoub[i].name; i++) {
        CHECK(ardoub[i].name);
        s = hoc_install(ardoub[i].name, UNDEF, 0.0, &hoc_symlist);
        s->type = VAR;
        arayinstal(s, 2, ardoub[i].index1, ardoub[i].index2, 0);
        s->u.pval = ardoub[i].pdoub;
        s->subtype = USERDOUBLE;
    }
    for (i = 0; thredim[i].name; i++) {
        CHECK(thredim[i].name);
        s = hoc_install(thredim[i].name, UNDEF, 0.0, &hoc_symlist);
        s->type = VAR;
        arayinstal(s, 3, thredim[i].index1, thredim[i].index2, thredim[i].index3);
        s->u.pval = thredim[i].pdoub;
        s->subtype = USERDOUBLE;
    }
    for (i = 0; functions[i].name; i++) {
        if (!strncmp(functions[i].name, "init", 4)) {
            hoc_fake_call(hoc_lookup(functions[i].name));
            (*functions[i].func)();
            continue;
        }
    }
    hoc_last_init();
}

void hoc_register_var(DoubScal* scdoub, DoubVec* vdoub, VoidFunc* fn) {
    int i;
    Symbol* s;

    if (scdoub)
        for (i = 0; scdoub[i].name; i++) {
            CHECK(scdoub[i].name);
            s = hoc_install(scdoub[i].name, UNDEF, 0.0, &hoc_symlist);
            s->type = VAR;
            s->u.pval = scdoub[i].pdoub;
            s->subtype = USERDOUBLE;
        }
    if (vdoub)
        for (i = 0; vdoub[i].name; i++) {
            CHECK(vdoub[i].name);
            s = hoc_install(vdoub[i].name, UNDEF, 0.0, &hoc_symlist);
            s->type = VAR;
            arayinstal(s, 1, vdoub[i].index1, 0, 0);
            s->u.pval = vdoub[i].pdoub;
            s->subtype = USERDOUBLE;
        }
    if (fn)
        for (i = 0; fn[i].name; i++) {
            CHECK(fn[i].name);
            s = hoc_install(fn[i].name, FUN_BLTIN, 0.0, &hoc_symlist);
            s->u.u_proc->defn.pf = fn[i].func;
            s->u.u_proc->nauto = 0;
            s->u.u_proc->nobjauto = 0;
        }
}

/* set up arayinfo */
static void arayinstal(Symbol* sp, int nsub, int sub1, int sub2, int sub3) {
    sp->type = VAR;
    sp->s_varn = 0;
    sp->arayinfo = (Arrayinfo*) emalloc((unsigned) (sizeof(Arrayinfo) + nsub * sizeof(int)));
    sp->arayinfo->a_varn = (unsigned*) 0;
    sp->arayinfo->nsub = nsub;
    sp->arayinfo->sub[0] = sub1;
    if (nsub > 1)
        sp->arayinfo->sub[1] = sub2;
    if (nsub > 2)
        sp->arayinfo->sub[2] = sub3;
}

void hoc_retpushx(double x) { /* utility return for user functions */
    hoc_ret();
    hoc_pushx(x);
}
