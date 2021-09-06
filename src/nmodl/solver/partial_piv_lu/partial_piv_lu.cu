/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "partial_piv_lu/partial_piv_lu.h"

template<int dim>
EIGEN_DEVICE_FUNC 
VecType<dim> partialPivLu(const MatType<dim>& A, const VecType<dim>& b)
{
    return A.partialPivLu().solve(b);
}

// Explicit Template Instantiation
template EIGEN_DEVICE_FUNC VecType<1> partialPivLu<1>(const MatType<1>& A, const VecType<1>& b);
template EIGEN_DEVICE_FUNC VecType<2> partialPivLu<2>(const MatType<2>& A, const VecType<2>& b);
template EIGEN_DEVICE_FUNC VecType<3> partialPivLu<3>(const MatType<3>& A, const VecType<3>& b);
template EIGEN_DEVICE_FUNC VecType<4> partialPivLu<4>(const MatType<4>& A, const VecType<4>& b);
template EIGEN_DEVICE_FUNC VecType<5> partialPivLu<5>(const MatType<5>& A, const VecType<5>& b);
template EIGEN_DEVICE_FUNC VecType<6> partialPivLu<6>(const MatType<6>& A, const VecType<6>& b);
template EIGEN_DEVICE_FUNC VecType<7> partialPivLu<7>(const MatType<7>& A, const VecType<7>& b);
template EIGEN_DEVICE_FUNC VecType<8> partialPivLu<8>(const MatType<8>& A, const VecType<8>& b);
template EIGEN_DEVICE_FUNC VecType<9> partialPivLu<9>(const MatType<9>& A, const VecType<9>& b);
template EIGEN_DEVICE_FUNC VecType<10> partialPivLu<10>(const MatType<10>& A, const VecType<10>& b);
template EIGEN_DEVICE_FUNC VecType<11> partialPivLu<11>(const MatType<11>& A, const VecType<11>& b);
template EIGEN_DEVICE_FUNC VecType<12> partialPivLu<12>(const MatType<12>& A, const VecType<12>& b);
template EIGEN_DEVICE_FUNC VecType<13> partialPivLu<13>(const MatType<13>& A, const VecType<13>& b);
template EIGEN_DEVICE_FUNC VecType<14> partialPivLu<14>(const MatType<14>& A, const VecType<14>& b);
template EIGEN_DEVICE_FUNC VecType<15> partialPivLu<15>(const MatType<15>& A, const VecType<15>& b);
template EIGEN_DEVICE_FUNC VecType<16> partialPivLu<16>(const MatType<16>& A, const VecType<16>& b);

// Currently there is an issue in Eigen (GPU-branch) for matrices 17x17 and above.
// ToDo: Check in a future release if this issue is resolved!
