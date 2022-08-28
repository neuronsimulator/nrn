/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/
#pragma once

#ifndef NMODL_EIGEN_NO_OPENACC
#include "coreneuron/utils/offload.hpp"
#endif

#include <Eigen/Dense>
#include <Eigen/LU>

template <int dim>
using MatType = Eigen::Matrix<double, dim, dim, Eigen::ColMajor, dim, dim>;

template <int dim>
using VecType = Eigen::Matrix<double, dim, 1, Eigen::ColMajor, dim, 1>;

// We want to declare a function template that is callable from OpenMP and
// OpenACC code, but whose instantiations are compiled by nvc++ -cuda. This is to avoid
// Eigen internals having to be digested by OpenACC/OpenMP compilers. The
// problem is that it is apparently not sufficient to declare a template in a OpenMP
// declare target region and have the OpenMP compiler assume that device
// versions of instantations of it will be available. The convoluted approach
// here has two ingredients:
//  - partialPivLu<N>(...), a function template that has explicit
//    specialisations visible in this header file that call:
//  - partialPivLuN(...), functions that are declared in this header but defined
//    in the C++/CUDA file partial_piv_lu.cpp.
//
// It does not seem to work to generically define the template function
// partialPivLu<N> in the .cpp file and then explicitly instantate it for 1..16
// there; the relevant device functions are not generated. A possible workaround
// is to define the template + a dummy function that calls many different
// instantiations of the template in the .cpp file, but this seems more fragile
// than the present approach.
#ifdef NMODL_EIGEN_NO_OPENACC
#define NMODL_EIGEN_ATTR __host__ __device__
#define NMODL_EIGEN_ROUTINE_SEQ
#else
nrn_pragma_omp(declare target)
#define NMODL_EIGEN_ATTR
#define NMODL_EIGEN_ROUTINE_SEQ nrn_pragma_acc(routine seq)
#endif
NMODL_EIGEN_ROUTINE_SEQ
template <int dim>
NMODL_EIGEN_ATTR VecType<dim> partialPivLu(const MatType<dim>&, const VecType<dim>&);
#define InstantiatePartialPivLu(N)                                                              \
    NMODL_EIGEN_ROUTINE_SEQ                                                                     \
    NMODL_EIGEN_ATTR VecType<N> partialPivLu##N(const MatType<N>&, const VecType<N>&);          \
    NMODL_EIGEN_ROUTINE_SEQ                                                                     \
    template <>                                                                                 \
    NMODL_EIGEN_ATTR inline VecType<N> partialPivLu(const MatType<N>& A, const VecType<N>& b) { \
        return partialPivLu##N(A, b);                                                           \
    }
// This PartialPivLuInstantiations is used in partial_piv_lu.cpp with a
// different definition of the InstantiatePartialPivLu macro. As of 2022-08-18 there is still an
// issue with instantiating for matrices larger than 16x16.
#define PartialPivLuInstantiations \
    InstantiatePartialPivLu(1)     \
    InstantiatePartialPivLu(2)     \
    InstantiatePartialPivLu(3)     \
    InstantiatePartialPivLu(4)     \
    InstantiatePartialPivLu(5)     \
    InstantiatePartialPivLu(6)     \
    InstantiatePartialPivLu(7)     \
    InstantiatePartialPivLu(8)     \
    InstantiatePartialPivLu(9)     \
    InstantiatePartialPivLu(10)    \
    InstantiatePartialPivLu(11)    \
    InstantiatePartialPivLu(12)    \
    InstantiatePartialPivLu(13)    \
    InstantiatePartialPivLu(14)    \
    InstantiatePartialPivLu(15)    \
    InstantiatePartialPivLu(16)
PartialPivLuInstantiations
#undef InstantiatePartialPivLu
#ifndef NMODL_EIGEN_NO_OPENACC
nrn_pragma_omp(end declare target)
#endif
// partial_piv_lu.cpp will request that this is left defined
#ifndef NMODL_LEAVE_PartialPivLuInstantiations_DEFINED
#undef PartialPivLuInstantiations
#endif
