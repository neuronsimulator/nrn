#include <../../nrnconf.h>
/* /local/src/master/nrn/src/oc/code2.cpp,v 1.12 1999/06/08 17:48:26 hines Exp */

#include "hoc.h"
#include "hocstr.h"
#include "parse.hpp"
#include "hocparse.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <ctype.h>
#include <errno.h>
#include "nrnfilewrap.h"
#include <cstring>


int units_on_flag_;

extern char** gargv;
extern int gargc;
extern double chkarg(int, double low, double high);
extern Symlist* hoc_built_in_symlist;
extern Symlist* hoc_top_level_symlist;
extern Symbol* hoc_table_lookup(const char*, Symlist*);

extern char** hoc_pgargstr(int);

float* hoc_sym_domain(Symbol* sym) {
    if (sym && sym->extra) {
        return sym->extra->parmlimits;
    }
    return (float*) 0;
}

HocSymExtension* hoc_var_extra(const char* name) {
    Symbol* sym;
    sym = hoc_lookup(name);
    if (sym) {
        return sym->extra;
    } else {
        return (HocSymExtension*) 0;
    }
}

Symbol* hoc_name2sym(const char* name) {
    char *buf, *cp;
    Symbol* sym;
    buf = static_cast<char*>(emalloc(strlen(name) + 1));
    strcpy(buf, name);
    for (cp = buf; *cp; ++cp) {
        if (*cp == '.') {
            *cp = '\0';
            ++cp;
            break;
        }
    }
    sym = hoc_table_lookup(buf, hoc_built_in_symlist);
    if (!sym) {
        sym = hoc_table_lookup(buf, hoc_top_level_symlist);
    }
    if (sym && *cp == '\0') {
        free(buf);
        return sym;
    } else if (sym && sym->type == TEMPLATE && *cp != '\0') {
        sym = hoc_table_lookup(cp, sym->u.ctemplate->symtable);
        if (sym) {
            free(buf);
            return sym;
        }
    }
    free(buf);
    return (Symbol*) 0;
}

void hoc_Symbol_limits(void) {
    Symbol* sym;
    extern Symbol* hoc_get_last_pointer_symbol();

    if (hoc_is_str_arg(1)) {
        char* name = gargstr(1);
        sym = hoc_name2sym(name);
        if (!sym) {
            hoc_execerror("Cannot find the symbol for ", name);
        }
    } else {
        sym = hoc_get_last_pointer_symbol();
        if (!sym) {
            hoc_execerror(
                "Cannot find the symbol associated with the pointer when called from Python.",
                "Use a string instead of pointer argument");
        }
    }
    assert(sym);
    hoc_symbol_limits(sym, *getarg(2), *getarg(3));
    ret();
    pushx(1.);
}

void hoc_symbol_limits(Symbol* sym, float low, float high) {
    sym_extra_alloc(sym);
    if (!sym->extra->parmlimits) {
        sym->extra->parmlimits = (float*) emalloc(2 * sizeof(float));
    }
    sym->extra->parmlimits[0] = low;
    sym->extra->parmlimits[1] = high;
}

void hoc_symbol_tolerance(Symbol* sym, double tol) {
    sym_extra_alloc(sym);
    sym->extra->tolerance = tol;
}

double check_domain_limits(float* limits, double val) {
    if (limits) {
        if (val < limits[0]) {
            return (double) limits[0];
        } else if (val > limits[1]) {
            return (double) limits[1];
        }
    }
    return val;
}


char* hoc_symbol_units(Symbol* sym, const char* units) {
    if (!sym) {
        return (char*) 0;
    }
    if (units) {
        if (sym->extra && sym->extra->units) {
            free(sym->extra->units);
            sym->extra->units = (char*) 0;
        }
        sym_extra_alloc(sym);
        sym->extra->units = static_cast<char*>(emalloc(strlen(units) + 1));
        strcpy(sym->extra->units, units);
    }
    if (sym->extra && sym->extra->units) {
        return sym->extra->units;
    } else {
        return (char*) 0;
    }
}

