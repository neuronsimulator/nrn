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

struct SolveInterleaved2MatrixPtrs {
    double* actual_a{};
    double* actual_b{};
    double* actual_d{};
    double* actual_rhs{};
    int* v_parent_index{};
};

__device__ void triang_interleaved2_device(SolveInterleaved2MatrixPtrs mat,
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
        if (icore < istride) {
            ip = mat.v_parent_index[i];
            p = mat.actual_a[i] / mat.actual_d[i];
            atomicAdd(&mat.actual_d[ip], -p * mat.actual_b[i]);
            atomicAdd(&mat.actual_rhs[ip], -p * mat.actual_rhs[i]);
        }
        --icycle;
        istride = stride[icycle];
        i -= istride;
    }
}

__device__ void bksub_interleaved2_device(SolveInterleaved2MatrixPtrs mat,
                                          int root,
                                          int lastroot,
                                          int icore,
                                          int ncycle,
                                          int* stride,
                                          int firstnode) {
    for (int i = root; i < lastroot; i += warpsize) {
        mat.actual_rhs[i] /= mat.actual_d[i];
    }

    int i = firstnode + icore;

    int ip;
    for (int icycle = 0; icycle < ncycle; ++icycle) {
        int istride = stride[icycle];
        if (icore < istride) {
            ip = mat.v_parent_index[i];
            mat.actual_rhs[i] -= mat.actual_b[i] * mat.actual_rhs[ip];
            mat.actual_rhs[i] /= mat.actual_d[i];
        }
        i += istride;
    }
}

__global__ void solve_interleaved2_kernel_ptrs(SolveInterleaved2MatrixPtrs mat,
                                             InterleaveInfo* ii,
                                             int ncore) {
    int icore = blockDim.x * blockIdx.x + threadIdx.x;

    int* ncycles = ii->cellsize;
    int* stridedispl = ii->stridedispl;
    int* strides = ii->stride;
    int* rootbegin = ii->firstnode;
    int* nodebegin = ii->lastnode;

    while (icore < ncore) {
        int iwarp = icore / warpsize;
        int ic = icore & (warpsize - 1);
        int ncycle = ncycles[iwarp];
        int* stride = strides + stridedispl[iwarp];
        int root = rootbegin[iwarp];
        int lastroot = rootbegin[iwarp + 1];
        int firstnode = nodebegin[iwarp];
        int lastnode = nodebegin[iwarp + 1];

        triang_interleaved2_device(mat, ic, ncycle, stride, lastnode);
        bksub_interleaved2_device(mat, root + ic, lastroot, ic, ncycle, stride, firstnode);

        icore += blockDim.x * gridDim.x;
    }
}

extern "C" void coreneuron_solve_interleaved2_launcher_ptrs(double* d_a,
                                                            double* d_b,
                                                            double* d_d,
                                                            double* d_rhs,
                                                            int* d_parent,
                                                            void* interleave_info,
                                                            int ncore,
                                                            void* stream) {
    auto* info = static_cast<InterleaveInfo*>(interleave_info);
    auto cuda_stream = static_cast<cudaStream_t>(stream);

    int threadsPerBlock = warpsize;
    const auto maxBlocksPerGrid = 65535;
    int provisionalBlocksPerGrid = (ncore + threadsPerBlock - 1) / threadsPerBlock;
    int blocksPerGrid = provisionalBlocksPerGrid <= maxBlocksPerGrid ? provisionalBlocksPerGrid
                                                                     : maxBlocksPerGrid;

    SolveInterleaved2MatrixPtrs mat{d_a, d_b, d_d, d_rhs, d_parent};
    solve_interleaved2_kernel_ptrs<<<blocksPerGrid, threadsPerBlock, 0, cuda_stream>>>(
        mat, info, ncore);

    cudaStreamSynchronize(cuda_stream);

    CHECKLAST("coreneuron_solve_interleaved2_launcher_ptrs");
}

void solve_interleaved2_launcher(NrnThread* nt, InterleaveInfo* info, int ncore, void* stream) {
    coreneuron_solve_interleaved2_launcher_ptrs(
        nt->_actual_a, nt->_actual_b, nt->_actual_d, nt->_actual_rhs, nt->_v_parent_index, info,
        ncore, stream);
    CHECKLAST("solve_interleaved2_launcher");
}

}  // namespace coreneuron