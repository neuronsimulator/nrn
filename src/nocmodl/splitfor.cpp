#include <../../nrnconf.h>

#include <unordered_map>
#include <stdio.h>
#include "modl.h"
#include "parse1.hpp"
#include "nmodlfunc.h"
#include "splitfor.h"

#define P(arg) fputs(arg, fcout)
#define ZF     "    _ZFOR {"

static bool split_cur_;
static bool split_solve_;

// check for assignment to non-range variable
static bool assign_non_range(List* lst) {
    bool b{false};
    for (Item* q = lst->next; q != lst; q = q->next) {
        if (q->itemtype == SYMBOL && strcmp(SYM(q)->name, "=") == 0) {
            Symbol* s = (q->prev->itemtype == SYMBOL) ? SYM(q->prev) : NULL;
            if (!s || (s->nrntype & (NRNRANGE | NRNCUROUT)) == 0) {
                printf("ZZZ assigning to %s nrntype=%o\n", s->name, s->nrntype);
                b = true;
                break;
            }
        }
    }
    return b;
}

static void split_printlist(List* lst) {
    std::unordered_map<Item*, bool> items{};
    // item Rangevar before "=", insert "_ZFOR{"
    // item SEMI, append "}"
    for (Item* q = lst->next; q != lst; q = q->next) {
        if (q->itemtype == SYMBOL && strcmp(SYM(q)->name, "=") == 0) {
            Symbol* s = (q->prev->itemtype == SYMBOL) ? SYM(q->prev) : NULL;
            if (s && (s->nrntype & (NRNRANGE | NRNCUROUT))) {
                items[q->prev] = true;
            }
        }
        if (q->itemtype == SYMBOL && SYM(q) == semi) {
            items[q] = false;
        }
    }

    for (Item* q = lst->next; q != lst; q = q->next) {
        auto search = items.find(q);
        if (search != items.end() && search->second) {
            P(ZF);
        }
        printitem(q);
        if (search != items.end() && !search->second) {
            P("}\n");
        }
    }
}

// return true if any splitfor code should be generated
bool splitfor() {
    static bool called;
    if (!called) {
        called = true;
        // if there is a breakpoint block with at least one current and assigns
        // only range variables.
        if (brkpnt_exists && currents->next != currents && !electrode_current) {
            // check for assignment to non-range variable
            if (!assign_non_range(modelfunc)) {
                split_cur_ = true;
            }
        }

        // if there is one nrnstate and it's solve method is cnexp
        printf("QQQ determine split_solve_\n");
        if (solvq) {
            for (Item* lq = solvq->next; lq != solvq; lq = lq->next) {
                Item* qsol = ITM(lq);
                lq = lq->next;
                Symbol* meth = SYM(lq);
                lq = lq->next;
                List* errstmt = LST(lq);
                if (meth && strcmp(meth->name, "cnexp") == 0) {
                    debugprintitem(qsol);
                }
            }
            split_solve_ = false;
        }
    }
    return split_cur_ || split_solve_;
}

// based on copies of code from noccout.cpp

void splitfor_current() {
    if (!split_cur_) {
        return;
    }
    printf("ZZZ splitfor_current()\n");
    P("\nstatic void _split_nrn_current(_internalthreadargsprotocomma_ "
      " int _nodecount){\n"
      "#define _ZFOR for (int _iml = 0; _iml < _nodecount; ++_iml)\n");

    split_printlist(modelfunc);
    P("\n#undef _ZFOR\n"
      "}\n");
}

static void splitfor_conductance_cout();
static void splitfor_ext_vdef();
static List* splitfor_ion_stmts(List* lst) {
    for (Item* q = lst->next; q != lst; q = q->next) {
        assert(q->itemtype == STRING);
        char* s = STR(q);
        int len = strlen(s);
        if (s[len - 1] != '\n') {  // rest of the statement is in q->next
            sprintf(buf, ZF "%s", s);
            replacstr(q, buf);
            q = q->next;  // this one must have newline
            assert(q->itemtype == STRING);
            s = STR(q);
            len = strlen(s);
            assert(s[len - 1] == '\n');
            s[len - 1] = '\0';
            sprintf(buf, "%s }\n", s);
            replacstr(q, buf);
        } else {  // an entire statement
            s[len - 1] = '\0';
            sprintf(buf, ZF "%s }\n", s);
            replacstr(q, buf);
        }
    }
    return lst;
}

