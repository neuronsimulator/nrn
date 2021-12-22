/*************************************************************************
 * Copyright (C) 2018-2021 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include "coreneuron/utils/offload.hpp"

#include "Eigen/Dense"
#include "Eigen/LU"

template<int dim>
using MatType = Eigen::Matrix<double, dim, dim, Eigen::ColMajor, dim, dim>;

template<int dim>
using VecType = Eigen::Matrix<double, dim, 1, Eigen::ColMajor, dim, 1>;

// Eigen-3.5+ provides better GPU support. However, some functions cannot be called directly
// from within an OpenACC region. Therefore, we need to wrap them in a special API (decorate
// them with __device__ & acc routine tokens), which allows us to eventually call them from OpenACC.
// Calling these functions from CUDA kernels presents no issue ...

nrn_pragma_omp(declare target)
nrn_pragma_acc(routine seq)
template<int dim>
EIGEN_DEVICE_FUNC VecType<dim> partialPivLu(const MatType<dim>&, const VecType<dim>&);
nrn_pragma_acc(routine seq)
EIGEN_DEVICE_FUNC VecType<1> partialPivLu1(const MatType<1>&, const VecType<1>&);
nrn_pragma_acc(routine seq)
EIGEN_DEVICE_FUNC VecType<2> partialPivLu2(const MatType<2>&, const VecType<2>&);
nrn_pragma_acc(routine seq)
EIGEN_DEVICE_FUNC VecType<3> partialPivLu3(const MatType<3>&, const VecType<3>&);
nrn_pragma_acc(routine seq)
EIGEN_DEVICE_FUNC VecType<4> partialPivLu4(const MatType<4>&, const VecType<4>&);
nrn_pragma_acc(routine seq)
EIGEN_DEVICE_FUNC VecType<5> partialPivLu5(const MatType<5>&, const VecType<5>&);
nrn_pragma_acc(routine seq)
EIGEN_DEVICE_FUNC VecType<6> partialPivLu6(const MatType<6>&, const VecType<6>&);
nrn_pragma_acc(routine seq)
EIGEN_DEVICE_FUNC VecType<7> partialPivLu7(const MatType<7>&, const VecType<7>&);
nrn_pragma_acc(routine seq)
EIGEN_DEVICE_FUNC VecType<8> partialPivLu8(const MatType<8>&, const VecType<8>&);
nrn_pragma_acc(routine seq)
EIGEN_DEVICE_FUNC VecType<9> partialPivLu9(const MatType<9>&, const VecType<9>&);
nrn_pragma_acc(routine seq)
EIGEN_DEVICE_FUNC VecType<10> partialPivLu10(const MatType<10>&, const VecType<10>&);
nrn_pragma_acc(routine seq)
EIGEN_DEVICE_FUNC VecType<11> partialPivLu11(const MatType<11>&, const VecType<11>&);
nrn_pragma_acc(routine seq)
EIGEN_DEVICE_FUNC VecType<12> partialPivLu12(const MatType<12>&, const VecType<12>&);
nrn_pragma_acc(routine seq)
EIGEN_DEVICE_FUNC VecType<13> partialPivLu13(const MatType<13>&, const VecType<13>&);
nrn_pragma_acc(routine seq)
EIGEN_DEVICE_FUNC VecType<14> partialPivLu14(const MatType<14>&, const VecType<14>&);
nrn_pragma_acc(routine seq)
EIGEN_DEVICE_FUNC VecType<15> partialPivLu15(const MatType<15>&, const VecType<15>&);
nrn_pragma_acc(routine seq)
EIGEN_DEVICE_FUNC VecType<16> partialPivLu16(const MatType<16>&, const VecType<16>&);
nrn_pragma_omp(end declare target)