void hoc_Symbol_units(void) {
    Symbol* sym;
    extern Symbol* hoc_get_last_pointer_symbol();
    char** units = hoc_temp_charptr();

    if (hoc_is_double_arg(1)) {
        units_on_flag_ = (int) chkarg(1, 0., 1.);
        if (units_on_flag_) {
            *units = const_cast<char*>("on");
        } else {
            *units = const_cast<char*>("off");
        }
    } else {
        if (hoc_is_str_arg(1)) {
            char* name = gargstr(1);
            sym = hoc_name2sym(name);
            if (!sym) {
                hoc_execerror("Cannot find the symbol for ", name);
            }
        } else {
            sym = hoc_get_last_pointer_symbol();
            if (!sym) {
                hoc_execerror(
                    "Cannot find the symbol associated with the pointer when called from Python.",
                    "Use a string instead of pointer argument");
            }
        }
        assert(sym);
        *units = (char*) 0;
        if (ifarg(2)) {
            *units = gargstr(2);
        }
        *units = hoc_symbol_units(sym, *units);
        if (*units == (char*) 0) {
            *units = const_cast<char*>("");
        }
    }
    hoc_ret();
    hoc_pushstr(units);
}

char* hoc_back2forward(char*);
char* neuronhome_forward(void) {
    extern char* neuron_home;
#ifdef WIN32
    static char* buf;
    extern void hoc_forward2back();
    if (!buf) {
        buf = static_cast<char*>(emalloc(strlen(neuron_home) + 1));
        strcpy(buf, neuron_home);
    }
    hoc_back2forward(buf);
    return buf;
#else
    return neuron_home;
#endif
}

char* neuron_home_dos;
extern void setneuronhome(const char*);
void hoc_neuronhome(void) {
    extern char* neuron_home;
#ifdef WIN32
    if (ifarg(1) && (int) chkarg(1, 0., 1.) == 1) {
        if (!neuron_home_dos) {
            setneuronhome(NULL);
        }
        hoc_ret();
        hoc_pushstr(&neuron_home_dos);
    } else {
        hoc_ret();
        hoc_pushstr(&neuron_home);
    }
#else
    hoc_ret();
    hoc_pushstr(&neuron_home);
#endif
}

char* gargstr(int narg) /* Return pointer to string which is the narg argument */
{
    return *hoc_pgargstr(narg);
}

void hoc_Strcmp(void) {
    char *s1, *s2;
    s1 = gargstr(1);
    s2 = gargstr(2);
    ret();
    pushx((double) strcmp(s1, s2));
}

static int hoc_vsscanf(const char* buf);

void hoc_sscanf(void) {
    int n;
    n = hoc_vsscanf(gargstr(1));
    ret();
    pushx((double) n);
}

