#include "coreneuron/nrnconf.h"
#include "coreneuron/nrnoc/multicore.h"
#include "coreneuron/nrnoc/nrnoc_decl.h"
#include "coreneuron/nrnmpi/nrnmpi.h"
#include "coreneuron/nrniv/partrans.h"

// This is the computational code for gap junction simulation.
// The setup code is in partrans_setup.cpp
// assert that all HalfGaps are of the same type
// assert that every HalfGap instance in the thread have been a
// ParallelContext.target(&HalfGap.vpre, sid)
namespace coreneuron {
int nrn_have_gaps;

using namespace nrn_partrans;

HalfGap_Info* nrn_partrans::halfgap_info;
TransferThreadData* nrn_partrans::transfer_thread_data_;

// TODO: Where should those go?
// MPI_Alltoallv buffer info
double* nrn_partrans::insrc_buf_;   // Receive buffer for gap voltages
double* nrn_partrans::outsrc_buf_;  // Send buffer for gap voltages
int* nrn_partrans::insrccnt_;
int* nrn_partrans::insrcdspl_;
int* nrn_partrans::outsrccnt_;
int* nrn_partrans::outsrcdspl_;

void nrnmpi_v_transfer() {
    // copy HalfGap source voltages to outsrc_buf_
    // note that same voltage may get copied to several locations in outsrc_buf

    // gather the source values. can be done in parallel
    for (int tid = 0; tid < nrn_nthread; ++tid) {
        TransferThreadData& ttd = transfer_thread_data_[tid];
        NrnThread& nt = nrn_threads[tid];
        int n = ttd.nsrc;
        if (n == 0) {
            continue;
        }
        double* vdata = nt._actual_v;
        int* v_indices = ttd.v_indices;

#undef METHOD
#define METHOD 2

#if METHOD == 1

// copy voltages to cpu and cpu gathers/scatters to outsrc_buf
// clang-format off
        #pragma acc update host(vdata[0 : nt.end]) if (nt.compute_gpu)
        // clang-format on
        int* outbuf_indices = ttd.outbuf_indices;
        for (int i = 0; i < n; ++i) {
            outsrc_buf_[outbuf_indices[i]] = vdata[v_indices[i]];
        }
    }

#elif METHOD == 2
        // gather voltages on gpu and copy to cpu, cpu scatters to outsrc_buf
        double* vg = ttd.v_gather;
// clang-format off
        #pragma acc parallel loop present(          \
            v_indices[0:n], vdata[0:nt.end],        \
            vg[0 : n]) /*copyout(vg[0:n])*/         \
            if (nt.compute_gpu) async(nt.stream_id)
        for (int i = 0; i < n; ++i) {
            vg[i] = vdata[v_indices[i]];
        }
        // do not know why the copyout above did not work
        // and the following update is needed
        #pragma acc update host(vg[0 : n])          \
            if (nrn_threads[0].compute_gpu)         \
            async(nt.stream_id)
        // clang-format on
    }

    // copy source values to outsrc_buf_
    for (int tid = 0; tid < nrn_nthread; ++tid) {
// clang-format off
        #pragma acc wait(nrn_threads[tid].stream_id)
        // clang-format on
        TransferThreadData& ttd = transfer_thread_data_[tid];
        int n = ttd.nsrc;
        if (n == 0) {
            continue;
        }
        int* outbuf_indices = ttd.outbuf_indices;
        double* vg = ttd.v_gather;
        for (int i = 0; i < n; ++i) {
            outsrc_buf_[outbuf_indices[i]] = vg[i];
        }
    }

#endif /* METHOD == 2 */

// transfer
#if NRNMPI
    if (nrnmpi_numprocs > 1) {  // otherwise insrc_buf_ == outsrc_buf_
        nrnmpi_barrier();
        nrnmpi_dbl_alltoallv(outsrc_buf_, outsrccnt_, outsrcdspl_, insrc_buf_, insrccnt_,
                             insrcdspl_);
    } else
#endif
    {  // actually use the multiprocess code even for one process to aid debugging
        for (int i = 0; i < outsrcdspl_[1]; ++i) {
            insrc_buf_[i] = outsrc_buf_[i];
        }
    }

// insrc_buf_ will get copied to targets via nrnthread_v_transfer
// clang-format off
    #pragma acc update device(                      \
        insrc_buf_[0:insrcdspl_[nrnmpi_numprocs]])  \
        if (nrn_threads[0].compute_gpu)
    // clang-format on
}

void nrnthread_v_transfer(NrnThread* _nt) {
    TransferThreadData& ttd = transfer_thread_data_[_nt->id];
    if (!ttd.halfgap_ml) {
        return;
    }
    int _cntml_actual = ttd.halfgap_ml->nodecount;
    double* vpre = ttd.halfgap_ml->data;
    int* insrc_indices = ttd.insrc_indices;

    if (halfgap_info->layout == 1) { /* AoS */
        int ix_vpre = halfgap_info->ix_vpre;
        int sz = halfgap_info->sz;
        vpre += ix_vpre;
        for (int _iml = 0; _iml < _cntml_actual; ++_iml) {
            vpre[_iml * sz] = insrc_buf_[insrc_indices[_iml]];
        }
    } else { /* SoA */
        int _cntml_padded = ttd.halfgap_ml->_nodecount_padded;
        int ix_vpre = halfgap_info->ix_vpre * _cntml_padded;
        vpre += ix_vpre;
// clang-format off
        #pragma acc parallel loop present(              \
            insrc_indices[0:_cntml_actual],             \
            vpre[0:_cntml_actual],                      \
            insrc_buf_[0:insrcdspl_[nrnmpi_numprocs]])  \
            if (_nt->compute_gpu)                       \
            async(_nt->stream_id)
        // clang-format on
        for (int _iml = 0; _iml < _cntml_actual; ++_iml) {
            vpre[_iml] = insrc_buf_[insrc_indices[_iml]];
        }
    }
}

void nrn_partrans::gap_update_indices() {
    if (insrcdspl_) {
// clang-format off
        #pragma acc enter data create(                  \
            insrc_buf_[0:insrcdspl_[nrnmpi_numprocs]])  \
            if (nrn_threads[0].compute_gpu)
        // clang-format on
    }
    for (int tid = 0; tid < nrn_nthread; ++tid) {
        TransferThreadData& ttd = transfer_thread_data_[tid];

#if METHOD == 2
        int n = ttd.nsrc;
        if (n) {
// clang-format off
            #pragma acc enter data copyin(ttd.v_indices[0 : n]) if (nrn_threads[0].compute_gpu)
            #pragma acc enter data create(ttd.v_gather[0 : n]) if (nrn_threads[0].compute_gpu)
            // clang-format on
        }
#endif /* METHOD == 2 */

        if (ttd.halfgap_ml) {
// clang-format off
            #pragma acc enter data copyin(ttd.insrc_indices[0 : ttd.ntar]) if (nrn_threads[0].compute_gpu)
            // clang-format on
        }
    }
}
}  // namespace coreneuron
