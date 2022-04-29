/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include "coreneuron/utils/offload.hpp"

#include "Eigen/Dense"
#include "Eigen/LU"

template <int dim>
using MatType = Eigen::Matrix<double, dim, dim, Eigen::ColMajor, dim, dim>;

template <int dim>
using VecType = Eigen::Matrix<double, dim, 1, Eigen::ColMajor, dim, 1>;

// Eigen-3.5+ provides better GPU support. However, some functions cannot be called directly
// from within an OpenACC region. Therefore, we need to wrap them in a special API (decorate
// them with __device__ & acc routine tokens), which allows us to eventually call them from OpenACC.
// Calling these functions from CUDA kernels presents no issue ...

// We want to declare a function template that is callable from OpenMP and
// OpenACC code, but whose instantiations are compiled by CUDA. This is to avoid
// Eigen internals having to be digested by OpenACC/OpenMP compilers. The
// problem is that it is apparently not sufficient to declare a template in a OpenMP
// declare target region and have the OpenMP compiler assume that device
// versions of instantations of it will be available. The convoluted approach
// here has two ingredients:
//  - partialPivLu<N>(...), a function template that has explicit
//    instantiations visible in this header file that call:
//  - partialPivLuN(...), functions that are declared in this header but defined
//    in the CUDA file partial_piv_lu.cu.
nrn_pragma_omp(declare target)
nrn_pragma_acc(routine seq)
template <int dim>
EIGEN_DEVICE_FUNC VecType<dim> partialPivLu(const MatType<dim>&, const VecType<dim>&);
#define InstantiatePartialPivLu(N)                                                               \
    nrn_pragma_acc(routine seq)                                                                  \
    EIGEN_DEVICE_FUNC VecType<N> partialPivLu##N(const MatType<N>&, const VecType<N>&);          \
    nrn_pragma_acc(routine seq)                                                                  \
    template <>                                                                                  \
    EIGEN_DEVICE_FUNC inline VecType<N> partialPivLu(const MatType<N>& A, const VecType<N>& b) { \
        return partialPivLu##N(A, b);                                                            \
    }
InstantiatePartialPivLu(1)
InstantiatePartialPivLu(2)
InstantiatePartialPivLu(3)
InstantiatePartialPivLu(4)
InstantiatePartialPivLu(5)
InstantiatePartialPivLu(6)
InstantiatePartialPivLu(7)
InstantiatePartialPivLu(8)
InstantiatePartialPivLu(9)
InstantiatePartialPivLu(10)
InstantiatePartialPivLu(11)
InstantiatePartialPivLu(12)
InstantiatePartialPivLu(13)
InstantiatePartialPivLu(14)
InstantiatePartialPivLu(15)
InstantiatePartialPivLu(16)
#undef InstantiatePartialPivLu
nrn_pragma_omp(end declare target)
