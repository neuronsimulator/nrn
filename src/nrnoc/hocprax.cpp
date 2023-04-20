#include <../../nrnconf.h>
/*
Hoc interface to praxis.

See praxis.cpp in the scopmath library about
tolerance (t0), maxstepsize (h0), and printmode (prin).
These are set with
    attr_praxis(t0, h0, prin)

Minimise an interpreted hoc function or compiled hoc function with
    double x[n]
    fit_praxis(n, "funname", &x[0], "afterquad statement")
where n is the number of variables on which funname depends and
x is a vector containing on entry a guess of the point of minimum
and on exit contains the estimated point of minimum.
(The third arg may be a Vector)
Funname will be called with 2 args to be retrieved by $1 and $&2[i].
afterquad statement (if the arg exists , will be executed
at the end of each main loop iteration (complete conjugate gradient search
followed by calculation of quadratic form)
eg.

double x[2]
attr_praxis(1e-5, 1, -1)
func f() {
    return $&0[0]^2 + 2*$&0[1]^4
}
fit_praxis(2, "f", &x)

After fit_praxis exits, (or on execution of afterquad stmt)
one can retrieve the i'th principal value
with pval_praxis(i). If you also want the i'th principal axis then use
double a[n]
pval = pval_praxis(i, &a)
also can use
pval = pval_praxis(i, Vector)
*/

#include <stdlib.h>
#include "hocdec.h"
#include "nrnpy.h"
#include "parse.hpp"
#include "scoplib.h"

extern double chkarg(int, double, double);
extern IvocVect* vector_new2(IvocVect* vec);
extern void vector_delete(IvocVect* vec);
extern int nrn_praxis_ran_index;
extern Object** hoc_objgetarg(int);

static double efun(double*, long int);
static Symbol* hoc_efun_sym;

static double tolerance, machep, maxstepsize;
static long int printmode;
/* for prax_pval to know the proper size, following has to be the value
at return of previous prax call
*/
static long int nvar;

static Object* efun_py;
static Object* efun_py_arg;
static void* vec_py_save;

void stop_praxis(void) {
    int i = 1;
    if (ifarg(1)) {
        i = (int) chkarg(1, 0., 1e5);
    }
    i = praxis_stop(i);
    hoc_retpushx((double) i);
}

/* want to get best result if user does a stoprun */
static double minerr, *minarg; /* too bad this is not recursive */

