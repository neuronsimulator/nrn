/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/
#include "partial_piv_lu/partial_piv_lu.h"

// See explanation in partial_piv_lu.h
EIGEN_DEVICE_FUNC VecType<1> partialPivLu1(const MatType<1>& A, const VecType<1>& b) {
    return A.partialPivLu().solve(b);
}
EIGEN_DEVICE_FUNC VecType<2> partialPivLu2(const MatType<2>& A, const VecType<2>& b) {
    return A.partialPivLu().solve(b);
}
EIGEN_DEVICE_FUNC VecType<3> partialPivLu3(const MatType<3>& A, const VecType<3>& b) {
    return A.partialPivLu().solve(b);
}
EIGEN_DEVICE_FUNC VecType<4> partialPivLu4(const MatType<4>& A, const VecType<4>& b) {
    return A.partialPivLu().solve(b);
}
EIGEN_DEVICE_FUNC VecType<5> partialPivLu5(const MatType<5>& A, const VecType<5>& b) {
    return A.partialPivLu().solve(b);
}
EIGEN_DEVICE_FUNC VecType<6> partialPivLu6(const MatType<6>& A, const VecType<6>& b) {
    return A.partialPivLu().solve(b);
}
EIGEN_DEVICE_FUNC VecType<7> partialPivLu7(const MatType<7>& A, const VecType<7>& b) {
    return A.partialPivLu().solve(b);
}
EIGEN_DEVICE_FUNC VecType<8> partialPivLu8(const MatType<8>& A, const VecType<8>& b) {
    return A.partialPivLu().solve(b);
}
EIGEN_DEVICE_FUNC VecType<9> partialPivLu9(const MatType<9>& A, const VecType<9>& b) {
    return A.partialPivLu().solve(b);
}
EIGEN_DEVICE_FUNC VecType<10> partialPivLu10(const MatType<10>& A, const VecType<10>& b) {
    return A.partialPivLu().solve(b);
}
EIGEN_DEVICE_FUNC VecType<11> partialPivLu11(const MatType<11>& A, const VecType<11>& b) {
    return A.partialPivLu().solve(b);
}
EIGEN_DEVICE_FUNC VecType<12> partialPivLu12(const MatType<12>& A, const VecType<12>& b) {
    return A.partialPivLu().solve(b);
}
EIGEN_DEVICE_FUNC VecType<13> partialPivLu13(const MatType<13>& A, const VecType<13>& b) {
    return A.partialPivLu().solve(b);
}
EIGEN_DEVICE_FUNC VecType<14> partialPivLu14(const MatType<14>& A, const VecType<14>& b) {
    return A.partialPivLu().solve(b);
}
EIGEN_DEVICE_FUNC VecType<15> partialPivLu15(const MatType<15>& A, const VecType<15>& b) {
    return A.partialPivLu().solve(b);
}
EIGEN_DEVICE_FUNC VecType<16> partialPivLu16(const MatType<16>& A, const VecType<16>& b) {
    return A.partialPivLu().solve(b);
}

// Currently there is an issue in Eigen (GPU-branch) for matrices 17x17 and above.
// ToDo: Check in a future release if this issue is resolved!
