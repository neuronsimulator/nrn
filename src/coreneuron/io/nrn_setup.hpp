/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#pragma once

#include <string>
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/io/nrn_filehandler.hpp"
#include "coreneuron/io/nrn2core_direct.h"
#include "coreneuron/io/user_params.hpp"
#include "coreneuron/io/mem_layout_util.hpp"
#include "coreneuron/io/nrn_checkpoint.hpp"

namespace coreneuron {
void read_phase1(NrnThread& nt, UserParams& userParams);
void read_phase2(NrnThread& nt, UserParams& userParams);
void read_phase3(NrnThread& nt, UserParams& userParams);
void read_phasegap(NrnThread& nt, UserParams& userParams);
void setup_ThreadData(NrnThread& nt);

void nrn_setup(const char* filesdat,
               bool is_mapping_needed,
               CheckPoints& checkPoints,
               bool run_setup_cleanup = true,
               const char* datapath = "",
               const char* restore_path = "",
               double* mindelay = nullptr);

// Functions to load and clean data;
extern void nrn_init_and_load_data(int argc,
                                   char** argv,
                                   CheckPoints& checkPoints,
                                   bool is_mapping_needed = false,
                                   bool run_setup_cleanup = true);
extern void allocate_data_in_mechanism_nrn_init();
extern void nrn_setup_cleanup();

extern int nrn_i_layout(int i, int cnt, int j, int size, int layout);

size_t memb_list_size(NrnThreadMembList* tml, bool include_data);

size_t model_size(bool detailed_report);

namespace coreneuron {


/// Reading phase number.
enum phase { one = 1, two, three, gap };

/// Get the phase number in form of the string.
template <phase P>
inline std::string getPhaseName();

template <>
inline std::string getPhaseName<one>() {
    return "1";
}

template <>
inline std::string getPhaseName<two>() {
    return "2";
}

template <>
inline std::string getPhaseName<three>() {
    return "3";
}

template <>
inline std::string getPhaseName<gap>() {
    return "gap";
}

/// Reading phase selector.
template <phase P>
inline void read_phase_aux(NrnThread& nt, UserParams&);

template <>
inline void read_phase_aux<one>(NrnThread& nt, UserParams& userParams) {
    read_phase1(nt, userParams);
}

template <>
inline void read_phase_aux<two>(NrnThread& nt, UserParams& userParams) {
    read_phase2(nt, userParams);
}

template <>
inline void read_phase_aux<three>(NrnThread& nt, UserParams& userParams) {
    read_phase3(nt, userParams);
}

template <>
inline void read_phase_aux<gap>(NrnThread& nt, UserParams& userParams) {
    read_phasegap(nt, userParams);
}

/// Reading phase wrapper for each neuron group.
template <phase P>
inline void* phase_wrapper_w(NrnThread* nt, UserParams& userParams, bool in_memory_transfer) {
    int i = nt->id;
    if (i < userParams.ngroup) {
        if (!in_memory_transfer) {
            const char* data_dir = userParams.path;
            // directory to read could be different for phase 2 if we are restoring
            // all other phases still read from dataset directory because the data
            // is constant
            if (P == 2) {
                data_dir = userParams.restore_path;
            }

            std::string fname = std::string(data_dir) + "/" +
                                std::to_string(userParams.gidgroups[i]) + "_" + getPhaseName<P>() +
                                ".dat";

            // Avoid trying to open the gid_gap.dat file if it doesn't exist when there are no
            // gap junctions in this gid.
            // Note that we still need to close `userParams.file_reader[i]`
            // because files are opened in the order of `gid_1.dat`, `gid_2.dat` and `gid_gap.dat`.
            // When we open next file, `gid_gap.dat` in this case, we are supposed to close the
            // handle for `gid_2.dat` even though file doesn't exist.
            if (P == gap && !FileHandler::file_exist(fname)) {
                userParams.file_reader[i].close();
            } else {
                // if no file failed to open or not opened at all
                userParams.file_reader[i].open(fname);
            }
        }
        read_phase_aux<P>(*nt, userParams);
        if (!in_memory_transfer) {
            userParams.file_reader[i].close();
        }
        if (P == 2) {
            setup_ThreadData(*nt);
        }
    }
    return nullptr;
}

/// Specific phase reading executed by threads.
template <phase P>
inline static void phase_wrapper(UserParams& userParams, int direct = 0) {
    nrn_multithread_job(phase_wrapper_w<P>, userParams, direct != 0);
}
}  // namespace coreneuron
}  // namespace coreneuron
