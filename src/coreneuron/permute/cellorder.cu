/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include "coreneuron/utils/utils_cuda.h"
#include "coreneuron/permute/cellorder.hpp"
#include "coreneuron/network/tnode.hpp"
#include "coreneuron/sim/multicore.hpp"

namespace coreneuron {

__device__ void triang_interleaved2_device(NrnThread* nt,
                                           int icore,
                                           int ncycle,
                                           int* stride,
                                           int lastnode) {
    int icycle = ncycle - 1;
    int istride = stride[icycle];
    int i = lastnode - istride + icore;

    int ip;
    double p;
    while (icycle >= 0) {
        // most efficient if istride equal warpsize, else branch divergence!
        if (icore < istride) {
            ip = nt->_v_parent_index[i];
            p = nt->_actual_a[i] / nt->_actual_d[i];
            atomicAdd(&nt->_actual_d[ip], -p * nt->_actual_b[i]);
            atomicAdd(&nt->_actual_rhs[ip], -p * nt->_actual_rhs[i]);
        }
        --icycle;
        istride = stride[icycle];
        i -= istride;
    }
}

__device__ void bksub_interleaved2_device(NrnThread* nt,
                                          int root,
                                          int lastroot,
                                          int icore,
                                          int ncycle,
                                          int* stride,
                                          int firstnode) {
    for (int i = root; i < lastroot; i += warpsize) {
        nt->_actual_rhs[i] /= nt->_actual_d[i];  // the root
    }

    int i = firstnode + icore;

    int ip;
    for (int icycle = 0; icycle < ncycle; ++icycle) {
        int istride = stride[icycle];
        if (icore < istride) {
            ip = nt->_v_parent_index[i];
            nt->_actual_rhs[i] -= nt->_actual_b[i] * nt->_actual_rhs[ip];
            nt->_actual_rhs[i] /= nt->_actual_d[i];
        }
        i += istride;
    }
}

__global__ void solve_interleaved2_kernel(NrnThread* nt, InterleaveInfo* ii, int ncore) {
    int icore = blockDim.x * blockIdx.x + threadIdx.x;

    int* ncycles = ii->cellsize;         // nwarp of these
    int* stridedispl = ii->stridedispl;  // nwarp+1 of these
    int* strides = ii->stride;           // sum ncycles of these (bad since ncompart/warpsize)
    int* rootbegin = ii->firstnode;      // nwarp+1 of these
    int* nodebegin = ii->lastnode;       // nwarp+1 of these

    int iwarp = icore / warpsize;     // figure out the >> value
    int ic = icore & (warpsize - 1);  // figure out the & mask
    int ncycle = ncycles[iwarp];
    int* stride = strides + stridedispl[iwarp];
    int root = rootbegin[iwarp];
    int lastroot = rootbegin[iwarp + 1];
    int firstnode = nodebegin[iwarp];
    int lastnode = nodebegin[iwarp + 1];

    triang_interleaved2_device(nt, ic, ncycle, stride, lastnode);
    bksub_interleaved2_device(nt, root + ic, lastroot, ic, ncycle, stride, firstnode);
}

void solve_interleaved2_launcher(NrnThread* nt, InterleaveInfo* info, int ncore, void* stream) {
    auto cuda_stream = static_cast<cudaStream_t>(stream);

    int threadsPerBlock = warpsize;
    // TODO: Should blocksPerGrid be a fixed number and have a while block inside the kernel?
    int blocksPerGrid = (ncore + threadsPerBlock - 1) / threadsPerBlock;

    solve_interleaved2_kernel<<<blocksPerGrid, threadsPerBlock, 0, cuda_stream>>>(nt, info, ncore);

    cudaStreamSynchronize(cuda_stream);

    CHECKLAST("solve_interleaved2_launcher");
}

}  // namespace coreneuron
