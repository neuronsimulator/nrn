/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

// Implementation of Newton method for solving
// system of non-linear equations using Eigen

#include <iostream>

#include "Eigen/LU"

namespace nmodl {
namespace newton {

constexpr int MAX_ITER = 1e3;
constexpr double EPS = 1e-12;

// Newton method
// given initial vector X
// and a functor that calculates F(X), J(X)
// solves for F(X) = 0,
// starting with initial value of X
// where J(X) is the Jacobian of F(X)
// by iterating:
// X_{n+1} = X_n - J(X_n)^{-1} F(X_n)
//
// when |F|^2 < eps^2, solution has converged
// returns number of iterations (-1 if failed to converge)
template <int N, typename FUNC>
EIGEN_DEVICE_FUNC int newton_solver(Eigen::Matrix<double, N, 1>& X,
                                    FUNC functor,
                                    double eps = EPS,
                                    int max_iter = MAX_ITER) {
    // Vector to store result of function F(X):
    Eigen::Matrix<double, N, 1> F;
    // Matrix to store jacobian of F(X):
    Eigen::Matrix<double, N, N> J;
    // Solver iteration count:
    int iter = -1;

    while (++iter < max_iter) {
        // calculate F, J from X using user-supplied functor
        functor(X, F, J);
        // get error norm: here we use sqrt(|F|^2)
        double error = F.norm();
        // std::cout.precision(15);
        // std::cout << "[newton_solver] iter: " << iter << "\terror: " << error << "\n";
        // std::cout << "\n" << X << std::endl;
        if (error < eps) {
            // we have converged: return iteration count
            return iter;
        }
        // update X
        // use in-place LU decomposition of J with partial pivoting
        // (suitable for any N, but less efficient than .inverse() for N <=4)
        X -= Eigen::PartialPivLU<Eigen::Ref<Eigen::Matrix<double, N, N>>>(J).solve(F);
    }
    // If we fail to converge after max_iter iterations, return -1
    return -1;
}

template <typename FUNC, int N>
EIGEN_DEVICE_FUNC int solver(Eigen::Matrix<double, N, 1>& X,
                             FUNC functor,
                             double eps,
                             int max_iter) {
    Eigen::Matrix<double, N, 1> F;
    Eigen::Matrix<double, N, N> J;
    int iter = -1;
    while (++iter < max_iter) {
        functor(X, F, J);
        double error = F.norm();
        if (error < eps) {
            return iter;
        }
        X -= J.inverse() * F;
    }
    return -1;
}

// template specializations for N <= 4
// use explicit inverse of F instead of LU decomposition
// (more efficient for small matrices - not safe for large matrices)
template <typename FUNC>
EIGEN_DEVICE_FUNC int newton_solver(Eigen::Matrix<double, 1, 1>& X,
                                    FUNC functor,
                                    double eps = EPS,
                                    int max_iter = MAX_ITER) {
    return solver<FUNC, 1>(X, functor, eps, max_iter);
}

template <typename FUNC>
EIGEN_DEVICE_FUNC int newton_solver(Eigen::Matrix<double, 2, 1>& X,
                                    FUNC functor,
                                    double eps = EPS,
                                    int max_iter = MAX_ITER) {
    return solver<FUNC, 2>(X, functor, eps, max_iter);
}

template <typename FUNC>
EIGEN_DEVICE_FUNC int newton_solver(Eigen::Matrix<double, 3, 1>& X,
                                    FUNC functor,
                                    double eps = EPS,
                                    int max_iter = MAX_ITER) {
    return solver<FUNC, 3>(X, functor, eps, max_iter);
}

template <typename FUNC>
EIGEN_DEVICE_FUNC int newton_solver(Eigen::Matrix<double, 4, 1>& X,
                                    FUNC functor,
                                    double eps = EPS,
                                    int max_iter = MAX_ITER) {
    return solver<FUNC, 4>(X, functor, eps, max_iter);
}

}  // namespace newton
}  // namespace nmodl
