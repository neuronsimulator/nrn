/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/
#define NMODL_EIGEN_NO_OPENACC                          // disable OpenACC/OpenMP annotations
#define NMODL_LEAVE_PartialPivLuInstantiations_DEFINED  // import PartialPivLuInstantiations
#include "partial_piv_lu/partial_piv_lu.h"

// See explanation in partial_piv_lu.h
#define InstantiatePartialPivLu(N)                                                          \
    NMODL_EIGEN_ATTR VecType<N> partialPivLu##N(const MatType<N>& A, const VecType<N>& b) { \
        return A.partialPivLu().solve(b);                                                   \
    }
PartialPivLuInstantiations
#undef InstantiatePartialPivLu
