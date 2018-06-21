#include <stdio.h>
#include <string.h>
#include <map>
#include <string>
#include <algorithm>

#include "coreneuron/utils/randoms/nrnran123.h"
#include "coreneuron/nrnconf.h"
#include "coreneuron/nrniv/nrniv_decl.h"
#include "coreneuron/nrnoc/membfunc.h"
#include "coreneuron/nrniv/nrn_assert.h"

using namespace std;
namespace coreneuron {
typedef pair<size_t, double*> PSD;
typedef map<string, PSD> N2V;

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

void set_globals(const char* path, int cli_global_seed) {
    if (!n2v) {
        n2v = new N2V();
    }
    (*n2v)["celsius"] = PSD(0, &celsius);
    (*n2v)["dt"] = PSD(0, &dt);
    (*n2v)["t"] = PSD(0, &t);

    string fname = string(path) + string("/globals.dat");
    FILE* f = fopen(fname.c_str(), "r");
    if (!f) {
        printf("ignore: could not open %s\n", fname.c_str());
        delete n2v;
        n2v = NULL;
        return;
    }

    char line[256];
    char name[256];
    double val;
    int n;

    fscanf(f, "%s\n", line);
    check_bbcore_write_version(line);

    for (;;) {
        nrn_assert(fgets(line, 256, f) != NULL);
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
                nrn_assert(it->second.first == (size_t)n);
                double* pval = it->second.second;
                for (int i = 0; i < n; ++i) {
                    nrn_assert(fgets(line, 256, f) != NULL);
                    nrn_assert(sscanf(line, "%lf\n", &val) == 1);
                    pval[i] = val;
                }
            }
        } else {
            nrn_assert(0);
        }
    }

    while (fgets(line, 256, f)) {
        if (sscanf(line, "%s %d", name, &n) == 2) {
            if (strcmp(name, "secondorder") == 0) {
                secondorder = n;
            } else if ( strcmp(name, "Random123_globalindex") == 0) {
                nrnran123_set_globalindex((uint32_t)n);
            }
        }
    }

    fclose(f);
    // overwrite global.dat config if seed is specified on Command line
    if (cli_global_seed != -1) {
      nrnran123_set_globalindex((uint32_t)cli_global_seed);
    }
#if 0
  for (N2V::iterator i = n2v->begin(); i != n2v->end(); ++i) {
    printf("%s %ld %p\n", i->first.c_str(), i->second.first, i->second.second);
  }
#endif

    delete n2v;
    n2v = NULL;
}
}  // namespace coreneuron
