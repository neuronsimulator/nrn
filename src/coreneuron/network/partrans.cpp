/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include "coreneuron/nrnconf.h"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/mpi/nrnmpi.h"
#include "coreneuron/mpi/core/nrnmpi.hpp"
#include "coreneuron/network/partrans.hpp"
#include "coreneuron/apps/corenrn_parameters.hpp"

// This is the computational code for src->target transfer (e.g. gap junction)
// simulation.
// The setup code is in partrans_setup.cpp

namespace coreneuron {
bool nrn_have_gaps;

using namespace nrn_partrans;

TransferThreadData* nrn_partrans::transfer_thread_data_;

// MPI_Alltoallv buffer info
double* nrn_partrans::insrc_buf_;   // Receive buffer for gap voltages
double* nrn_partrans::outsrc_buf_;  // Send buffer for gap voltages
int* nrn_partrans::insrccnt_;
int* nrn_partrans::insrcdspl_;
int* nrn_partrans::outsrccnt_;
int* nrn_partrans::outsrcdspl_;

void nrnmpi_v_transfer() {
    // copy source values to outsrc_buf_ and mpi transfer to insrc_buf

    // note that same source value (usually voltage) may get copied to
    // several locations in outsrc_buf

    // gather the source values. can be done in parallel
    for (int tid = 0; tid < nrn_nthread; ++tid) {
        auto& ttd = transfer_thread_data_[tid];
        auto* nt = &nrn_threads[tid];
        int n = int(ttd.outsrc_indices.size());
        if (n == 0) {
            continue;
        }
        double* src_data = nt->_data;
        int* src_indices = ttd.src_indices.data();

        // gather sources on gpu and copy to cpu, cpu scatters to outsrc_buf
        double* src_gather = ttd.src_gather.data();
        size_t n_src_gather = ttd.src_gather.size();

        nrn_pragma_acc(parallel loop present(src_indices [0:n_src_gather],
                                             src_data [0:nt->_ndata],
                                             src_gather [0:n_src_gather]) if (nt->compute_gpu)
                           async(nt->stream_id))
        nrn_pragma_omp(target teams distribute parallel for simd if(nt->compute_gpu))
        for (int i = 0; i < n_src_gather; ++i) {
            src_gather[i] = src_data[src_indices[i]];
        }
        nrn_pragma_acc(update host(src_gather [0:n_src_gather]) if (nt->compute_gpu)
                           async(nt->stream_id))
        nrn_pragma_omp(target update from(src_gather [0:n_src_gather]) if (nt->compute_gpu))
    }

    // copy gathered source values to outsrc_buf_
    bool compute_gpu = false;
    for (int tid = 0; tid < nrn_nthread; ++tid) {
        if (nrn_threads[tid].compute_gpu) {
            compute_gpu = true;
            nrn_pragma_acc(wait(nrn_threads[tid].stream_id))
        }
        TransferThreadData& ttd = transfer_thread_data_[tid];
        size_t n_outsrc_indices = ttd.outsrc_indices.size();
        int* outsrc_indices = ttd.outsrc_indices.data();
        double* src_gather = ttd.src_gather.data();
        int* src_gather_indices = ttd.gather2outsrc_indices.data();
        for (size_t i = 0; i < n_outsrc_indices; ++i) {
            outsrc_buf_[outsrc_indices[i]] = src_gather[src_gather_indices[i]];
        }
    }

    // transfer
    int n_insrc_buf = insrcdspl_[nrnmpi_numprocs];
#if NRNMPI
    if (corenrn_param.mpi_enable) {  // otherwise insrc_buf_ == outsrc_buf_
        nrnmpi_barrier();
        nrnmpi_dbl_alltoallv(
            outsrc_buf_, outsrccnt_, outsrcdspl_, insrc_buf_, insrccnt_, insrcdspl_);
    } else
#endif
    {  // Use the multiprocess code even for one process to aid debugging
        // For nrnmpi_numprocs == 1, insrc_buf_ and outsrc_buf_ are same size.
        for (int i = 0; i < n_insrc_buf; ++i) {
            insrc_buf_[i] = outsrc_buf_[i];
        }
    }

    // insrc_buf_ will get copied to targets via nrnthread_v_transfer
    nrn_pragma_acc(update device(insrc_buf_ [0:n_insrc_buf]) if (compute_gpu))
    nrn_pragma_omp(target update to(insrc_buf_ [0:n_insrc_buf]) if (compute_gpu))
}

void nrnthread_v_transfer(NrnThread* _nt) {
    // Copy insrc_buf_ values to the target locations. (An insrc_buf_ value
    // may be copied to several target locations.
    TransferThreadData& ttd = transfer_thread_data_[_nt->id];
    size_t ntar = ttd.tar_indices.size();
    int* tar_indices = ttd.tar_indices.data();
    int* insrc_indices = ttd.insrc_indices.data();
    double* tar_data = _nt->_data;
    // last element in the displacement vector gives total length
#if defined(CORENEURON_ENABLE_GPU) && !defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && \
    defined(_OPENACC)
    int n_insrc_buf = insrcdspl_[nrnmpi_numprocs];
    int ndata = _nt->_ndata;
#endif

    nrn_pragma_acc(parallel loop present(insrc_indices [0:ntar],
                                         tar_data [0:ndata],
                                         insrc_buf_ [0:n_insrc_buf]) if (_nt->compute_gpu)
                       async(_nt->stream_id))
    nrn_pragma_omp(target teams distribute parallel for simd map(to: tar_indices[0:ntar]) if(_nt->compute_gpu))
    for (size_t i = 0; i < ntar; ++i) {
        tar_data[tar_indices[i]] = insrc_buf_[insrc_indices[i]];
    }
}

/// TODO: Corresponding exit data cluase for OpenACC/OpenMP is missing and hence
///       GPU buffers are not freed.
void nrn_partrans::gap_update_indices() {
    // Ensure index vectors, src_gather, and insrc_buf_ are on the gpu.
    if (insrcdspl_) {
        int n_insrc_buf = insrcdspl_[nrnmpi_numprocs];
        nrn_pragma_acc(enter data create(insrc_buf_ [0:n_insrc_buf]) if (corenrn_param.gpu))
        // clang-format off
        nrn_pragma_omp(target enter data map(alloc: insrc_buf_[0:n_insrc_buf])
                                         if(corenrn_param.gpu))
        // clang-format off
    }
    for (int tid = 0; tid < nrn_nthread; ++tid) {
        TransferThreadData& ttd = transfer_thread_data_[tid];

        size_t n_src_indices = ttd.src_indices.size();
        size_t n_src_gather = ttd.src_gather.size();
        NrnThread* nt = nrn_threads + tid;
        if (n_src_indices) {
            int* src_indices = ttd.src_indices.data();
            double* src_gather = ttd.src_gather.data();
            nrn_pragma_acc(enter data copyin(src_indices[0:n_src_indices]) if(nt->compute_gpu))
            nrn_pragma_acc(enter data create(src_gather[0:n_src_gather]) if(nt->compute_gpu))
            // clang-format off
            nrn_pragma_omp(target enter data map(to: src_indices [0:n_src_indices])
                                             map(alloc: src_gather[0:n_src_gather])
                                             if(nt->compute_gpu))
            // clang-format on
        }

        if (ttd.insrc_indices.size()) {
            int* insrc_indices = ttd.insrc_indices.data();
            size_t n_insrc_indices = ttd.insrc_indices.size();
            nrn_pragma_acc(
                enter data copyin(insrc_indices [0:n_insrc_indices]) if (nt->compute_gpu))
            // clang-format off
            nrn_pragma_omp(target enter data map(to: insrc_indices[0:n_insrc_indices])
                                             if(nt->compute_gpu))
            // clang-format on
        }
    }
}
}  // namespace coreneuron
