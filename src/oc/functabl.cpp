#include <../../nrnconf.h>
#include "hoc.h"

struct TableArg {
    int nsize;
    double* argvec; /* if nullptr use min,max */
    double min;
    double max;
    double frac; /* temporary storage */
};

struct FuncTable {
    double* table;
    TableArg* targs;
    double value; /* if constant this is it */
};

static int arg_index(TableArg* ta, double x) {
    int j;

    if (ta->argvec) {
        ta->frac = 0.;
        int t0 = 0;
        int t1 = ta->nsize - 1;
        if (x <= ta->argvec[t0]) {
            j = 0;
        } else if (x >= ta->argvec[t1]) {
            j = t1;
        } else {
            while (t0 < (t1 - 1)) {
                j = (t0 + t1) / 2;
                if (ta->argvec[j] <= x) {
                    t0 = j;
                } else {
                    t1 = j;
                }
            }
            j = t0;
            ta->frac = (x - ta->argvec[j]) / (ta->argvec[j + 1] - ta->argvec[j]);
        }
    } else {
        ta->frac = 0.;
        if (x <= ta->min) {
            j = 0;
        } else if (x >= ta->max) {
            j = ta->nsize - 1;
        } else {
            double d = (ta->max - ta->min) / ((double) (ta->nsize - 1));
            double x1 = (x - ta->min) / d;
            j = (int) x1;
            ta->frac = x1 - (double) j;
        }
    }
    return j;
}

static double inter(double frac, double* tab, int j) {
    if (frac > 0.) {
        return (1. - frac) * tab[j] + frac * tab[j + 1];
    } else {
        return tab[j];
    }
}

static double interp(double frac, double x1, double x2) {
    if (frac > 0.) {
        return (1. - frac) * x1 + frac * x2;
    } else {
        return x1;
    }
}

double hoc_func_table(void* vpft, int n, double* args) {
    auto* const ft = static_cast<FuncTable*>(vpft);
    if (!ft) {
        hoc_execerror("table not specified in hoc_func_table", nullptr);
    }
    if (ft->value && ft->targs[0].nsize == 1) {
        return ft->value;
    }
    double* tab = ft->table;
    int j = 0;
    // matrix order is row order.
    // I.e. [irow][jcol] is at location irow*ncol + jcol
    for (size_t i = 0; i < n; ++i) {
        j = (j * ft->targs[i].nsize) + arg_index(ft->targs + i, args[i]);
    }
    if (n == 1) {
        return inter(ft->targs[0].frac, tab, j);
    } else if (n == 2) {
        double fy = ft->targs[1].frac;
        double y1 = inter(fy, tab, j);
        if (ft->targs[0].frac) { /* x dimension fraction */
            int j1 = j + ft->targs[1].nsize;
            double y2 = inter(fy, tab, j1);
            return interp(ft->targs[0].frac, y1, y2);
        } else {
            return y1;
        }
    } else {
        return tab[j];
    }
}

void hoc_spec_table(void** vppt, int n) {
    auto ppt = reinterpret_cast<FuncTable**>(vppt);
    if (!*ppt) {
        *ppt = (FuncTable*) ecalloc(1, sizeof(FuncTable));
        (*ppt)->targs = (TableArg*) ecalloc(n, sizeof(TableArg));
    }
    FuncTable* ft = *ppt;
    TableArg* ta = ft->targs;
    if (!ifarg(2)) { /* set to constant */
        ft->value = *getarg(1);
        ft->table = &ft->value;
        for (size_t i = 0; i < n; ++i) {
            ta[i].nsize = 1;
            ta[i].argvec = nullptr;
            ta[i].min = 1e20;
            ta[i].max = 1e20;
        }
        return;
    }
    if (hoc_is_object_arg(1) && hoc_is_object_arg(2)) {
        if (n > 1) {
            hoc_execerror("Vector arguments allowed only for functions", "of one variable");
        }
        Object* ob1 = *hoc_objgetarg(1);
        hoc_obj_ref(ob1);
        Object* ob2 = *hoc_objgetarg(2);
        hoc_obj_ref(ob2);
        int ns = vector_arg_px(1, &ft->table);
        ta[0].nsize = vector_arg_px(2, &ta[0].argvec);
        if (ns != ta[0].nsize) {
            hoc_execerror("Vector arguments not same size", nullptr);
        }
    } else {
        if (hoc_is_object_arg(1)) {
            Object* ob1 = *hoc_objgetarg(1);
            hoc_obj_ref(ob1);
        }
        ft->table = hoc_pgetarg(1);
        int argcnt = 2;
        for (size_t i = 0; i < n; ++i) {
            ta[i].nsize = *getarg(argcnt++);
            if (ta[i].nsize < 1) {
                hoc_execerror("size arg < 1 in hoc_spec_table", nullptr);
            }
            if (hoc_is_double_arg(argcnt)) {
                ta[i].min = *getarg(argcnt++);
                ta[i].max = *getarg(argcnt++);
                if (ta[i].max < ta[i].min) {
                    hoc_execerror("min > max in hoc_spec_table", nullptr);
                }
                ta[i].argvec = nullptr;
            } else {
                if (hoc_is_object_arg(argcnt)) {
                    Object* ob1 = *hoc_objgetarg(argcnt);
                    hoc_obj_ref(ob1);
                }
                ta[i].argvec = hoc_pgetarg(argcnt++);
            }
        }
    }
}