void fit_praxis(void) {
    char* after_quad;
    int i;
    double err, fmin;
    double* px;
    /* allow nested calls to fit_praxis. I.e. store all the needed
       statics specific to this invocation with proper reference
       counting and then unref/destoy on exit from this invocation.
       Before the prax call save the statics from earlier invocation
       without increasing the
       ref count and on exit restore without decreasing the ref count.
    */

    /* save before setting statics, restore after prax */
    double minerrsav, *minargsav, maxstepsizesav, tolerancesav;
    long int printmodesav, nvarsav;
    Symbol* efun_sym_sav;
    Object *efun_py_save, *efun_py_arg_save;
    void* vec_py_save_save;

    /* store statics specified by this invocation */
    /* will be copied just before calling prax */
    double minerr_, *minarg_;
    long int nvar_;
    Symbol* efun_sym_;
    Object *efun_py_, *efun_py_arg_;
    void* vec_py_save_;

    minerr_ = 0.0;
    nvar_ = 0;
    minarg_ = NULL;
    efun_sym_ = NULL;
    efun_py_ = NULL;
    efun_py_arg_ = NULL;
    vec_py_save_ = NULL;

    fmin = 0.;

    if (hoc_is_object_arg(1)) {
        assert(neuron::python::methods.praxis_efun);
        efun_py_ = *hoc_objgetarg(1);
        hoc_obj_ref(efun_py_);
        efun_py_arg_ = *vector_pobj(vector_arg(2));
        hoc_obj_ref(efun_py_arg_);
        vec_py_save_ = vector_new2(static_cast<IvocVect*>(efun_py_arg_->u.this_pointer));
        nvar_ = vector_capacity(static_cast<IvocVect*>(vec_py_save_));
        px = vector_vec(static_cast<IvocVect*>(vec_py_save_));
    } else {
        nvar_ = (int) chkarg(1, 0., 1e6);
        efun_sym_ = hoc_lookup(gargstr(2));
        if (!efun_sym_ || (efun_sym_->type != FUNCTION && efun_sym_->type != FUN_BLTIN)) {
            hoc_execerror(gargstr(2), "not a function name");
        }

        if (!hoc_is_pdouble_arg(3)) {
            IvocVect* vec = vector_arg(3);
            if (vector_capacity(vec) != nvar_) {
                hoc_execerror("first arg not equal to size of Vector", 0);
            }
            px = vector_vec(vec);
        } else {
            px = hoc_pgetarg(3);
        }
    }
    minarg_ = (double*) ecalloc(nvar_, sizeof(double));

    if (maxstepsize == 0.) {
        hoc_execerror("call attr_praxis first to set attributes", 0);
    }
    machep = 1e-15;

    if (ifarg(4)) {
        after_quad = gargstr(4);
    } else {
        after_quad = (char*) 0;
    }

    /* save the values set by earlier invocation */
    minerrsav = minerr;
    minargsav = minarg;
    tolerancesav = tolerance;
    maxstepsizesav = maxstepsize;
    printmodesav = printmode;
    nvarsav = nvar;
    efun_sym_sav = hoc_efun_sym;
    efun_py_save = efun_py;
    efun_py_arg_save = efun_py_arg;
    vec_py_save_save = vec_py_save;


    /* copy this invocation values to the statics */
    minerr = minerr_;
    minarg = minarg_;
    nvar = nvar_;
    hoc_efun_sym = efun_sym_;
    efun_py = efun_py_;
    efun_py_arg = efun_py_arg_;
    vec_py_save = vec_py_save_;


    minerr = 1e9;
    err = praxis(&tolerance, &machep, &maxstepsize, nvar, &printmode, px, efun, &fmin, after_quad);
    err = minerr;
    if (minerr < 1e9) {
        for (i = 0; i < nvar; ++i) {
            px[i] = minarg[i];
        }
    }

    /* restore the values set by earlier invocation */
    minerr = minerrsav;
    minarg = minargsav;
    tolerance = tolerancesav;
    maxstepsize = maxstepsizesav;
    printmode = printmodesav;
    nvar = nvar_; /* in case one calls prax_pval */
    hoc_efun_sym = efun_sym_sav;
    efun_py = efun_py_save;
    efun_py_arg = efun_py_arg_save;
    vec_py_save = vec_py_save_save;

    if (efun_py_) {
        double* px = vector_vec(static_cast<IvocVect*>(efun_py_arg_->u.this_pointer));
        for (i = 0; i < nvar_; ++i) {
            px[i] = minarg_[i];
        }
        hoc_obj_unref(efun_py_);
        hoc_obj_unref(efun_py_arg_);
        vector_delete(static_cast<IvocVect*>(vec_py_save_));
    }
    if (minarg_) {
        free(minarg_);
    }
    hoc_retpushx(err);
}

void hoc_after_prax_quad(char* s) {
    efun(minarg, nvar);
    hoc_obj_run(s, hoc_thisobject);
}

void attr_praxis(void) {
    if (ifarg(2)) {
        tolerance = *getarg(1);
        maxstepsize = *getarg(2);
        printmode = (int) chkarg(3, 0., 3.);
        hoc_retpushx(0.);
    } else {
        int old = nrn_praxis_ran_index;
        if (ifarg(1)) {
            nrn_praxis_ran_index = (int) chkarg(1, 0., 1e9);
        }
        hoc_retpushx((double) old);
    }
}

void pval_praxis(void) {
    int i;
    i = (int) chkarg(1, 0., (double) (nvar - 1));
    if (ifarg(2)) {
        int j;
        double *p1, *p2;
        p1 = praxis_paxis(i);
        if (!hoc_is_pdouble_arg(2)) {
            IvocVect* vec = vector_arg(2);
            vector_resize(vec, nvar);
            p2 = vector_vec(vec);
        } else {
            p2 = hoc_pgetarg(2);
        }
        for (j = 0; j < nvar; ++j) {
            p2[j] = p1[j];
        }
    }
    hoc_retpushx(praxis_pval(i));
}

static double efun(double* v, long int n) {
    int i;
    double err;
    if (efun_py) {
        double* px = vector_vec(static_cast<IvocVect*>(efun_py_arg->u.this_pointer));
        for (i = 0; i < n; ++i) {
            px[i] = v[i];
        }
        err = neuron::python::methods.praxis_efun(efun_py, efun_py_arg);
        for (i = 0; i < n; ++i) {
            v[i] = px[i];
        }
    } else {
        int i;
        hoc_pushx((double) n);
        hoc_pushpx(v);
        err = hoc_call_func(hoc_efun_sym, 2);
    }
    if (!stoprun && err < minerr) {
        minerr = err;
        for (i = 0; i < n; ++i) {
            minarg[i] = v[i];
        }
    }
    return err;
}
