/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once


/// no parallelization using openmp
#define EIGEN_DONT_PARALLELIZE

/// keep host and CUDA code compatible
#ifndef EIGEN_DEFAULT_DENSE_INDEX_TYPE
#define EIGEN_DEFAULT_DENSE_INDEX_TYPE int
#endif

#include "crout/crout.hpp"
#include "newton/newton.hpp"
