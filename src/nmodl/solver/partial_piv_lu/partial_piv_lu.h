/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

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

#if defined(_OPENACC) && !defined(DISABLE_OPENACC)
#pragma acc routine seq
#endif
template<int dim>
VecType<dim> partialPivLu(const MatType<dim>&, const VecType<dim>&);
