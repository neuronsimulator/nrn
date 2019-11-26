/*
Copyright (c) 2016, Blue Brain Project
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once

/***
 * Includes all headers required to communicate and run all methods
 * described in CoreNEURON, neurox, and mod2c C-generated mechanisms
 * functions.
 **/


#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string.h>
#include <vector>
#include <array>

#include "coreneuron/utils/randoms/nrnran123.h"      //Random Number Generator
#include "coreneuron/scopmath_core/newton_struct.h"  //Newton Struct
#include "coreneuron/nrnoc/membdef.h"                //static definitions
#include "coreneuron/nrnoc/nrnoc_ml.h"               //Memb_list and mechs info

#include "coreneuron/nrniv/memory.h"  //Memory alignments and padding
#include "coreneuron/nrnconf.h"
#include "coreneuron/nrnoc/multicore.h"
#include "coreneuron/nrnoc/nrnoc_decl.h"
#include "coreneuron/nrnoc/mech_mapping.hpp"

namespace coreneuron {

// from nrnoc/capac.c
extern void nrn_init_capacitance(NrnThread*, Memb_list*, int);
extern void nrn_cur_capacitance(NrnThread* _nt, Memb_list* ml, int type);
extern void nrn_alloc_capacitance(double* data, Datum* pdata, int type);

// from nrnoc/eion.c
extern void nrn_init_ion(NrnThread*, Memb_list*, int);
extern void nrn_cur_ion(NrnThread* _nt, Memb_list* ml, int type);
extern void nrn_alloc_ion(double* data, Datum* pdata, int type);
extern void second_order_cur(NrnThread* _nt, int secondorder);

using DependencyTable = std::vector<std::vector<int>>;

/**
 * A class representing the CoreNEURON state, holding pointers to the various data structures
 *
 * The pointers to "global" data such as the NrnThread, Memb_list and Memb_func data structures
 * are managed here. they logically share their lifetime and runtime scope with instances of
 * this class.
 */
class CoreNeuron {


    /// Local to coreneuron, used to keep track of point process IDs
    int pointtype = 1; /* starts at 1 since 0 means not point in pnt_map*/

    /**
     * map if mech is a point process
     * In the future only a field of Mechanism class
     */
    std::vector<char> pnt_map; /* so prop_free can know its a point mech*/

    /** Vector mapping the types (IDs) of different mechanisms of mod files between NEURON and
     * CoreNEURON
     */
    std::vector<int> different_mechanism_type;

    /**
     * dependency helper filled by calls to hoc_register_dparam_semantics
     * used when nrn_mech_depend is called
     * vector-of-vector DS. First idx is the mech, second idx is the dependent mech.
     */
    DependencyTable ion_write_dependency;

    std::vector<Memb_func> memb_funcs;

    /**
     * Net send / Net receive
     * only used in CoreNEURON for book keeping synapse mechs, should go into CoreNEURON class
     */
    std::vector<std::pair<NetBufReceive_t, int>> net_buf_receive;
    std::vector<int> net_buf_send_type;

    /**
     * before-after-blocks from nmodl are registered here as function pointers
     */
    std::array<BAMech*, BEFORE_AFTER_SIZE> bamech;

    /**
     * Internal lookup tables. Number of float and int variables in each mechanism and memory layout
     * future --> mech class
     */
    std::vector<int> nrn_prop_param_size;
    std::vector<int> nrn_prop_dparam_size;
    std::vector<int> nrn_mech_data_layout; /* 1 AoS (default), 0 SoA */
    /* array is parallel to memb_func. All are 0 except 1 for ARTIFICIAL_CELL */
    std::vector<short> nrn_artcell_qindex;
    std::vector<bool> nrn_is_artificial;

    /**
     * Net Receive function pointer lookup tables
     */
    std::vector<pnt_receive_t> pnt_receive; /* for synaptic events. */
    std::vector<pnt_receive_t> pnt_receive_init;
    std::vector<short> pnt_receive_size;

    /**
     * Holds function pointers for WATCH callback
     */
    std::vector<nrn_watch_check_t> nrn_watch_check;

    /**
     * values are type numbers of mechanisms which do net_send call
     * related to NMODL net_event()
     *
     */
    std::vector<int> nrn_has_net_event;

    /**
     * inverse of nrn_has_net_event_ maps the values of nrn_has_net_event_ to the index of
     * ptntype2presyn
     */
    std::vector<int> pnttype2presyn;


    std::vector<bbcore_read_t> nrn_bbcore_read;
    std::vector<bbcore_write_t> nrn_bbcore_write;

  public:

    auto& get_memb_funcs() {
        return memb_funcs;
    }

    auto& get_memb_func(size_t idx) {
        return memb_funcs[idx];
    }

    auto& get_different_mechanism_type() {
        return different_mechanism_type;
    }

    auto& get_pnt_map() {
        return pnt_map;
    }

    auto& get_ion_write_dependency() {
        return ion_write_dependency;
    }

    auto& get_net_buf_receive() {
        return net_buf_receive;
    }

    auto& get_net_buf_send_type() {
        return net_buf_send_type;
    }

    auto& get_bamech() {
        return bamech;
    }

    auto& get_prop_param_size() {
        return nrn_prop_param_size;
    }

    auto& get_prop_dparam_size() {
        return nrn_prop_dparam_size;
    }

    auto& get_mech_data_layout() {
        return nrn_mech_data_layout;
    }

    auto& get_is_artificial() {
        return nrn_is_artificial;
    }

    auto& get_artcell_qindex() {
        return nrn_artcell_qindex;
    }

    auto& get_pnt_receive() {
        return pnt_receive;
    }

    auto& get_pnt_receive_init() {
        return pnt_receive_init;
    }

    auto& get_pnt_receive_size() {
        return pnt_receive_size;
    }

    auto& get_watch_check() {
        return nrn_watch_check;
    }

    auto& get_has_net_event() {
        return nrn_has_net_event;
    }

    auto& get_pnttype2presyn() {
        return pnttype2presyn;
    }

    auto& get_bbcore_read() {
        return nrn_bbcore_read;
    }

    auto& get_bbcore_write() {
        return nrn_bbcore_write;
    }


    /**
     * Generate point process IDs for pnt_map starting at 1 (since 0 means no point process)
     * \return the next available point process ID
     */
    int get_next_pointtype() {
        return pointtype++;
    }

};

extern CoreNeuron corenrn;

}  // namespace coreneuron