static List* splitfor_end_dion_stmt(const char* strdel) {
    Item *q, *q1;
    static List* l;
    char* strion;

    l = newlist();
    ITERATE(q, useion) {
        strion = SYM(q)->name;
        q = q->next;
        q = q->next;
        ITERATE(q1, LST(q)) {
            if (SYM(q1)->nrntype & NRNCUROUT) {
                Sprintf(buf,
                        ZF " _ion_di%sdv += (_temp_%s[_iml] - %s)/%s",
                        strion,
                        SYM(q1)->name,
                        SYM(q1)->name,
                        strdel);
                Lappendstr(l, buf);
                if (point_process) {
                    Lappendstr(l, "* 1.e2/ (_nd_area); }\n");
                } else {
                    Lappendstr(l, "; }\n");
                }
            }
        }
        q = q->next;
    }
    return l;
}

void splitfor_cur(int part) {
    if (!split_cur_) {
        return;
    }
    if (part == 1) {
        P("#if !SPLITFOR\n");
        return;
    }
    P("#else /*SPLITFOR*/\n");
    P("#define _ZFOR for (int _iml = 0; _iml < _cntml; ++_iml)\n");
    P("  {\n");

    // modified copy of noccout.cpp output between calls to splitfor_cur
    // P("for (_iml = 0; _iml < _cntml; ++_iml) {\n");
    // P(" _ppvar = _ml_arg->_pdata[_iml];\n");
    if (currents->next != currents) {
        printlist(splitfor_ion_stmts(get_ion_variables(0)));
        cvode_rw_cur(buf);
        P(buf);
    }
    if (cvode_nrn_cur_solve_) {
        fprintf(fcout, "if (cvode_active_) { %s(_threadargs_); }\n", cvode_nrn_cur_solve_->name);
    }

    if (currents->next != currents) {
        if (conductance_) {
            P("#undef _DV\n#define _DV /**/\n");
            splitfor_ext_vdef();
            P(" {\n");
            splitfor_conductance_cout();
            printlist(splitfor_ion_stmts(set_ion_variables(0)));
            P(" }\n");
        } else {
            P("#undef _DV\n#define _DV + 0.001\n");
            splitfor_ext_vdef();
            P("    _split_nrn_current(_threadargscomma_ _cntml);\n");

            std::string s{"    std::vector<double>"};
            for (Item* q = currents->next; q != currents; q = q->next) {
                s += " _temp_";
                s += SYM(q)->name;
                s += "(_cntml)";
                s += (q->next == currents) ? ";\n" : ",";
            }
            P(s.c_str());

            s = ZF;
            for (Item* q = currents->next; q != currents; q = q->next) {
                sprintf(buf, " _temp_%s[_iml] = %s;", SYM(q)->name, SYM(q)->name);
                s += buf;
            }
            s += " }\n";
            P(s.c_str());

            P(ZF " v -= .001; }\n");
            if (state_discon_list_) {
                P("    state_discon_flag_ = 1; _split_nrn_current(_threadargscomma_ _cntml);"
                  " state_discon_flag_ = 0;\n");
            } else {
                P("    _split_nrn_current(_threadargscomma_ _cntml);\n");
            }


            printlist(splitfor_end_dion_stmt(".001"));

            s = ZF " _g = (";
            for (Item* q = currents->next; q != currents; q = q->next) {
                sprintf(buf, "(_temp_%s[_iml] - %s)", SYM(q)->name, SYM(q)->name);
                s += buf;
                s += (q->next == currents) ? ") / 0.001" : " + ";
            }
            s += point_process ? " * 1.e2/(_nd_area)" : "";
            s += "; }\n";
            P(s.c_str());

            /* set the ion variable values */
            printlist(splitfor_ion_stmts(set_ion_variables(0)));
        } /* end of not conductance */

        // store scaled sum of currents
        P("    std::vector<double> _rhsv(_cntml);\n");
        std::string s1 = ZF "_rhsv[_iml] = (";
        for (Item* q = currents->next; q != currents; q = q->next) {
            s1 += SYM(q)->name;
            s1 += (q->next == currents) ? ")" : " + ";
        }
        s1 += point_process ? " * 1.e2/(_nd_area)" : "";
        s1 += "; }\n";
        P(s1.c_str());

        if (electrode_current) {
            P(ZF "  _vec_rhs[_ni[_iml]] += _rhsv[_iml];\n");
            P("    if (_vec_sav_rhs) {\n");
            P("        _ZFOR { _vec_sav_rhs[_ni[_iml]] += _rhsv[_iml]; }\n");
            P("    }\n");
            P("#if EXTRACELLULAR\n");
            P(ZF "\n");
            P("        if (auto* const _extnode = _nrn_mechanism_access_extnode(_nd); _extnode) "
              "{\n");
            P("            *_extnode->_rhs[0] += _rhsv[_iml];\n");
            P("        }\n");
            P("    }\n");
            P("#endif\n");
        } else {
            P(ZF " _vec_rhs[_ni[_iml]] -= _rhsv[_iml]; }\n");
        }
    }

    P("  }\n");
    P("\n#undef _ZFOR\n");
    P("#endif /*SPLITFOR*/\n");
}

