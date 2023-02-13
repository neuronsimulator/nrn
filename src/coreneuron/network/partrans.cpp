/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
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
        for (std::size_t i = 0; i < n_src_gather; ++i) {
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
    static_cast<void>(compute_gpu);

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
    nrn_pragma_acc(parallel loop copyin(tar_indices [0:ntar])
                       present(insrc_indices [0:ntar],
                               tar_data [0:ndata],
                               insrc_buf_ [0:n_insrc_buf]) if (_nt->compute_gpu)
                           async(_nt->stream_id))
    nrn_pragma_omp(target teams distribute parallel for simd map(to: tar_indices[0:ntar]) if(_nt->compute_gpu))
    for (size_t i = 0; i < ntar; ++i) {
        tar_data[tar_indices[i]] = insrc_buf_[insrc_indices[i]];
    }
}

void nrn_partrans::copy_gap_indices_to_device() {
    // Ensure index vectors, src_gather, and insrc_buf_ are on the gpu.
    if (insrcdspl_) {
        // TODO: we don't actually need to copy here, just allocate + associate
        // storage on the device
        cnrn_target_copyin(insrc_buf_, insrcdspl_[nrnmpi_numprocs]);
    }
    for (int tid = 0; tid < nrn_nthread; ++tid) {
        const NrnThread* nt = nrn_threads + tid;
        if (!nt->compute_gpu) {
            continue;
        }

        const TransferThreadData& ttd = transfer_thread_data_[tid];

        if (!ttd.src_indices.empty()) {
            cnrn_target_copyin(ttd.src_indices.data(), ttd.src_indices.size());
            // TODO: we don't actually need to copy here, just allocate +
            // associate storage on the device.
            cnrn_target_copyin(ttd.src_gather.data(), ttd.src_gather.size());
        }

        if (ttd.insrc_indices.size()) {
            cnrn_target_copyin(ttd.insrc_indices.data(), ttd.insrc_indices.size());
        }
    }
}

void nrn_partrans::delete_gap_indices_from_device() {
    if (insrcdspl_) {
        int n_insrc_buf = insrcdspl_[nrnmpi_numprocs];
        cnrn_target_delete(insrc_buf_, n_insrc_buf);
    }
    for (int tid = 0; tid < nrn_nthread; ++tid) {
        const NrnThread* nt = nrn_threads + tid;
        if (!nt->compute_gpu) {
            continue;
        }

        TransferThreadData& ttd = transfer_thread_data_[tid];

        if (!ttd.src_indices.empty()) {
            cnrn_target_delete(ttd.src_indices.data(), ttd.src_indices.size());
            cnrn_target_delete(ttd.src_gather.data(), ttd.src_gather.size());
        }

        if (!ttd.insrc_indices.empty()) {
            cnrn_target_delete(ttd.insrc_indices.data(), ttd.insrc_indices.size());
        }
    }
}
}  // namespace coreneuron