static int hoc_vsscanf(const char* buf) {
    /* assumes arg2 format string from hoc as well as remaining args */
    char *pf, *format, errbuf[100];
    void* arglist[20];
    int n = 0, iarg, i, islong, convert, sawnum;
    struct {
        union {
            double d;
            float f;
            long l;
            int i;
            char* s;
            char c;
        } u;
        int type;
    } arg[20];
    for (i = 0; i < 20; ++i) {
        arglist[i] = nullptr;
    }
    format = gargstr(2);
    iarg = 0;
    errbuf[0] = '\0';
    for (pf = format; *pf; ++pf) {
        if (*pf == '%') {
            convert = 1;
            islong = 0;
            sawnum = 0;
            ++pf;
            if (!*pf)
                goto incomplete;
            if (*pf == '*') {
                convert = 0;
                ++pf;
                if (!*pf)
                    goto incomplete;
            }
            if (convert && iarg >= 19) {
                goto too_many;
            }
            while (isdigit(*pf)) {
                sawnum = 1;
                ++pf;
                if (!*pf)
                    goto incomplete;
            }
            if (*pf == 'l') {
                islong = 1;
                ++pf;
                if (!*pf)
                    goto incomplete;
            }
            if (convert)
                switch (*pf) {
                case '%':
                    convert = 0;
                    break;
                case 'd':
                case 'i':
                case 'o':
                case 'u':
                case 'x':
                    if (islong) {
                        arg[iarg].type = 'l';
                        arglist[iarg] = (void*) &arg[iarg].u.l;
                    } else {
                        arg[iarg].type = 'i';
                        arglist[iarg] = (void*) &arg[iarg].u.i;
                    }
                    break;
                case 'e':
                case 'f':
                case 'g':
                    if (islong) {
                        arg[iarg].type = 'd';
                        arglist[iarg] = (void*) &arg[iarg].u.d;
                    } else {
                        arg[iarg].type = 'f';
                        arglist[iarg] = (void*) &arg[iarg].u.f;
                    }
                    break;
                case '[':
                    if (islong) {
                        goto bad_specifier;
                    }
                    i = 0; /* watch out for []...] and [^]...] */
                    for (;;) {
                        if (!*pf)
                            goto incomplete;
                        if (*pf == ']') {
                            if (!((i == 1) || ((i == 2) && (pf[-1] == '^')))) {
                                break;
                            }
                        }
                        ++i;
                        ++pf;
                    }
                case 's':
                    if (islong) {
                        goto bad_specifier;
                    }
                    arg[iarg].type = 's';
                    arg[iarg].u.s = static_cast<char*>(emalloc(strlen(buf) + 1));
                    arglist[iarg] = (void*) arg[iarg].u.s;
                    break;
                case 'c':
                    if (islong || sawnum) {
                        goto bad_specifier;
                    }
                    arg[iarg].type = 'c';
                    arglist[iarg] = (void*) &arg[iarg].u.c;
                    break;
                default:
                    goto bad_specifier;
                    /*break;*/
                }
            if (convert) {
                ++iarg;
                if (!ifarg(iarg + 2)) {
                    goto missing_arg;
                }
                switch (arg[iarg - 1].type) {
                case 's':
                    if (!hoc_is_str_arg(iarg + 2)) {
                        Sprintf(errbuf, "arg %d must be a string", iarg + 2);
                        goto bad_arg;
                    }
                    break;
                default:
                    if (!hoc_is_pdouble_arg(iarg + 2)) {
                        Sprintf(errbuf, "arg %d must be a pointer to a number", iarg + 2);
                        goto bad_arg;
                    }
                    break;
                }
            }
        }
    }

    if (iarg < 4) {
        n = sscanf(buf, format, arglist[0], arglist[1], arglist[2]);
    } else if (iarg < 13) {
        n = sscanf(buf,
                   format,
                   arglist[0],
                   arglist[1],
                   arglist[2],
                   arglist[3],
                   arglist[4],
                   arglist[5],
                   arglist[6],
                   arglist[7],
                   arglist[8],
                   arglist[9],
                   arglist[10],
                   arglist[11]);
    } else {
        goto too_many;
    }
    assert(n <= iarg);

    for (i = 0; i < n; ++i) {
        switch (arg[i].type) {
        case 'd':
            *hoc_pgetarg(i + 3) = arg[i].u.d;
            break;
        case 'i':
            *hoc_pgetarg(i + 3) = (double) arg[i].u.i;
            break;
        case 'l':
            *hoc_pgetarg(i + 3) = (double) arg[i].u.l;
            break;
        case 'f':
            *hoc_pgetarg(i + 3) = (double) arg[i].u.f;
            break;
        case 's':
            hoc_assign_str(hoc_pgargstr(i + 3), arg[i].u.s);
            break;
        case 'c':
            *hoc_pgetarg(i + 3) = (double) arg[i].u.c;
            break;
        }
    }
    goto normal;
incomplete:
    Sprintf(errbuf, "incomplete format specifier for arg %d", iarg + 3);
    goto normal;
bad_specifier:
    Sprintf(errbuf, "unknown conversion specifier for arg %d", iarg + 3);
    goto normal;
missing_arg:
    Sprintf(errbuf, "missing arg %d", iarg + 2);
    goto normal;
bad_arg:
    goto normal;
too_many:
    Sprintf(errbuf, "too many ( > %d) args", iarg + 2);
    goto normal;
normal:
    for (i = 0; i < iarg; ++i) {
        if (arg[i].type == 's') {
            free(arg[i].u.s);
        }
    }
    if (errbuf[0]) {
        hoc_execerror("scan error:", errbuf);
    }
    return n;
}

void System(void) {
    extern int hoc_plttext;
    static char stdoutfile[] = "/systmp.tmp";
    double d;
    FILE* fp;

    if (hoc_plttext && !strchr(gargstr(1), '>')) {
        int n;
        HocStr* st;
        n = strlen(gargstr(1)) + strlen(stdoutfile);
        st = hocstr_create(n + 256);
        std::snprintf(st->buf, st->size + 1, "%s > %s", gargstr(1), stdoutfile);
        d = (double) system(st->buf);
#if 1
        if ((fp = fopen(stdoutfile, "r")) == (FILE*) 0) {
            hoc_execerror("Internal error in System(): can't open", stdoutfile);
        }
        while (fgets(st->buf, 255, fp) == st->buf) {
            plprint(st->buf);
        }
#endif
        hocstr_delete(st);
        IGNORE(unlink(stdoutfile));
    } else if (ifarg(2)) {
        NrnFILEWrap* fpw;
        extern HocStr* hoc_tmpbuf;
        HocStr* line;
        int i;
        fp = popen(gargstr(1), "r");
        if (!fp) {
            hoc_execerror("could not popen the command:", gargstr(1));
        }
        line = hocstr_create(1000);
        i = 0;
        hoc_tmpbuf->buf[0] = '\0';
        fpw = nrn_fw_wrap(fp);
        while (fgets_unlimited(line, fpw)) {
            i += strlen(line->buf);
            if (hoc_tmpbuf->size <= i) {
                hocstr_resize(hoc_tmpbuf, 2 * hoc_tmpbuf->size);
            }
            strcat(hoc_tmpbuf->buf, line->buf);
        }
        hocstr_delete(line);
        d = (double) pclose(fp);
        nrn_fw_delete(fpw);
        hoc_assign_str(hoc_pgargstr(2), hoc_tmpbuf->buf);
    } else {
        d = (double) system(gargstr(1));
    }
    errno = 0;
    ret();
    pushx(d);
}