static void splitfor_conductance_cout() {
    int i = 0;
    Item* q;
    List* m;

    /* replace v with _v */
    m = newlist();
    ITERATE(q, modelfunc) {
        if (q->itemtype == SYMBOL) {
            if (strcmp(SYM(q)->name, "v") == 0) {
                lappendstr(m, "_v");
            } else {
                lappendsym(m, SYM(q));
            }
        } else if (q->itemtype == STRING) {
            lappendstr(m, STR(q));
        } else {
            diag("modelfunc contains item which is not a SYMBOL or STRING", (char*) 0);
        }
    }
    /* eliminate first { */
    ITERATE(q, m) {
        if (q->itemtype == SYMBOL) {
            if (strcmp(SYM(q)->name, "{") == 0) {
                remove(q);
                break;
            }
        }
    }
    /* eliminate last } */
    for (q = m->prev; q != m; q = q->prev) {
        if (q->itemtype == SYMBOL) {
            if (strcmp(SYM(q)->name, "}") == 0) {
                remove(q);
                break;
            }
        }
    }

    printlist(m);

    ITERATE(q, currents) {
        if (i == 0) {
            Sprintf(buf, "  _rhs = %s", breakpoint_current(SYM(q))->name);
        } else {
            Sprintf(buf, " + %s", breakpoint_current(SYM(q))->name);
        }
        P(buf);
        i += 1;
    }
    if (i > 0) {
        P(";\n");
    }

    i = 0;
    ITERATE(q, conductance_) {
        if (i == 0) {
            Sprintf(buf, "  _g = %s", SYM(q)->name);
        } else {
            Sprintf(buf, " + %s", SYM(q)->name);
        }
        P(buf);
        i += 1;
        q = q->next;
    }
    if (i > 0) {
        P(";\n");
    }

    ITERATE(q, conductance_) {
        if (SYM(q->next)) {
            Sprintf(buf, "  _ion_di%sdv += %s", SYM(q->next)->name, SYM(q)->name);
            P(buf);
            if (point_process) {
                P("* 1.e2/(_nd_area)");
            }
            P(";\n");
        }
        q = q->next;
    }
}

static void splitfor_ext_vdef() {
    if (artificial_cell) {
        return;
    }
    if (electrode_current) {
        P("#if EXTRACELLULAR\n");
        P(ZF "\n");
        P("        auto* _nd = _ml_arg->_nodelist[_iml];\n");
        P("        if (auto* const _extnode = _nrn_mechanism_access_extnode(_nd); _extnode) {\n");
        P("            v = NODEV(_nd) + _extnode->_v[0] _DV;\n");
        P("        }else\n");
        P("            v = _vec_v[_ni[_iml]] _DV;\n");
        P("        }\n");
        P("   }\n");
        P("#else\n");
        P(ZF "\n");
        P("        v = _vec_v[_ni[_iml]] _DV;\n");
        P("    }\n");
        P("#endif\n");
    } else {
        P(ZF " v = _vec_v[_ni[_iml]] _DV; }\n");
    }
}

void splitfor_solve() {
    if (!split_solve_) {
        return;
    }
    printf("ZZZ splitfor_solve()\n");
}
