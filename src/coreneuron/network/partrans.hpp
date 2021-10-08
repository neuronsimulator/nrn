/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#pragma once

#include "coreneuron/sim/multicore.hpp"

#ifndef NRNLONGSGID
#define NRNLONGSGID 0
#endif

#if NRNLONGSGID
using sgid_t = int64_t;
#else
using sgid_t = int;
#endif

namespace coreneuron {
struct Memb_list;

extern bool nrn_have_gaps;
extern void nrnmpi_v_transfer();
extern void nrnthread_v_transfer(NrnThread*);

namespace nrn_partrans {

/** The basic problem is to copy sources to targets.
 *  It may be the case that a source gets copied to several targets.
 *  Sources and targets are a set of indices in NrnThread.data.
 *  A copy may be intrathread, interthread, interprocess.
 *  Copies happen every time step so efficiency is desirable.
 *  SetupTransferInfo gives us the source and target (sid, type, index) triples
 *  for a thread and all the global threads define what gets copied where.
 *  Need to process that info into TransferThreadData for each thread and
 *  the interprocessor mpi buffers insrc_buf_ and outsrc_buf transfered with
 *  MPI_Alltoallv, hopefully with a more or less optimal ordering.
 *  The compute strategy is: 1) Each thread copies its NrnThread.data source
 *  items to outsrc_buf_. 2) MPI_Allgatherv transfers outsrc_buf_ to insrc_buf_.
 *  3) Each thread, copies insrc_buf_ values to Nrnthread.data target.
 *
 *  Optimal ordering is probably beyond our reach but a few considerations
 *  may be useful. The typical use is for gap junctions where only voltage
 *  transferred and all instances of the HalfGap Point_process receive a
 *  voltage. Two situations are common. Voltage transfer is sparse and one
 *  to one, i.e many compartments do not have gap junctions, and those that do
 *  have only one. The other situation is that all compartments have gap
 *  junctions (e.g. syncytium of single compartment cells in the heart) and
 *  the voltage needs to be transferred to all neighboring cells (e.g. 6-18
 *  cells can be neighbors to the central cell). So on the target side, it
 *  might be good to copy to the target in target index order from the
 *  input_buf_. And on the source side, it is certainly simple to scatter
 *  to the outbut_buf_ in NrnThread.data order.  Note that one expects a wide
 *  scatter to the outsrc_buf and also a wide scatter within the insrc_buf_.
 **/

/*
 * In partrans.cpp: nrnmpi_v_transfer
 *   Copy NrnThead.data to outsrc_buf_ for all threads via
 *     gpu: gather src_gather[i] = NrnThread._data[src_indices[i]];
 *     gpu to host src_gather
 *     cpu: outsrc_buf_[outsrc_indices[i]] = src_gather[gather2outsrc_indices[i]];
 *
 *   MPI_Allgatherv outsrc_buf_ to insrc_buf_
 *
 *   host to gpu insrc_buf_
 *
 * In partrans.cpp: nrnthread_v_transfer
 *   insrc_buf_ to NrnThread._data via
 *   NrnThread.data[tar_indices[i]] = insrc_buf_[insrc_indices[i]];
 *     where tar_indices depends on layout, type, etc.
 */

struct TransferThreadData {
    std::vector<int> src_indices;            // indices into NrnThread._data
    std::vector<double> src_gather;          // copy of NrnThread._data[src_indices]
    std::vector<int> gather2outsrc_indices;  // ix of src_gather that send into outsrc_indices
    std::vector<int> outsrc_indices;         // ix of outsrc_buf that receive src_gather values

    std::vector<int> insrc_indices;  // insrc_buf_ indices copied to ...
    std::vector<int> tar_indices;    // indices of NrnThread.data.
};
extern TransferThreadData* transfer_thread_data_; /* array for threads */

}  // namespace nrn_partrans
}  // namespace coreneuron

// For direct transfer,
// must be same as corresponding struct SetupTransferInfo in NEURON
struct SetupTransferInfo {
    std::vector<sgid_t> src_sid;
    std::vector<int> src_type;
    std::vector<int> src_index;
    std::vector<sgid_t> tar_sid;
    std::vector<int> tar_type;
    std::vector<int> tar_index;
};

namespace coreneuron {
namespace nrn_partrans {

extern SetupTransferInfo* setup_info_; /* array for threads exists only during setup*/

extern void gap_mpi_setup(int ngroup);
extern void gap_data_indices_setup(NrnThread* nt);
extern void gap_update_indices();
extern void gap_cleanup();

extern double* insrc_buf_;   // Receive buffer for gap voltages
extern double* outsrc_buf_;  // Send buffer for gap voltages
extern int *insrccnt_, *insrcdspl_, *outsrccnt_, *outsrcdspl_;
}  // namespace nrn_partrans
}  // namespace coreneuron
