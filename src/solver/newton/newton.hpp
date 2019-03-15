/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

// Implementation of Newton method for solving system of non-linear equations using Eigen
// - newton_solver is the preferred option: requires user to provide Jacobian
// - newton_numerical_diff_solver is the fallback option: Jacobian not required

#include <Eigen/LU>

namespace nmodl {
namespace newton {

constexpr int MAX_ITER = 1e3;
constexpr double EPS = 1e-12;

// Newton method with user-provided Jacobian:
// given initial vector X
// and a functor that calculates F(X), J(X)
// where J(X) is the Jacobian of F(X),
// solves for F(X) = 0,
// starting with initial value of X
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

// Newton method without user-provided Jacobian:
// given initial vector X
// and a functor that calculates F(X),
// solves for F(X) = 0,
// starting with initial value of X
// by iterating:
// X_{n+1} = X_n - J(X_n)^{-1} F(X_n)
// where J(X) is the Jacobian of F(X), which is
// approximated numerically using a symmetric
// finite difference approximation to the derivative

// when |F|^2 < eps^2, solution has converged
// returns number of iterations (-1 if failed to converge)
constexpr double SQUARE_ROOT_ULP = 1e-7;
constexpr double CUBIC_ROOT_ULP = 1e-5;
template <int N, typename FUNC>
EIGEN_DEVICE_FUNC int newton_numerical_diff_solver(Eigen::Matrix<double, N, 1>& X,
                                                   FUNC functor,
                                                   double eps = EPS,
                                                   int max_iter = MAX_ITER) {
    // Vector to store result of function F(X):
    Eigen::Matrix<double, N, 1> F;
    // Temporary storage for F(X+dx)
    Eigen::Matrix<double, N, 1> F_p;
    // Temporary storage for F(X-dx)
    Eigen::Matrix<double, N, 1> F_m;
    // Matrix to store jacobian of F(X):
    Eigen::Matrix<double, N, N> J;
    // Solver iteration count:
    int iter = 0;
    while (iter < max_iter) {
        // calculate F from X using user-supplied functor
        functor(X, F);
        // get error norm: here we use sqrt(|F|^2)
        double error = F.norm();
        if (error < eps) {
            // we have converged: return iteration count
            return iter;
        }
        ++iter;
        // calculate approximate Jacobian
        for (int i = 0; i < N; ++i) {
            // symmetric finite difference approximation to derivative
            // df/dx ~= ( f(x+dx) - f(x-dx) ) / (2*dx)
            // choose dx to be ~(ULP)^{1/3}*X[i]
            // https://aip.scitation.org/doi/pdf/10.1063/1.4822971
            // also enforce a lower bound ~sqrt(ULP) to avoid dx being too small
            double dX = std::max(CUBIC_ROOT_ULP * X[i], SQUARE_ROOT_ULP);
            // F(X + dX)
            X[i] += dX;
            functor(X, F_p);
            // F(X - dX)
            X[i] -= 2.0 * dX;
            functor(X, F_m);
            F_p -= F_m;
            // J = (F(X + dX) - F(X - dX)) / (2*dX)
            J.col(i) = F_p / (2.0 * dX);
            // restore X
            X[i] += dX;
        }
        // update X
        // use in-place LU decomposition of J with partial pivoting
        // (suitable for any N, but less efficient than .inverse() for N <=4)
        X -= Eigen::PartialPivLU<Eigen::Ref<Eigen::Matrix<double, N, N>>>(J).solve(F);
    }
    // If we fail to converge after max_iter iterations, return -1
    return -1;
}

// Newton method template specializations for N <= 4
// Use explicit inverse of F instead of LU decomposition
// This is faster, as there is no pivoting and therefore no branches,
// but it is not numerically safe for N > 4
template <typename FUNC, int N>
EIGEN_DEVICE_FUNC int newton_solver_small_N(Eigen::Matrix<double, N, 1>& X,
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
template <typename FUNC>
EIGEN_DEVICE_FUNC int newton_solver(Eigen::Matrix<double, 1, 1>& X,
                                    FUNC functor,
                                    double eps = EPS,
                                    int max_iter = MAX_ITER) {
    return newton_solver_small_N<FUNC, 1>(X, functor, eps, max_iter);
}

template <typename FUNC>
EIGEN_DEVICE_FUNC int newton_solver(Eigen::Matrix<double, 2, 1>& X,
                                    FUNC functor,
                                    double eps = EPS,
                                    int max_iter = MAX_ITER) {
    return newton_solver_small_N<FUNC, 2>(X, functor, eps, max_iter);
}

template <typename FUNC>
EIGEN_DEVICE_FUNC int newton_solver(Eigen::Matrix<double, 3, 1>& X,
                                    FUNC functor,
                                    double eps = EPS,
                                    int max_iter = MAX_ITER) {
    return newton_solver_small_N<FUNC, 3>(X, functor, eps, max_iter);
}

template <typename FUNC>
EIGEN_DEVICE_FUNC int newton_solver(Eigen::Matrix<double, 4, 1>& X,
                                    FUNC functor,
                                    double eps = EPS,
                                    int max_iter = MAX_ITER) {
    return newton_solver_small_N<FUNC, 4>(X, functor, eps, max_iter);
}

}  // namespace newton
}  // namespace nmodl
