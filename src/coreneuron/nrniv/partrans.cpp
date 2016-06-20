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

int nrn_have_gaps;

using namespace nrn_partrans;

HalfGap_Info* nrn_partrans::halfgap_info;
TransferThreadData* nrn_partrans::transfer_thread_data_;

// MPI_Alltoallv buffer info
double* nrn_partrans::insrc_buf_; // Receive buffer for gap voltages
double* nrn_partrans::outsrc_buf_; // Send buffer for gap voltages
int* nrn_partrans::insrccnt_;
int* nrn_partrans::insrcdspl_;
int* nrn_partrans::outsrccnt_;
int* nrn_partrans::outsrcdspl_;


void nrnmpi_v_transfer() {
  // copy HalfGap source voltages to outsrc_buf_
  // note that same voltage may get copied to several locations in outsrc_buf
  for (int tid = 0; tid < nrn_nthread; ++tid) {
    int n = transfer_thread_data_[tid].nsrc;
    if (n == 0) { continue; }
    double* vdata = nrn_threads[tid]._actual_v;
    int* v_indices = transfer_thread_data_[tid].v_indices;
    int* outbuf_indices = transfer_thread_data_[tid].outbuf_indices;
    for (int i=0; i < n; ++i) {
      outsrc_buf_[outbuf_indices[i]] = vdata[v_indices[i]];
    }
  }
  // transfer
  if (nrnmpi_numprocs > 1) { // otherwise insrc_buf_ == outsrc_buf_
    nrnmpi_barrier();
    nrnmpi_dbl_alltoallv(outsrc_buf_, outsrccnt_, outsrcdspl_,
      insrc_buf_, insrccnt_, insrcdspl_);
  } else { // actually use the multiprocess code even for one process to aid debugging
    for (int i=0; i < outsrcdspl_[1]; ++i) {
      insrc_buf_[i] = outsrc_buf_[i];
    }
  }
  // insrc_buf_ will get copied to targets via nrnthread_v_transfer
}

void nrnthread_v_transfer(NrnThread* _nt) {
  TransferThreadData& ttd = transfer_thread_data_[_nt->id];
  if (!ttd.halfgap_ml) { return; }
  int _cntml_actual = ttd.halfgap_ml->nodecount;
  double* vpre = ttd.halfgap_ml->data;
  int* insrc_indices = ttd.insrc_indices;

  if (halfgap_info->layout == 1) { /* AoS */
    int ix_vpre = halfgap_info->ix_vpre;
    int sz = halfgap_info->sz;
    vpre += ix_vpre;
    for (int _iml = 0; _iml < _cntml_actual; ++_iml) {
      vpre[_iml*sz] = insrc_buf_[insrc_indices[_iml]];
    }
  }else{ /* SoA */
    int _cntml_padded = ttd.halfgap_ml->_nodecount_padded;
    int ix_vpre = halfgap_info->ix_vpre * _cntml_padded;
    vpre += ix_vpre;
    for (int _iml = 0; _iml < _cntml_actual; ++_iml) {
      vpre[_iml] = insrc_buf_[insrc_indices[_iml]];
    }
  }
}
