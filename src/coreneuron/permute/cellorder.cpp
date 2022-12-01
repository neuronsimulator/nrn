/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include "coreneuron/nrnconf.h"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/utils/nrn_assert.h"
#include "coreneuron/permute/cellorder.hpp"
#include "coreneuron/network/tnode.hpp"
#include "coreneuron/utils/lpt.hpp"
#include "coreneuron/utils/memory.h"
#include "coreneuron/utils/offload.hpp"
#include "coreneuron/apps/corenrn_parameters.hpp"

#include "coreneuron/permute/node_permute.h"  // for print_quality

#ifdef _OPENACC
#include <openacc.h>
#endif

#include <set>

namespace coreneuron {
int interleave_permute_type;
InterleaveInfo* interleave_info;  // nrn_nthread array


void InterleaveInfo::swap(InterleaveInfo& info) {
    std::swap(nwarp, info.nwarp);
    std::swap(nstride, info.nstride);

    std::swap(stridedispl, info.stridedispl);
    std::swap(stride, info.stride);
    std::swap(firstnode, info.firstnode);
    std::swap(lastnode, info.lastnode);
    std::swap(cellsize, info.cellsize);

    std::swap(nnode, info.nnode);
    std::swap(ncycle, info.ncycle);
    std::swap(idle, info.idle);
    std::swap(cache_access, info.cache_access);
    std::swap(child_race, info.child_race);
}

InterleaveInfo::InterleaveInfo(const InterleaveInfo& info) {
    nwarp = info.nwarp;
    nstride = info.nstride;

    copy_align_array(stridedispl, info.stridedispl, nwarp + 1);
    copy_align_array(stride, info.stride, nstride);
    copy_align_array(firstnode, info.firstnode, nwarp + 1);
    copy_align_array(lastnode, info.lastnode, nwarp + 1);
    copy_align_array(cellsize, info.cellsize, nwarp);

    copy_array(nnode, info.nnode, nwarp);
    copy_array(ncycle, info.ncycle, nwarp);
    copy_array(idle, info.idle, nwarp);
    copy_array(cache_access, info.cache_access, nwarp);
    copy_array(child_race, info.child_race, nwarp);
}

InterleaveInfo& InterleaveInfo::operator=(const InterleaveInfo& info) {
    // self assignment
    if (this == &info)
        return *this;

    InterleaveInfo temp(info);

    this->swap(temp);
    return *this;
}

InterleaveInfo::~InterleaveInfo() {
    if (stride) {
        free_memory(stride);
        free_memory(firstnode);
        free_memory(lastnode);
        free_memory(cellsize);
    }
    if (stridedispl) {
        free_memory(stridedispl);
    }
    if (idle) {
        delete[] nnode;
        delete[] ncycle;
        delete[] idle;
        delete[] cache_access;
        delete[] child_race;
    }
}

void create_interleave_info() {
    destroy_interleave_info();
    interleave_info = new InterleaveInfo[nrn_nthread];
}

void destroy_interleave_info() {
    if (interleave_info) {
        delete[] interleave_info;
        interleave_info = nullptr;
    }
}

// more precise visualization of the warp quality
// can be called after admin2
static void print_quality2(int iwarp, InterleaveInfo& ii, int* p) {
    int pc = (iwarp == 0);  // print warp 0
    pc = 0;                 // turn off printing
    int nodebegin = ii.lastnode[iwarp];
    int* stride = ii.stride + ii.stridedispl[iwarp];
    int ncycle = ii.cellsize[iwarp];

    int inode = nodebegin;

    size_t nn = 0;  // number of nodes in warp. '.'
    size_t nx = 0;  // number of idle cores on all cycles. 'X'
    size_t ncacheline = 0;
    ;                // number of parent memory cacheline accesses.
                     //   assmue warpsize is max number in a cachline so all o
    size_t ncr = 0;  // number of child race. nchild-1 of same parent in same cycle

    for (int icycle = 0; icycle < ncycle; ++icycle) {
        int s = stride[icycle];
        int lastp = -2;
        if (pc)
            printf("  ");
        std::set<int> crace;  // how many children have same parent in a cycle
        for (int icore = 0; icore < warpsize; ++icore) {
            char ch = '.';
            if (icore < s) {
                int par = p[inode];
                if (crace.find(par) != crace.end()) {
                    ch = 'r';
                    ++ncr;
                } else {
                    crace.insert(par);
                }

                if (par != lastp + 1) {
                    ch = (ch == 'r') ? 'R' : 'o';
                    ++ncacheline;
                }
                lastp = p[inode++];
                ++nn;
            } else {
                ch = 'X';
                ++nx;
            }
            if (pc)
                printf("%c", ch);
        }
        if (pc)
            printf("\n");
    }

    ii.nnode[iwarp] = nn;
    ii.ncycle[iwarp] = size_t(ncycle);
    ii.idle[iwarp] = nx;
    ii.cache_access[iwarp] = ncacheline;
    ii.child_race[iwarp] = ncr;
    if (pc)
        printf("warp %d:  %ld nodes, %d cycles, %ld idle, %ld cache access, %ld child races\n",
               iwarp,
               nn,
               ncycle,
               nx,
               ncacheline,
               ncr);
}

static void print_quality1(int iwarp, InterleaveInfo& ii, int ncell, int* p) {
    int pc = ((iwarp == 0) || iwarp == (ii.nwarp - 1));  // warp not to skip printing
    pc = 0;                                              // turn off printing.
    int* stride = ii.stride;
    int cellbegin = iwarp * warpsize;
    int cellend = cellbegin + warpsize;
    cellend = (cellend < stride[0]) ? cellend : stride[0];

    int ncycle = 0;
    for (int i = cellbegin; i < cellend; ++i) {
        if (ncycle < ii.cellsize[i]) {
            ncycle = ii.cellsize[i];
        }
    }
    nrn_assert(ncycle == ii.cellsize[cellend - 1]);
    nrn_assert(ncycle <= ii.nstride);

    int ncell_in_warp = cellend - cellbegin;

    size_t n = 0;   // number of nodes in warp (not including roots)
    size_t nx = 0;  // number of idle cores on all cycles. X
    size_t ncacheline = 0;
    ;  // number of parent memory cacheline accesses.
       // assume warpsize is max number in a cachline so
       // first core has all o

    int inode = ii.firstnode[cellbegin];
    for (int icycle = 0; icycle < ncycle; ++icycle) {
        int sbegin = ncell - stride[icycle] - cellbegin;
        int lastp = -2;
        if (pc)
            printf("  ");
        for (int icore = 0; icore < warpsize; ++icore) {
            char ch = '.';
            if (icore < ncell_in_warp && icore >= sbegin) {
                int par = p[inode + icore];
                if (par != lastp + 1) {
                    ch = 'o';
                    ++ncacheline;
                }
                lastp = par;
                ++n;
            } else {
                ch = 'X';
                ++nx;
            }
            if (pc)
                printf("%c", ch);
        }
        if (pc)
            printf("\n");
        inode += ii.stride[icycle + 1];
    }

    ii.nnode[iwarp] = n;
    ii.ncycle[iwarp] = (size_t) ncycle;
    ii.idle[iwarp] = nx;
    ii.cache_access[iwarp] = ncacheline;
    ii.child_race[iwarp] = 0;
    if (pc)
        printf("warp %d:  %ld nodes, %d cycles, %ld idle, %ld cache access\n",
               iwarp,
               n,
               ncycle,
               nx,
               ncacheline);
}

static void warp_balance(int ith, InterleaveInfo& ii) {
    size_t nwarp = size_t(ii.nwarp);
    size_t smm[4][3];  // sum_min_max see cp below
    for (size_t j = 0; j < 4; ++j) {
        smm[j][0] = 0;
        smm[j][1] = 1000000000;
        smm[j][2] = 0;
    }
    double emax = 0.0, emin = 1.0;
    for (size_t i = 0; i < nwarp; ++i) {
        size_t n = ii.nnode[i];
        double e = double(n) / (n + ii.idle[i]);
        if (emax < e) {
            emax = e;
        }
        if (emin > e) {
            emin = e;
        }
        size_t s[4] = {n, ii.idle[i], ii.cache_access[i], ii.child_race[i]};
        for (size_t j = 0; j < 4; ++j) {
            smm[j][0] += s[j];
            if (smm[j][1] > s[j]) {
                smm[j][1] = s[j];
            }
            if (smm[j][2] < s[j]) {
                smm[j][2] = s[j];
            }
        }
    }
    std::vector<size_t> v(nwarp);
    for (size_t i = 0; i < nwarp; ++i) {
        v[i] = ii.ncycle[i];
    }
    double bal = load_balance(v);
#ifdef DEBUG
    printf(
        "thread %d nwarp=%ld  balance=%g  warp_efficiency %g to %g\n", ith, nwarp, bal, emin, emax);
    const char* cp[4] = {"nodes", "idle", "ca", "cr"};
    for (size_t i = 0; i < 4; ++i) {
        printf("  %s=%ld (%ld:%ld)", cp[i], smm[i][0], smm[i][1], smm[i][2]);
    }
    printf("\n");
#else
    (void) bal;  // Remove warning about unused
#endif
}

int* interleave_order(int ith, int ncell, int nnode, int* parent) {
    // return if there are no nodes to permute
    if (nnode <= 0)
        return nullptr;

    // ensure parent of root = -1
    for (int i = 0; i < ncell; ++i) {
        if (parent[i] == 0) {
            parent[i] = -1;
        }
    }

    int nwarp = 0, nstride = 0, *stride = nullptr, *firstnode = nullptr;
    int *lastnode = nullptr, *cellsize = nullptr, *stridedispl = nullptr;

    int* order = node_order(
        ncell, nnode, parent, nwarp, nstride, stride, firstnode, lastnode, cellsize, stridedispl);

    if (interleave_info) {
        InterleaveInfo& ii = interleave_info[ith];
        ii.nwarp = nwarp;
        ii.nstride = nstride;
        ii.stridedispl = stridedispl;
        ii.stride = stride;
        ii.firstnode = firstnode;
        ii.lastnode = lastnode;
        ii.cellsize = cellsize;
        if (0 && ith == 0 && interleave_permute_type == 1) {
            printf("ith=%d nstride=%d ncell=%d nnode=%d\n", ith, nstride, ncell, nnode);
            for (int i = 0; i < ncell; ++i) {
                printf("icell=%d cellsize=%d first=%d last=%d\n",
                       i,
                       cellsize[i],
                       firstnode[i],
                       lastnode[i]);
            }
            for (int i = 0; i < nstride; ++i) {
                printf("istride=%d stride=%d\n", i, stride[i]);
            }
        }
        if (ith == 0) {
            // needed for print_quality[12] and done once here to save time
            int* p = new int[nnode];
            for (int i = 0; i < nnode; ++i) {
                p[i] = parent[i];
            }
            permute_ptr(p, nnode, order);
            node_permute(p, nnode, order);

            ii.nnode = new size_t[nwarp];
            ii.ncycle = new size_t[nwarp];
            ii.idle = new size_t[nwarp];
            ii.cache_access = new size_t[nwarp];
            ii.child_race = new size_t[nwarp];
            for (int i = 0; i < nwarp; ++i) {
                if (interleave_permute_type == 1) {
                    print_quality1(i, interleave_info[ith], ncell, p);
                }
                if (interleave_permute_type == 2) {
                    print_quality2(i, interleave_info[ith], p);
                }
            }
            delete[] p;
            warp_balance(ith, interleave_info[ith]);
        }
    }

    return order;
}

#if INTERLEAVE_DEBUG  // only the cell per core style
static int** cell_indices_debug(NrnThread& nt, InterleaveInfo& ii) {
    int ncell = nt.ncell;
    int nnode = nt.end;
    int* parents = nt._v_parent_index;

    // we expect the nodes to be interleave ordered with smallest cell first
    // establish consistency with ii.
    // first ncell parents are -1
    for (int i = 0; i < ncell; ++i) {
        nrn_assert(parents[i] == -1);
    }
    int* sz = new int[ncell];
    int* cell = new int[nnode];
    for (int i = 0; i < ncell; ++i) {
        sz[i] = 0;
        cell[i] = i;
    }
    for (int i = ncell; i < nnode; ++i) {
        cell[i] = cell[parents[i]];
        sz[cell[i]] += 1;
    }

    // cells are in inceasing sz order;
    for (int i = 1; i < ncell; ++i) {
        nrn_assert(sz[i - 1] <= sz[i]);
    }
    // same as ii.cellsize
    for (int i = 0; i < ncell; ++i) {
        nrn_assert(sz[i] == ii.cellsize[i]);
    }

    int** cellindices = new int*[ncell];
    for (int i = 0; i < ncell; ++i) {
        cellindices[i] = new int[sz[i]];
        sz[i] = 0;  // restart sz counts
    }
    for (int i = ncell; i < nnode; ++i) {
        cellindices[cell[i]][sz[cell[i]]] = i;
        sz[cell[i]] += 1;
    }
    // cellindices first and last same as ii first and last
    for (int i = 0; i < ncell; ++i) {
        nrn_assert(cellindices[i][0] == ii.firstnode[i]);
        nrn_assert(cellindices[i][sz[i] - 1] == ii.lastnode[i]);
    }

    delete[] sz;
    delete[] cell;

    return cellindices;
}

static int*** cell_indices_threads;
void mk_cell_indices() {
    cell_indices_threads = new int**[nrn_nthread];
    for (int i = 0; i < nrn_nthread; ++i) {
        NrnThread& nt = nrn_threads[i];
        if (nt.ncell) {
            cell_indices_threads[i] = cell_indices_debug(nt, interleave_info[i]);
        } else {
            cell_indices_threads[i] = nullptr;
        }
    }
}
#endif  // INTERLEAVE_DEBUG

#define GPU_V(i)      nt->_actual_v[i]
#define GPU_A(i)      nt->_actual_a[i]
#define GPU_B(i)      nt->_actual_b[i]
#define GPU_D(i)      nt->_actual_d[i]
#define GPU_RHS(i)    nt->_actual_rhs[i]
#define GPU_PARENT(i) nt->_v_parent_index[i]

// How does the interleaved permutation with stride get used in
// triagularization?

// each cell in parallel regardless of inhomogeneous topology
static void triang_interleaved(NrnThread* nt,
                               int icell,
                               int icellsize,
                               int nstride,
                               int* stride,
                               int* lastnode) {
    int i = lastnode[icell];
    for (int istride = nstride - 1; istride >= 0; --istride) {
        if (istride < icellsize) {  // only first icellsize strides matter
            // what is the index
            int ip = GPU_PARENT(i);
#ifndef CORENEURON_ENABLE_GPU
            nrn_assert(ip >= 0);  // if (ip < 0) return;
#endif
            double p = GPU_A(i) / GPU_D(i);
            GPU_D(ip) -= p * GPU_B(i);
            GPU_RHS(ip) -= p * GPU_RHS(i);
            i -= stride[istride];
        }
    }
}

// back substitution?
static void bksub_interleaved(NrnThread* nt,
                              int icell,
                              int icellsize,
                              int /* nstride */,
                              int* stride,
                              int* firstnode) {
    int i = firstnode[icell];
    GPU_RHS(icell) /= GPU_D(icell);  // the root
    for (int istride = 0; istride < icellsize; ++istride) {
        int ip = GPU_PARENT(i);
#ifndef CORENEURON_ENABLE_GPU
        nrn_assert(ip >= 0);
#endif
        GPU_RHS(i) -= GPU_B(i) * GPU_RHS(ip);
        GPU_RHS(i) /= GPU_D(i);
        i += stride[istride + 1];
    }
}

// icore ranges [0:warpsize) ; stride[ncycle]
nrn_pragma_acc(routine vector)
static void triang_interleaved2(NrnThread* nt, int icore, int ncycle, int* stride, int lastnode) {
    int icycle = ncycle - 1;
    int istride = stride[icycle];
    int i = lastnode - istride + icore;
    int ii = i;

    // execute until all tree depths are executed
    bool has_subtrees_to_compute = true;

    // clang-format off
    nrn_pragma_acc(loop seq)
    for (; has_subtrees_to_compute; ) {  // ncycle loop
        // serial test, gpu does this in parallel
        nrn_pragma_acc(loop vector)
        nrn_pragma_omp(loop bind(parallel))
        for (int icore = 0; icore < warpsize; ++icore) {
            int i = ii + icore;
            if (icore < istride) {  // most efficient if istride equal  warpsize
                // what is the index
                int ip = GPU_PARENT(i);
                double p = GPU_A(i) / GPU_D(i);
                nrn_pragma_acc(atomic update)
                nrn_pragma_omp(atomic update)
                GPU_D(ip) -= p * GPU_B(i);
                nrn_pragma_acc(atomic update)
                nrn_pragma_omp(atomic update)
                GPU_RHS(ip) -= p * GPU_RHS(i);
            }
        }
        // if finished with all tree depths then ready to break
        // (note that break is not allowed in OpenACC)
        if (icycle == 0) {
            has_subtrees_to_compute = false;
            continue;
        }
        --icycle;
        istride = stride[icycle];
        i -= istride;
        ii -= istride;
    }
}

// icore ranges [0:warpsize) ; stride[ncycle]
nrn_pragma_acc(routine vector)
static void bksub_interleaved2(NrnThread* nt,
                               int root,
                               int lastroot,
                               int icore,
                               int ncycle,
                               int* stride,
                               int firstnode) {
    nrn_pragma_acc(loop seq)
    for (int i = root; i < lastroot; i += 1) {
        GPU_RHS(i) /= GPU_D(i);  // the root
    }

    int i = firstnode + icore;
    int ii = i;
    nrn_pragma_acc(loop seq)
    for (int icycle = 0; icycle < ncycle; ++icycle) {
        int istride = stride[icycle];
        // serial test, gpu does this in parallel
        nrn_pragma_acc(loop vector)
        nrn_pragma_omp(loop bind(parallel))
        for (int icore = 0; icore < warpsize; ++icore) {
            int i = ii + icore;
            if (icore < istride) {
                int ip = GPU_PARENT(i);
                GPU_RHS(i) -= GPU_B(i) * GPU_RHS(ip);
                GPU_RHS(i) /= GPU_D(i);
            }
            i += istride;
        }
        ii += istride;
    }
}

/**
 * \brief Solve Hines matrices/cells with compartment-based granularity.
 *
 * The node ordering/permuation guarantees cell interleaving (as much coalesced memory access as
 * possible) and balanced warps (through the use of lpt algorithm to define the groups/warps). Every
 * warp deals with a group of cells, therefore multiple compartments (finer level of parallelism).
 */
void solve_interleaved2(int ith) {
    NrnThread* nt = nrn_threads + ith;
    InterleaveInfo& ii = interleave_info[ith];
    int nwarp = ii.nwarp;
    if (nwarp == 0)
        return;

    int ncore = nwarp * warpsize;

#ifdef _OPENACC
    if (corenrn_param.gpu && corenrn_param.cuda_interface) {
        auto* d_nt = static_cast<NrnThread*>(acc_deviceptr(nt));
        auto* d_info = static_cast<InterleaveInfo*>(acc_deviceptr(interleave_info + ith));
        solve_interleaved2_launcher(d_nt, d_info, ncore, acc_get_cuda_stream(nt->stream_id));
    } else {
#endif
        int* ncycles = ii.cellsize;         // nwarp of these
        int* stridedispl = ii.stridedispl;  // nwarp+1 of these
        int* strides = ii.stride;           // sum ncycles of these (bad since ncompart/warpsize)
        int* rootbegin = ii.firstnode;      // nwarp+1 of these
        int* nodebegin = ii.lastnode;       // nwarp+1 of these
#if defined(CORENEURON_ENABLE_GPU) && !defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && \
    defined(_OPENACC)
        int nstride = stridedispl[nwarp];
#endif
        /* If we compare this loop with the one from cellorder.cu (CUDA version), we will understand 
         * that the parallelism here is exposed in steps, while in the CUDA version all the parallelism 
         * is exposed from the very beginning of the loop. In more details, here we initially distribute
         * the outermost loop, e.g. in the CUDA blocks, and for the innermost loops we explicitly use multiple
         * threads for the parallelization (see for example the loop directives in triang/bksub_interleaved2). 
         * On the other hand, in the CUDA version the outermost loop is distributed to all the available threads,
         * and therefore there is no need to have the innermost loops. Here, the loop/icore jumps every warpsize,
         * while in the CUDA version the icore increases by one. Other than this, the two loop versions
         * are equivalent (same results).
         */
        nrn_pragma_acc(parallel loop gang present(nt [0:1],
                              strides [0:nstride],
                              ncycles [0:nwarp],
                              stridedispl [0:nwarp + 1],
                              rootbegin [0:nwarp + 1],
                              nodebegin [0:nwarp + 1]) if (nt->compute_gpu) async(nt->stream_id))
        nrn_pragma_omp(target teams loop if(nt->compute_gpu))
        for (int icore = 0; icore < ncore; icore += warpsize) {
            int iwarp = icore / warpsize;     // figure out the >> value
            int ic = icore & (warpsize - 1);  // figure out the & mask
            int ncycle = ncycles[iwarp];
            int* stride = strides + stridedispl[iwarp];
            int root = rootbegin[iwarp];  // cell ID -> [0, ncell)
            int lastroot = rootbegin[iwarp + 1];
            int firstnode = nodebegin[iwarp];
            int lastnode = nodebegin[iwarp + 1];
            
            triang_interleaved2(nt, ic, ncycle, stride, lastnode);
            bksub_interleaved2(nt, root + ic, lastroot, ic, ncycle, stride, firstnode);
        }
        nrn_pragma_acc(wait(nt->stream_id))
#ifdef _OPENACC
    }
#endif
}

/**
 * \brief Solve Hines matrices/cells with cell-based granularity.
 *
 * The node ordering guarantees cell interleaving (as much coalesced memory access as possible),
 * but parallelism granularity is limited to a per cell basis. Therefore every execution stream
 * is mapped to a cell/tree.
 */
void solve_interleaved1(int ith) {
    NrnThread* nt = nrn_threads + ith;
    int ncell = nt->ncell;
    if (ncell == 0) {
        return;
    }
    InterleaveInfo& ii = interleave_info[ith];
    int nstride = ii.nstride;
    int* stride = ii.stride;
    int* firstnode = ii.firstnode;
    int* lastnode = ii.lastnode;
    int* cellsize = ii.cellsize;

    // OL211123: can we preserve the error checking behaviour of OpenACC's
    // present clause with OpenMP? It is a bug if these data are not present,
    // so diagnostics are helpful...
    nrn_pragma_acc(parallel loop present(nt [0:1],
                                         stride [0:nstride],
                                         firstnode [0:ncell],
                                         lastnode [0:ncell],
                                         cellsize [0:ncell]) if (nt->compute_gpu)
                       async(nt->stream_id))
    nrn_pragma_omp(target teams distribute parallel for simd if(nt->compute_gpu))
    for (int icell = 0; icell < ncell; ++icell) {
        int icellsize = cellsize[icell];
        triang_interleaved(nt, icell, icellsize, nstride, stride, lastnode);
        bksub_interleaved(nt, icell, icellsize, nstride, stride, firstnode);
    }
    nrn_pragma_acc(wait(nt->stream_id))
}

void solve_interleaved(int ith) {
    if (interleave_permute_type != 1) {
        solve_interleaved2(ith);
    } else {
        solve_interleaved1(ith);
    }
}
}  // namespace coreneuron
