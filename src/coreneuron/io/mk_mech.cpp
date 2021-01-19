/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#include <cstring>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>

#include "coreneuron/nrnconf.h"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/membrane_definitions.h"
#include "coreneuron/mechanism/register_mech.hpp"
#include "coreneuron/nrniv/nrniv_decl.h"
#include "coreneuron/utils/nrn_assert.h"
#include "coreneuron/mechanism/mech/cfile/cabvars.h"
#include "coreneuron/io/nrn2core_direct.h"
#include "coreneuron/coreneuron.hpp"
#include "coreneuron/mechanism//eion.hpp"

static char banner[] = "Duke, Yale, and the BlueBrain Project -- Copyright 1984-2020";

namespace coreneuron {
extern int nrn_nobanner_;

// NB: this should go away
extern std::string cnrn_version();
std::map<std::string, int> mech2type;

extern "C" {
void (*nrn2core_mkmech_info_)(std::ostream&);
}
static void mk_mech();
static void mk_mech(std::istream&);

/// Read meta data about the mechanisms and allocate corresponding mechanism management data
/// structures
void mk_mech(const char* datpath) {
    if (corenrn_embedded) {
        // we are embedded in NEURON
        mk_mech();
        return;
    }
    {
        std::string fname = std::string(datpath) + "/bbcore_mech.dat";
        std::ifstream fs(fname);

        if (!fs.good()) {
            fprintf(stderr,
                    "Error: couldn't find bbcore_mech.dat file in the dataset directory \n");
            fprintf(stderr,
                    "       Make sure to pass full directory path of dataset using -d DIR or "
                    "--datpath=DIR \n");
        }

        nrn_assert(fs.good());
        mk_mech(fs);
        fs.close();
    }
}

// we are embedded in NEURON, get info as stringstream from nrnbbcore_write.cpp
static void mk_mech() {
    static bool already_called = false;
    if (already_called) {
        return;
    }
    std::stringstream ss;
    nrn_assert(nrn2core_mkmech_info_);
    (*nrn2core_mkmech_info_)(ss);
    mk_mech(ss);
    already_called = true;
}

static void mk_mech(std::istream& s) {
    char version[256];
    s >> version;
    check_bbcore_write_version(version);

    //  printf("reading %s\n", fname);
    int n = 0;
    nrn_assert(s >> n);

    /// Allocate space for mechanism related data structures
    alloc_mech(n);

    /// Read all the mechanisms and their meta data
    for (int i = 2; i < n; ++i) {
        char mname[100];
        int type = 0, pnttype = 0, is_art = 0, is_ion = 0, dsize = 0, pdsize = 0;
        nrn_assert(s >> mname >> type >> pnttype >> is_art >> is_ion >> dsize >> pdsize);
        nrn_assert(i == type);
#ifdef DEBUG
        printf("%s %d %d %d %d %d %d\n", mname, type, pnttype, is_art, is_ion, dsize, pdsize);
#endif
        std::string str(mname);
        corenrn.get_memb_func(type).sym = (Symbol*) strdup(mname);
        mech2type[str] = type;
        corenrn.get_pnt_map()[type] = (char) pnttype;
        corenrn.get_prop_param_size()[type] = dsize;
        corenrn.get_prop_dparam_size()[type] = pdsize;
        corenrn.get_is_artificial()[type] = is_art;
        if (is_ion) {
            double charge = 0.;
            nrn_assert(s >> charge);
            // strip the _ion
            char iname[100];
            strcpy(iname, mname);
            iname[strlen(iname) - 4] = '\0';
            // printf("%s %s\n", mname, iname);
            ion_reg(iname, charge);
        }
        // printf("%s %d %d\n", mname, nrn_get_mechtype(mname), type);
    }

    if (nrnmpi_myid < 1 && nrn_nobanner_ == 0) {
        fprintf(stderr, " \n");
        fprintf(stderr, " %s\n", banner);
        fprintf(stderr, " Version : %s\n", cnrn_version().c_str());
        fprintf(stderr, " \n");
        fflush(stderr);
    }
    /* will have to put this back if any mod file refers to diam */
    //	register_mech(morph_mech, morph_alloc, (Pfri)0, (Pfri)0, (Pfri)0, (Pfri)0, -1, 0);

    /// Calling _reg functions for the default mechanisms from the file mech/cfile/cabvars.h
    for (int i = 0; mechanism[i]; i++) {
        (*mechanism[i])();
    }
}

/// Get mechanism type by the mechanism name
int nrn_get_mechtype(const char* name) {
    std::string str(name);
    std::map<std::string, int>::const_iterator mapit = mech2type.find(str);
    if (mapit == mech2type.end())
        return -1;  // Could not find the mechanism
    return mapit->second;
}

const char* nrn_get_mechname(int type) {
    for (std::map<std::string, int>::iterator i = mech2type.begin(); i != mech2type.end(); ++i) {
        if (type == i->second) {
            return i->first.c_str();
        }
    }
    return nullptr;
}
}  // namespace coreneuron