void Xred(void) /* read with prompt string and default and limits */
{
    double d;

    d = xred(gargstr(1), *getarg(2), *getarg(3), *getarg(4));
    ret();
    pushx(d);
}

static struct { /* symbol types */
    const char* name;
    short t_type;
} type_sym[] = {{"Builtins", BLTIN},
                {"Other Builtins", FUN_BLTIN},
                {"Functions", FUNCTION},
                {"Procedures", PROCEDURE},
                {"Undefined", UNDEF},
                {"Scalars", VAR},
                {0, 0}};

static void symdebug(const char* s, Symlist* list) /* for debugging display the symbol lists */
{
    Symbol* sp;

    Printf("\n\nSymbol list %s\n\n", s);
    if (list)
        for (sp = list->first; sp != (Symbol*) 0; sp = sp->next) {
            Printf("name:%s\ntype:", sp->name);
            switch (sp->type) {
            case VAR:
                if (!ISARRAY(sp)) {
                    if (sp->subtype == USERINT)
                        Printf("VAR USERINT  %8d", *(sp->u.pvalint));
                    else if (sp->subtype == USERDOUBLE)
                        Printf("VAR USERDOUBLE  %.8g", *(OPVAL(sp)));
                    else
                        Printf("VAR   %.8g", *(OPVAL(sp)));
                } else {
                    if (sp->subtype == USERINT)
                        Printf("ARRAY USERINT");
                    else if (sp->subtype == USERDOUBLE)
                        Printf("ARRAY USERDOUBLE");
                    else
                        Printf("ARRAY");
                }
                break;
            case NUMBER:
                Printf("NUMBER   %.8g", *(OPVAL(sp)));
                break;
            case STRING:
                Printf("STRING   %s", *(OPSTR(sp)));
                break;
            case UNDEF:
                Printf("UNDEF");
                break;
            case BLTIN:
                Printf("BLTIN");
                break;
            case AUTO:
                Printf("AUTO");
                break;
            case FUNCTION:
                Printf("FUNCTION");
                symdebug(sp->name, sp->u.u_proc->list);
                break;
            case PROCEDURE:
                Printf("PROCEDURE");
                symdebug(sp->name, sp->u.u_proc->list);
                break;
            case FUN_BLTIN:
                Printf("FUN_BLTIN");
                break;
            default:
                Printf("%d", sp->type);
                break;
            }
            Printf("\n");
        }
}

void symbols(void) /* display the types above */
{
    int i, j;
    Symbol* sp;

    if (zzdebug == 0)
        for (i = 0; type_sym[i].t_type != 0; i++) {
            Printf("\n%s\n", type_sym[i].name);
            for (sp = symlist->first; sp != (Symbol*) 0; sp = sp->next)
                if (sp->type == type_sym[i].t_type) {
                    Printf("\t%s", sp->name);
                    switch (sp->type) {
                    case VAR:
                        if (ISARRAY(sp)) {
                            for (j = 0; j < sp->arayinfo->nsub; j++)
                                Printf("[%d]", sp->arayinfo->sub[j]);
                        }
                        break;

                    default:
                        break;
                    }
                }
            Printf("\n");
        }
    else {
        symdebug("p_symlist", p_symlist);
        symdebug("symlist", symlist);
    }
    ret();
    pushx(0.);
}

double chkarg(int arg, double low, double high) /* argument checking for user functions */
{
    double val;

    val = *getarg(arg);
    if (val > high || val < low) {
        hoc_execerror("Arg out of range in user function", (char*) 0);
    }
    return val;
}


