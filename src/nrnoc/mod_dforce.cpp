#include <../../nrnconf.h>
/**
 * @file mod_dforce.cpp
 * @brief Call MOD PROCEDURE dforce() at IDA consistent initialization.
 *
 * Density mechanisms that define PROCEDURE dforce() are registered via
 * hoc_register_npy_direct as "dforce". At each IDA reinit (and finitialize),
 * NEURON invokes those procedures with global t set to the IC time so
 * assigned rates (e.g. dc/dt for variable capacitance) reflect the classical
 * right limit at t+.
 *
 * Point processes do not use npy_direct; they should set rates in BEFORE
 * BREAKPOINT and handle parameter jumps / charge conservation in NET_RECEIVE
 * (see models/dcmdt dcdt_pp.mod).
 */

#include "membfunc.h"
#include "multicore.h"
#include "nrn_ansi.h"
#include "section.h"

#include <string>
#include <unordered_map>

extern double t;
extern std::unordered_map<int, NPyDirectMechFuncs> nrn_mech2funcs_map;

int nrn_call_mod_dforce(double tt) {
    int ncalled = 0;
    if (nrn_nthread <= 0 || !nrn_threads) {
        return 0;
    }
    const double t_sav = t;
    t = tt;
    for (int it = 0; it < nrn_nthread; ++it) {
        NrnThread* nt = nrn_threads + it;
        const double nt_sav = nt->_t;
        nt->_t = tt;
        for (NrnThreadMembList* tml = nt->tml; tml; tml = tml->next) {
            const int type = tml->index;
            auto map_it = nrn_mech2funcs_map.find(type);
            if (map_it == nrn_mech2funcs_map.end()) {
                continue;
            }
            auto fit = map_it->second.find("dforce");
            if (fit == map_it->second.end() || !fit->second || !fit->second->func) {
                continue;
            }
            Memb_list* ml = tml->ml;
            if (!ml || ml->nodecount <= 0 || !ml->prop) {
                continue;
            }
            auto* const f = fit->second->func;
            for (int i = 0; i < ml->nodecount; ++i) {
                Prop* p = ml->prop[i];
                if (p) {
                    (void) f(p);
                    ++ncalled;
                }
            }
        }
        nt->_t = nt_sav;
    }
    t = t_sav;
    return ncalled;
}
