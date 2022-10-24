/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once


/// no parallelization using openmp
#define EIGEN_DONT_PARALLELIZE

/// keep host and CUDA code compatible
#ifndef EIGEN_DEFAULT_DENSE_INDEX_TYPE
#define EIGEN_DEFAULT_DENSE_INDEX_TYPE int
#endif

#include "crout/crout.hpp"
#include "newton/newton.hpp"
