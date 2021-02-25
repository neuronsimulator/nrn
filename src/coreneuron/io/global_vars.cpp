/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <algorithm>

#include "coreneuron/utils/randoms/nrnran123.h"
#include "coreneuron/nrnconf.h"
#include "coreneuron/mechanism/membfunc.hpp"
#include "coreneuron/utils/nrn_assert.h"
#include "coreneuron/io/nrn2core_direct.h"
#include "coreneuron/utils/nrnoc_aux.hpp"

void* (*nrn2core_get_global_dbl_item_)(void*, const char*& name, int& size, double*& val);
int (*nrn2core_get_global_int_item_)(const char* name);

using namespace std;
namespace coreneuron {
using PSD = pair<size_t, double*>;
using N2V = map<string, PSD>;

static N2V* n2v;

void hoc_register_var(DoubScal* ds, DoubVec* dv, VoidFunc*) {
    if (!n2v) {
        n2v = new N2V();
    }
    for (size_t i = 0; ds[i].name; ++i) {
        (*n2v)[ds[i].name] = PSD(0, ds[i].pdoub);
    }
    for (size_t i = 0; dv[i].name; ++i) {
        (*n2v)[dv[i].name] = PSD(dv[i].index1, ds[i].pdoub);
    }
}

void set_globals(const char* path, bool cli_global_seed, int cli_global_seed_value) {
    if (!n2v) {
        n2v = new N2V();
    }
    (*n2v)["celsius"] = PSD(0, &celsius);
    (*n2v)["dt"] = PSD(0, &dt);
    (*n2v)["t"] = PSD(0, &t);
    (*n2v)["PI"] = PSD(0, &pi);

    if (corenrn_embedded) {  // CoreNEURON embedded, get info direct from NEURON

        const char* name;
        int size;
        double* val = nullptr;
        for (void* p = nullptr;
             (p = (*nrn2core_get_global_dbl_item_)(p, name, size, val)) != nullptr;) {
            N2V::iterator it = n2v->find(name);
            if (it != n2v->end()) {
                if (size == 0) {
                    nrn_assert(it->second.first == 0);
                    *(it->second.second) = val[0];
                } else {
                    nrn_assert(it->second.first == (size_t) size);
                    double* pval = it->second.second;
                    for (int i = 0; i < size; ++i) {
                        pval[i] = val[i];
                    }
                }
            }
            delete[] val;
        }
        secondorder = (*nrn2core_get_global_int_item_)("secondorder");
        nrnran123_set_globalindex((*nrn2core_get_global_int_item_)("Random123_global_index"));

    } else {  // get the info from the globals.dat file
        string fname = string(path) + string("/globals.dat");
        FILE* f = fopen(fname.c_str(), "r");
        if (!f) {
            printf("ignore: could not open %s\n", fname.c_str());
            delete n2v;
            n2v = nullptr;
            return;
        }

        char line[256];

        nrn_assert(fscanf(f, "%s\n", line) == 1);
        check_bbcore_write_version(line);

        for (;;) {
            char name[256];
            double val;
            int n;
            nrn_assert(fgets(line, 256, f) != nullptr);
            N2V::iterator it;
            if (sscanf(line, "%s %lf", name, &val) == 2) {
                if (strcmp(name, "0") == 0) {
                    break;
                }
                it = n2v->find(name);
                if (it != n2v->end()) {
                    nrn_assert(it->second.first == 0);
                    *(it->second.second) = val;
                }
            } else if (sscanf(line, "%[^[][%d]\n", name, &n) == 2) {
                if (strcmp(name, "0") == 0) {
                    break;
                }
                it = n2v->find(name);
                if (it != n2v->end()) {
                    nrn_assert(it->second.first == (size_t) n);
                    double* pval = it->second.second;
                    for (int i = 0; i < n; ++i) {
                        nrn_assert(fgets(line, 256, f) != nullptr);
                        nrn_assert(sscanf(line, "%lf\n", &val) == 1);
                        pval[i] = val;
                    }
                }
            } else {
                nrn_assert(0);
            }
        }

        while (fgets(line, 256, f)) {
            char name[256];
            int n;
            if (sscanf(line, "%s %d", name, &n) == 2) {
                if (strcmp(name, "secondorder") == 0) {
                    secondorder = n;
                } else if (strcmp(name, "Random123_globalindex") == 0) {
                    nrnran123_set_globalindex((uint32_t) n);
                } else if (strcmp(name, "_nrnunit_use_legacy_") == 0) {
                    if (n != CORENRN_USE_LEGACY_UNITS) {
                        hoc_execerror(
                            "CORENRN_ENABLE_LEGACY_UNITS not"
                            " consistent with NEURON value of"
                            " nrnunit_use_legacy()",
                            NULL);
                    }
                }
            }
        }

        fclose(f);

        // overwrite global.dat config if seed is specified on Command line
        if (cli_global_seed) {
            nrnran123_set_globalindex((uint32_t) cli_global_seed_value);
        }
    }

#if DEBUG
    for (N2V::iterator i = n2v->begin(); i != n2v->end(); ++i) {
        printf("%s %ld %p\n", i->first.c_str(), i->second.first, i->second.second);
    }
#endif

    delete n2v;
    n2v = nullptr;
}

}  // namespace coreneuron