double hoc_run_expr(Symbol* sym) /* value of expression in sym made by hoc_parse_expr*/
{
    Inst* pcsav = hoc_pc;
    hoc_execute(sym->u.u_proc->defn.in);
    hoc_pc = pcsav;
    return hoc_ac_;
}

Symbol* hoc_parse_expr(const char* str, Symlist** psymlist) {
    Symbol* sp;
    char s[BUFSIZ];
    extern Symlist* hoc_top_level_symlist;

    if (!psymlist) {
        psymlist = &hoc_top_level_symlist;
    }
    sp = hoc_install(str, PROCEDURE, 0., psymlist);
    sp->u.u_proc->defn.in = STOP;
    sp->u.u_proc->list = (Symlist*) 0;
    sp->u.u_proc->nauto = 0;
    sp->u.u_proc->nobjauto = 0;
    if (strlen(str) > BUFSIZ - 20) {
        HocStr* s;
        s = hocstr_create(strlen(str) + 20);
        std::snprintf(s->buf, s->size + 1, "hoc_ac_ = %s\n", str);
        hoc_xopen_run(sp, s->buf);
        hocstr_delete(s);
    } else {
        Sprintf(s, "hoc_ac_ = %s\n", str);
        hoc_xopen_run(sp, s);
    }
    return sp;
}

void hoc_run_stmt(Symbol* sym) {
    Inst* pcsav = hoc_pc;
    hoc_execute(sym->u.u_proc->defn.in);
    hoc_pc = pcsav;
}
extern Symlist* hoc_top_level_symlist;

Symbol* hoc_parse_stmt(const char* str, Symlist** psymlist) {
    Symbol* sp;
    char s[BUFSIZ];

    if (!psymlist) {
        psymlist = &hoc_top_level_symlist;
    }
    sp = hoc_install(str, PROCEDURE, 0., psymlist);
    sp->u.u_proc->defn.in = STOP;
    sp->u.u_proc->list = (Symlist*) 0;
    sp->u.u_proc->nauto = 0;
    sp->u.u_proc->nobjauto = 0;
    if (strlen(str) > BUFSIZ - 10) {
        HocStr* s;
        s = hocstr_create(strlen(str) + 10);
        std::snprintf(s->buf, s->size + 1, "{%s}\n", str);
        hoc_xopen_run(sp, s->buf);
        hocstr_delete(s);
    } else {
        Sprintf(s, "{%s}\n", str);
        hoc_xopen_run(sp, s);
    }
    return sp;
}

/**
 * @brief Executing hoc_pointer(&var) will put the address of the variable in this location.
 */
neuron::container::data_handle<double> hoc_varhandle;

void hoc_pointer() {
    hoc_varhandle = hoc_hgetarg<double>(1);
    hoc_ret();
    hoc_pushx(1.);
}

neuron::container::data_handle<double> hoc_val_handle(std::string_view s) {
    constexpr std::string_view prefix{"{hoc_pointer_(&"}, suffix{")}\n"};
    std::string code;
    code.reserve(prefix.size() + suffix.size() + s.size());
    code.append(prefix);
    code.append(s);
    code.append(")}\n");
    hoc_varhandle = {};
    auto const status = hoc_oc(code.c_str());
    assert(status == 0);
    return hoc_varhandle;
}

double* hoc_val_pointer(const char* s) {
    return static_cast<double*>(hoc_val_handle(s));
}

void hoc_name_declared(void) {
    Symbol* s;
    extern Symlist* hoc_top_level_symlist;
    Symlist* slsav;
    int x;
    int arg2 = 0;
    if (ifarg(2)) {
        arg2 = chkarg(2, 0., 2.);
    }
    if (arg2 == 1.) {
        s = hoc_lookup(gargstr(1));
    } else {
        slsav = hoc_symlist;
        hoc_symlist = hoc_top_level_symlist;
        s = hoc_lookup(gargstr(1));
        hoc_symlist = slsav;
    }
    x = s ? 1. : 0.;
    if (s) {
        x = (s->type == OBJECTVAR) ? 2 : x;
        x = (s->type == SECTION) ? 3 : x;
        x = (s->type == STRING) ? 4 : x;
        x = (s->type == VAR) ? 5 : x;
        if (x == 5 && arg2 == 2) {
            x = (s->arayinfo) ? 6 : x;
            x = (s->subtype == USERINT) ? 7 : x;
            x = (s->subtype == USERPROPERTY) ? 8 : x;
        }
    }
    ret();
    pushx((double) x);
}
