/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#define CATCH_CONFIG_MAIN

#include "nmodl.hpp"

#include <catch2/catch.hpp>

#include <chrono>
#include <iostream>
#include <random>

#include "Eigen/Dense"
#include "Eigen/LU"

using namespace nmodl;
using namespace Eigen;
using namespace std;


/// https://stackoverflow.com/questions/15051367/how-to-compare-vectors-approximately-in-eigen
template <typename DerivedA, typename DerivedB>
bool allclose(const Eigen::DenseBase<DerivedA>& a,
              const Eigen::DenseBase<DerivedB>& b,
              const typename DerivedA::RealScalar& rtol =
                  Eigen::NumTraits<typename DerivedA::RealScalar>::dummy_precision(),
              const typename DerivedA::RealScalar& atol =
                  Eigen::NumTraits<typename DerivedA::RealScalar>::epsilon()) {
    return ((a.derived() - b.derived()).array().abs() <= (atol + rtol * b.derived().array().abs()))
        .all();
}


template <typename T>
bool test_Crout_correctness(T rtol = 1e-8, T atol = 1e-8) {
    std::random_device rd;  // seeding
    auto seed = rd();
    std::mt19937 mt(seed);
    std::uniform_real_distribution<T> nums(-1e3, 1e3);

    std::chrono::duration<double> eigen_timing(std::chrono::duration<double>::zero());
    std::chrono::duration<double> crout_timing(std::chrono::duration<double>::zero());

    auto t1 = std::chrono::high_resolution_clock::now();
    auto t2 = std::chrono::high_resolution_clock::now();

    for (int mat_size = 5; mat_size < 15; mat_size++) {
        Matrix<T, Dynamic, Dynamic, Eigen::ColMajor> A_ColMajor(mat_size,
                                                                mat_size);  // default in Eigen!
        Matrix<T, Dynamic, Dynamic, Eigen::RowMajor> A_RowMajor(mat_size, mat_size);
        Matrix<T, Dynamic, 1> b(mat_size);

        for (int repetitions = 0; repetitions < static_cast<int>(1e3); ++repetitions) {
            do {
                // initialization
                for (int r = 0; r < mat_size; r++) {
                    for (int c = 0; c < mat_size; c++) {
                        A_ColMajor(r, c) = nums(mt);
                        A_RowMajor(r, c) = A_ColMajor(r, c);
                        b(r) = nums(mt);
                    }
                }
            } while (!A_ColMajor.fullPivLu().isInvertible());  // Checking Invertibility

            t1 = std::chrono::high_resolution_clock::now();
            // Eigen (ColMajor)
            Matrix<T, Dynamic, 1> eigen_x_ColMajor(mat_size);
            eigen_x_ColMajor = A_ColMajor.partialPivLu().solve(b);

            // Eigen (RowMajor)
            Matrix<T, Dynamic, 1> eigen_x_RowMajor(mat_size);
            eigen_x_RowMajor = A_RowMajor.partialPivLu().solve(b);
            t2 = std::chrono::high_resolution_clock::now();
            eigen_timing += (t2 - t1);

            if (!allclose(eigen_x_ColMajor, eigen_x_RowMajor, rtol, atol)) {
                cerr << "eigen_x_ColMajor vs eigen_x_RowMajor (issue) / seed = " << seed << endl;
                return false;
            }

            t1 = std::chrono::high_resolution_clock::now();
            // Crout with A_ColMajor
            Matrix<T, Dynamic, 1> crout_x_ColMajor(mat_size);
            if (!A_ColMajor.IsRowMajor)
                A_ColMajor.transposeInPlace();
            Matrix<int, Dynamic, 1> pivot(mat_size);
            crout::Crout<T>(mat_size, A_ColMajor.data(), pivot.data());
            crout::solveCrout<T>(
                mat_size, A_ColMajor.data(), b.data(), crout_x_ColMajor.data(), pivot.data());

            // Crout with A_RowMajor
            Matrix<T, Dynamic, 1> crout_x_RowMajor(mat_size);
            crout::Crout<T>(mat_size, A_RowMajor.data(), pivot.data());
            crout::solveCrout<T>(
                mat_size, A_RowMajor.data(), b.data(), crout_x_RowMajor.data(), pivot.data());
            t2 = std::chrono::high_resolution_clock::now();
            crout_timing += (t2 - t1);

            if (!allclose(eigen_x_ColMajor, crout_x_ColMajor, rtol, atol)) {
                cerr << "eigen_x_ColMajor vs crout_x_ColMajor (issue) / seed = " << seed << endl;
                return false;
            }

            if (!allclose(eigen_x_RowMajor, crout_x_RowMajor, rtol, atol)) {
                cerr << "eigen_x_RowMajor vs crout_x_RowMajor (issue) / seed = " << seed << endl;
                return false;
            }
        }
    }

    std::cout << "eigen_timing [ms] : " << eigen_timing.count() * 1e3 << std::endl;
    std::cout << "crout_timing [ms] : " << crout_timing.count() * 1e3 << std::endl;

    return true;
}


SCENARIO("Compare Crout solver with Eigen") {
    GIVEN("crout (double)") {
        constexpr double rtol = 1e-8;
        constexpr double atol = 1e-8;

        auto test = test_Crout_correctness<double>(rtol, atol);

        THEN("run tests & compare") {
            REQUIRE(test);
        }
    }
}
