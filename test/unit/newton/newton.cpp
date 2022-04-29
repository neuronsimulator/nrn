/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#define CATCH_CONFIG_MAIN

#include <cmath>

#include <catch/catch.hpp>

#include "nmodl.hpp"

using namespace nmodl;

constexpr double max_error_norm = 1e-12;

SCENARIO("Non-linear system to solve with Newton Numerical Diff Solver", "[numerical][solver]") {
    GIVEN("1 linear eq") {
        struct functor {
            void operator()(const Eigen::Matrix<double, 1, 1>& X,
                            Eigen::Matrix<double, 1, 1>& F) const {
                // Function F(X) to find F(X)=0 solution
                F[0] = -3.0 * X[0] + 3.0;
            }
        };
        Eigen::Matrix<double, 1, 1> X{22.2536};
        Eigen::Matrix<double, 1, 1> F;
        functor fn;
        int iter_newton = newton::newton_numerical_diff_solver(X, fn);
        fn(X, F);
        THEN("find the solution") {
            CAPTURE(iter_newton);
            CAPTURE(X);
            REQUIRE(iter_newton > 0);
            REQUIRE(X[0] == Approx(1.0));
            REQUIRE(F.norm() < max_error_norm);
        }
    }

    GIVEN("1 non-linear eq") {
        struct functor {
            void operator()(const Eigen::Matrix<double, 1, 1>& X,
                            Eigen::Matrix<double, 1, 1>& F) const {
                F[0] = -3.0 * X[0] + std::sin(X[0]) + std::log(X[0] * X[0] + 11.435243) + 3.0;
            }
        };
        Eigen::Matrix<double, 1, 1> X{-0.21421};
        Eigen::Matrix<double, 1, 1> F;
        functor fn;
        int iter_newton = newton::newton_numerical_diff_solver(X, fn);
        fn(X, F);
        THEN("find the solution") {
            CAPTURE(iter_newton);
            CAPTURE(X);
            REQUIRE(iter_newton > 0);
            REQUIRE(X[0] == Approx(2.19943987001206));
            REQUIRE(F.norm() < max_error_norm);
        }
    }

    GIVEN("system of 2 non-linear eqs") {
        struct functor {
            void operator()(const Eigen::Matrix<double, 2, 1>& X,
                            Eigen::Matrix<double, 2, 1>& F) const {
                F[0] = -3.0 * X[0] * X[1] + X[0] + 2.0 * X[1] - 1.0;
                F[1] = 4.0 * X[0] - 0.29999999999999999 * std::pow(X[1], 2) + X[1] + 0.4;
            }
        };
        Eigen::Matrix<double, 2, 1> X{0.2, 0.4};
        Eigen::Matrix<double, 2, 1> F;
        functor fn;
        int iter_newton = newton::newton_numerical_diff_solver(X, fn);
        fn(X, F);
        THEN("find a solution") {
            CAPTURE(iter_newton);
            CAPTURE(X);
            REQUIRE(iter_newton > 0);
            REQUIRE(F.norm() < max_error_norm);
        }
    }

    GIVEN("system of 3 non-linear eqs") {
        struct functor {
            double _x_old = 0.5;
            double _y_old = -231.5;
            double _z_old = 1.4565;
            double dt = 0.2;
            double a = 0.2354;
            double b = 436.2;
            double d = 23.1;
            double e = 0.01;
            double z = 0.99;
            void operator()(const Eigen::Matrix<double, 3, 1>& X,
                            Eigen::Matrix<double, 3, 1>& F) const {
                F(0) = -(-_x_old - dt * (a * std::pow(X[0], 2) + X[1]) + X[0]);
                F(1) = -(_y_old - dt * (a * X[0] + b * X[1] + d) + X[1]);
                F(2) = -(_z_old - dt * (e * z - 3.0 * X[0] + 2.0 * X[1]) + X[2]);
            }
        };
        Eigen::Matrix<double, 3, 1> X{0.21231, 0.4435, -0.11537};
        Eigen::Matrix<double, 3, 1> F;
        functor fn;
        int iter_newton = newton::newton_numerical_diff_solver(X, fn);
        fn(X, F);
        THEN("find a solution") {
            CAPTURE(iter_newton);
            CAPTURE(X);
            REQUIRE(iter_newton > 0);
            REQUIRE(F.norm() < max_error_norm);
        }
    }

    GIVEN("system of 4 non-linear eqs") {
        struct functor {
            double _X0_old = 1.2345;
            double _X1_old = 1.2345;
            double _X2_old = 1.2345;
            double _X3_old = 1.2345;
            double dt = 0.2;
            void operator()(const Eigen::Matrix<double, 4, 1>& X,
                            Eigen::Matrix<double, 4, 1>& F) const {
                F[0] = -(-3.0 * X[0] * X[2] * dt + X[0] - _X0_old + 2.0 * dt / X[1]);
                F[1] = -(X[1] - _X1_old + dt * (4.0 * X[0] - 6.2 * X[1] + X[3]));
                F[2] = -((X[2] * (X[2] - _X2_old) - dt * (X[2] * (-1.2 * X[1] + 3.0) + 0.3)) /
                         X[2]);
                F[3] = -(-4.0 * X[0] * X[1] * X[2] * dt + X[3] - _X3_old + 6.0 * dt / X[2]);
            }
        };
        Eigen::Matrix<double, 4, 1> X{0.21231, 0.4435, -0.11537, -0.8124312};
        Eigen::Matrix<double, 4, 1> F;
        functor fn;
        int iter_newton = newton::newton_numerical_diff_solver(X, fn);
        fn(X, F);
        THEN("find a solution") {
            CAPTURE(iter_newton);
            CAPTURE(X);
            REQUIRE(iter_newton > 0);
            REQUIRE(F.norm() < max_error_norm);
        }
    }

    GIVEN("system of 5 non-linear eqs") {
        struct functor {
            void operator()(const Eigen::Matrix<double, 5, 1>& X,
                            Eigen::Matrix<double, 5, 1>& F) const {
                F[0] = -3.0 * X[0] * X[2] + X[0] + 2.0 / X[1];
                F[1] = 4.0 * X[0] - 5.2 * X[1] + X[3];
                F[2] = 1.2 * X[1] + X[2] - 3.0 - 0.3 / X[2];
                F[3] = -4.0 * X[0] * X[1] * X[2] + X[3] + 6.0 / X[2];
                F[4] = (-4.0 * X[0] + (X[4] + cos(X[1])) * (X[1] * X[2] - X[3] * X[4])) /
                       (X[1] * X[2] - X[3] * X[4]);
            }
        };
        Eigen::Matrix<double, 5, 1> X;
        X << 8.234, -245.46, 123.123, 0.8343, 5.555;
        Eigen::Matrix<double, 5, 1> F;
        functor fn;
        int iter_newton = newton::newton_numerical_diff_solver(X, fn);
        fn(X, F);
        THEN("find a solution") {
            CAPTURE(iter_newton);
            CAPTURE(X);
            REQUIRE(iter_newton > 0);
            REQUIRE(F.norm() < max_error_norm);
        }
    }

    GIVEN("system of 10 non-linear eqs") {
        struct functor {
            void operator()(const Eigen::Matrix<double, 10, 1>& X,
                            Eigen::Matrix<double, 10, 1>& F) const {
                F[0] = -3.0 * X[0] * X[1] + X[0] + 2.0 * X[1];
                F[1] = 4.0 * X[0] - 0.29999999999999999 * std::pow(X[1], 2) + X[1];
                F[2] = 2.0 * X[1] + X[2] + 2.0 * X[3] * X[5] * X[7] - 3.0 * X[4] * X[8] - X[5];
                F[3] = 4.0 * X[0] - 0.29999999999999999 * std::pow(X[1], 2) + X[3] -
                       X[4] * X[6] * X[7];
                F[4] = -3.0 * X[0] * X[7] + 2.0 * X[1] - 4.0 * X[3] * X[8] + X[4];
                F[5] = -X[2] * X[5] * X[8] + 4.0 * X[3] - 0.29999999999999999 * X[4] * X[9] + X[5];
                F[6] = -3.0 * X[0] * X[1] - 2.1000000000000001 * X[3] * X[4] * X[5] + X[6] +
                       2.0 * X[8];
                F[7] = 4.0 * X[0] - 0.29999999999999999 * X[6] * X[7] + X[7];
                F[8] = -3.0 * X[0] * X[1] - X[2] * X[3] * X[4] * std::pow(X[5], 2) + 2.0 * X[5] +
                       X[8];
                F[9] = -0.29999999999999999 * X[2] * X[4] + 4.0 * std::pow(X[9], 2) + X[9];
            }
        };
        Eigen::Matrix<double, 10, 1> X;
        X << 8.234, -5.46, 1.123, 0.8343, 5.555, 18.234, -2.46, 0.123, 10.8343, -4.685;
        Eigen::Matrix<double, 10, 1> F;
        functor fn;
        int iter_newton = newton::newton_numerical_diff_solver(X, fn);
        fn(X, F);
        THEN("find a solution") {
            CAPTURE(iter_newton);
            CAPTURE(X);
            REQUIRE(iter_newton > 0);
            REQUIRE(F.norm() < max_error_norm);
        }
    }
}

SCENARIO("Non-linear system to solve with Newton Solver", "[analytic][solver]") {
    GIVEN("1 linear eq") {
        struct functor {
            void operator()(const Eigen::Matrix<double, 1, 1>& X,
                            Eigen::Matrix<double, 1, 1>& F,
                            Eigen::Matrix<double, 1, 1>& J) const {
                // Function F(X) to find F(X)=0 solution
                F[0] = -3.0 * X[0] + 3.0;
                // Jacobian of F(X), i.e. the matrix dF(X)_i/dX_j
                J(0, 0) = -3.0;
            }
        };
        Eigen::Matrix<double, 1, 1> X{22.2536};
        Eigen::Matrix<double, 1, 1> F;
        Eigen::Matrix<double, 1, 1> J;
        functor fn;
        int iter_newton = newton::newton_solver(X, fn);
        fn(X, F, J);
        THEN("find the solution") {
            CAPTURE(iter_newton);
            CAPTURE(X);
            REQUIRE(iter_newton > 0);
            REQUIRE(X[0] == Approx(1.0));
            REQUIRE(F.norm() < max_error_norm);
        }
    }

    GIVEN("1 non-linear eq") {
        struct functor {
            void operator()(const Eigen::Matrix<double, 1, 1>& X,
                            Eigen::Matrix<double, 1, 1>& F,
                            Eigen::Matrix<double, 1, 1>& J) const {
                F[0] = -3.0 * X[0] + std::sin(X[0]) + std::log(X[0] * X[0] + 11.435243) + 3.0;
                J(0, 0) = -3.0 + std::cos(X[0]) + 2.0 * X[0] / (X[0] * X[0] + 11.435243);
            }
        };
        Eigen::Matrix<double, 1, 1> X{-0.21421};
        Eigen::Matrix<double, 1, 1> F;
        Eigen::Matrix<double, 1, 1> J;
        functor fn;
        int iter_newton = newton::newton_solver(X, fn);
        fn(X, F, J);
        THEN("find the solution") {
            CAPTURE(iter_newton);
            CAPTURE(X);
            REQUIRE(iter_newton > 0);
            REQUIRE(X[0] == Approx(2.19943987001206));
            REQUIRE(F.norm() < max_error_norm);
        }
    }

    GIVEN("system of 2 non-linear eqs") {
        struct functor {
            void operator()(const Eigen::Matrix<double, 2, 1>& X,
                            Eigen::Matrix<double, 2, 1>& F,
                            Eigen::Matrix<double, 2, 2>& J) const {
                F[0] = -3.0 * X[0] * X[1] + X[0] + 2.0 * X[1] - 1.0;
                F[1] = 4.0 * X[0] - 0.29999999999999999 * std::pow(X[1], 2) + X[1] + 0.4;
                J(0, 0) = -3.0 * X[1] + 1.0;
                J(0, 1) = -3.0 * X[0] + 2.0;
                J(1, 0) = 4.0;
                J(1, 1) = -0.59999999999999998 * X[1] + 1.0;
            }
        };
        Eigen::Matrix<double, 2, 1> X{0.2, 0.4};
        Eigen::Matrix<double, 2, 1> F;
        Eigen::Matrix<double, 2, 2> J;
        functor fn;
        int iter_newton = newton::newton_solver(X, fn);
        fn(X, F, J);
        THEN("find a solution") {
            CAPTURE(iter_newton);
            CAPTURE(X);
            REQUIRE(iter_newton > 0);
            REQUIRE(F.norm() < max_error_norm);
        }
    }

    GIVEN("system of 3 non-linear eqs") {
        struct functor {
            double _x_old = 0.5;
            double _y_old = -231.5;
            double _z_old = 1.4565;
            double dt = 0.2;
            double a = 0.2354;
            double b = 436.2;
            double d = 23.1;
            double e = 0.01;
            double z = 0.99;
            void operator()(const Eigen::Matrix<double, 3, 1>& X,
                            Eigen::Matrix<double, 3, 1>& F,
                            Eigen::Matrix<double, 3, 3>& J) const {
                F(0) = -(-_x_old - dt * (a * std::pow(X[0], 2) + X[1]) + X[0]);
                F(1) = -(_y_old - dt * (a * X[0] + b * X[1] + d) + X[1]);
                F(2) = -(_z_old - dt * (e * z - 3.0 * X[0] + 2.0 * X[1]) + X[2]);
                J(0, 0) = 2.0 * a * dt * X[0] - 1.0;
                J(0, 1) = dt;
                J(0, 2) = 0;
                J(1, 0) = a * dt;
                J(1, 1) = b * dt - 1.0;
                J(1, 2) = 0;
                J(2, 0) = -3.0 * dt;
                J(2, 1) = 2.0 * dt;
                J(2, 2) = dt * e - 1.0;
            }
        };
        Eigen::Matrix<double, 3, 1> X{0.21231, 0.4435, -0.11537};
        Eigen::Matrix<double, 3, 1> F;
        Eigen::Matrix<double, 3, 3> J;
        functor fn;
        int iter_newton = newton::newton_solver(X, fn);
        fn(X, F, J);
        THEN("find a solution") {
            CAPTURE(iter_newton);
            CAPTURE(X);
            REQUIRE(iter_newton > 0);
            REQUIRE(F.norm() < max_error_norm);
        }
    }

    GIVEN("system of 4 non-linear eqs") {
        struct functor {
            double _X0_old = 1.2345;
            double _X1_old = 1.2345;
            double _X2_old = 1.2345;
            double _X3_old = 1.2345;
            double dt = 0.2;
            void operator()(const Eigen::Matrix<double, 4, 1>& X,
                            Eigen::Matrix<double, 4, 1>& F,
                            Eigen::Matrix<double, 4, 4>& J) const {
                F[0] = -(-3.0 * X[0] * X[2] * dt + X[0] - _X0_old + 2.0 * dt / X[1]);
                F[1] = -(X[1] - _X1_old + dt * (4.0 * X[0] - 6.2 * X[1] + X[3]));
                F[2] = -((X[2] * (X[2] - _X2_old) - dt * (X[2] * (-1.2 * X[1] + 3.0) + 0.3)) /
                         X[2]);
                F[3] = -(-4.0 * X[0] * X[1] * X[2] * dt + X[3] - _X3_old + 6.0 * dt / X[2]);
                J(0, 0) = 3.0 * X[2] * dt - 1.0;
                J(0, 1) = 2.0 * dt / std::pow(X[1], 2);
                J(0, 2) = 3.0 * X[0] * dt;
                J(0, 3) = 0;
                J(1, 0) = -4.0 * dt;
                J(1, 1) = 6.2 * dt - 1.0;
                J(1, 2) = 0;
                J(1, 3) = -dt;
                J(2, 0) = 0;
                J(2, 1) = -1.2 * dt;
                J(2, 2) = -1.0 - 0.3 * dt / std::pow(X[2], 2);
                J(2, 3) = 0;
                J(3, 0) = 4.0 * X[1] * X[2] * dt;
                J(3, 1) = 4.0 * X[0] * X[2] * dt;
                J(3, 2) = 4.0 * X[0] * X[1] * dt + 6.0 * dt / std::pow(X[2], 2);
                J(3, 3) = -1.0;
            }
        };
        Eigen::Matrix<double, 4, 1> X{0.21231, 0.4435, -0.11537, -0.8124312};
        Eigen::Matrix<double, 4, 1> F;
        Eigen::Matrix<double, 4, 4> J;
        functor fn;
        int iter_newton = newton::newton_solver(X, fn);
        fn(X, F, J);
        THEN("find a solution") {
            CAPTURE(iter_newton);
            CAPTURE(X);
            REQUIRE(iter_newton > 0);
            REQUIRE(F.norm() < max_error_norm);
        }
    }

    GIVEN("system of 5 non-linear eqs") {
        struct functor {
            void operator()(const Eigen::Matrix<double, 5, 1>& X,
                            Eigen::Matrix<double, 5, 1>& F,
                            Eigen::Matrix<double, 5, 5>& J) const {
                F[0] = -3.0 * X[0] * X[2] + X[0] + 2.0 / X[1];
                F[1] = 4.0 * X[0] - 5.2 * X[1] + X[3];
                F[2] = 1.2 * X[1] + X[2] - 3.0 - 0.3 / X[2];
                F[3] = -4.0 * X[0] * X[1] * X[2] + X[3] + 6.0 / X[2];
                F[4] = (-4.0 * X[0] + (X[4] + cos(X[1])) * (X[1] * X[2] - X[3] * X[4])) /
                       (X[1] * X[2] - X[3] * X[4]);
                J(0, 0) = -3.0 * X[2] + 1.0;
                J(0, 1) = -2.0 / std::pow(X[1], 2);
                J(0, 2) = -3.0 * X[0];
                J(0, 3) = 0;
                J(0, 4) = 0;
                J(1, 0) = 4.0;
                J(1, 1) = -5.2;
                J(1, 2) = 0;
                J(1, 3) = 1.0;
                J(1, 4) = 0;
                J(2, 0) = 0;
                J(2, 1) = 1.2;
                J(2, 2) = 1.0 + 0.3 / std::pow(X[2], 2);
                J(2, 3) = 0;
                J(2, 4) = 0;
                J(3, 0) = -4.0 * X[1] * X[2];
                J(3, 1) = -4.0 * X[0] * X[2];
                J(3, 2) = -4.0 * X[0] * X[1] - 6.0 / std::pow(X[2], 2);
                J(3, 3) = 1.0;
                J(3, 4) = 0;
                J(4, 0) = -4.0 / (X[1] * X[2] - X[3] * X[4]);
                J(4, 1) = 4.0 * X[0] * X[2] / std::pow(X[1] * X[2] - X[3] * X[4], 2) -
                          std::sin(X[1]);
                J(4, 2) = 4.0 * X[0] * X[1] / std::pow(X[1] * X[2] - X[3] * X[4], 2);
                J(4, 3) = -4.0 * X[0] * X[4] / std::pow(X[1] * X[2] - X[3] * X[4], 2);
                J(4, 4) = -4.0 * X[0] * X[3] / std::pow(X[1] * X[2] - X[3] * X[4], 2) + 1.0;
            }
        };
        Eigen::Matrix<double, 5, 1> X;
        X << 8.234, -245.46, 123.123, 0.8343, 5.555;
        Eigen::Matrix<double, 5, 1> F;
        Eigen::Matrix<double, 5, 5> J;
        functor fn;
        int iter_newton = newton::newton_solver(X, fn);
        fn(X, F, J);
        THEN("find a solution") {
            CAPTURE(iter_newton);
            CAPTURE(X);
            REQUIRE(iter_newton > 0);
            REQUIRE(F.norm() < max_error_norm);
        }
    }

    GIVEN("system of 10 non-linear eqs") {
        struct functor {
            void operator()(const Eigen::Matrix<double, 10, 1>& X,
                            Eigen::Matrix<double, 10, 1>& F,
                            Eigen::Matrix<double, 10, 10>& J) const {
                F[0] = -3.0 * X[0] * X[1] + X[0] + 2.0 * X[1];
                F[1] = 4.0 * X[0] - 0.29999999999999999 * std::pow(X[1], 2) + X[1];
                F[2] = 2.0 * X[1] + X[2] + 2.0 * X[3] * X[5] * X[7] - 3.0 * X[4] * X[8] - X[5];
                F[3] = 4.0 * X[0] - 0.29999999999999999 * std::pow(X[1], 2) + X[3] -
                       X[4] * X[6] * X[7];
                F[4] = -3.0 * X[0] * X[7] + 2.0 * X[1] - 4.0 * X[3] * X[8] + X[4];
                F[5] = -X[2] * X[5] * X[8] + 4.0 * X[3] - 0.29999999999999999 * X[4] * X[9] + X[5];
                F[6] = -3.0 * X[0] * X[1] - 2.1000000000000001 * X[3] * X[4] * X[5] + X[6] +
                       2.0 * X[8];
                F[7] = 4.0 * X[0] - 0.29999999999999999 * X[6] * X[7] + X[7];
                F[8] = -3.0 * X[0] * X[1] - X[2] * X[3] * X[4] * std::pow(X[5], 2) + 2.0 * X[5] +
                       X[8];
                F[9] = -0.29999999999999999 * X[2] * X[4] + 4.0 * std::pow(X[9], 2) + X[9];
                J(0, 0) = -3.0 * X[1] + 1.0;
                J(0, 1) = -3.0 * X[0] + 2.0;
                J(0, 2) = 0;
                J(0, 3) = 0;
                J(0, 4) = 0;
                J(0, 5) = 0;
                J(0, 6) = 0;
                J(0, 7) = 0;
                J(0, 8) = 0;
                J(0, 9) = 0;
                J(1, 0) = 4.0;
                J(1, 1) = -0.59999999999999998 * X[1] + 1.0;
                J(1, 2) = 0;
                J(1, 3) = 0;
                J(1, 4) = 0;
                J(1, 5) = 0;
                J(1, 6) = 0;
                J(1, 7) = 0;
                J(1, 8) = 0;
                J(1, 9) = 0;
                J(2, 0) = 0;
                J(2, 1) = 2.0;
                J(2, 2) = 1.0;
                J(2, 3) = 2.0 * X[5] * X[7];
                J(2, 4) = -3.0 * X[8];
                J(2, 5) = 2.0 * X[3] * X[7] - 1.0;
                J(2, 6) = 0;
                J(2, 7) = 2.0 * X[3] * X[5];
                J(2, 8) = -3.0 * X[4];
                J(2, 9) = 0;
                J(3, 0) = 4.0;
                J(3, 1) = -0.59999999999999998 * X[1];
                J(3, 2) = 0;
                J(3, 3) = 1.0;
                J(3, 4) = -X[6] * X[7];
                J(3, 5) = 0;
                J(3, 6) = -X[4] * X[7];
                J(3, 7) = -X[4] * X[6];
                J(3, 8) = 0;
                J(3, 9) = 0;
                J(4, 0) = -3.0 * X[7];
                J(4, 1) = 2.0;
                J(4, 2) = 0;
                J(4, 3) = -4.0 * X[8];
                J(4, 4) = 1.0;
                J(4, 5) = 0;
                J(4, 6) = 0;
                J(4, 7) = -3.0 * X[0];
                J(4, 8) = -4.0 * X[3];
                J(4, 9) = 0;
                J(5, 0) = 0;
                J(5, 1) = 0;
                J(5, 2) = -X[5] * X[8];
                J(5, 3) = 4.0;
                J(5, 4) = -0.29999999999999999 * X[9];
                J(5, 5) = -X[2] * X[8] + 1.0;
                J(5, 6) = 0;
                J(5, 7) = 0;
                J(5, 8) = -X[2] * X[5];
                J(5, 9) = -0.29999999999999999 * X[4];
                J(6, 0) = -3.0 * X[1];
                J(6, 1) = -3.0 * X[0];
                J(6, 2) = 0;
                J(6, 3) = -2.1000000000000001 * X[4] * X[5];
                J(6, 4) = -2.1000000000000001 * X[3] * X[5];
                J(6, 5) = -2.1000000000000001 * X[3] * X[4];
                J(6, 6) = 1.0;
                J(6, 7) = 0;
                J(6, 8) = 2.0;
                J(6, 9) = 0;
                J(7, 0) = 4.0;
                J(7, 1) = 0;
                J(7, 2) = 0;
                J(7, 3) = 0;
                J(7, 4) = 0;
                J(7, 5) = 0;
                J(7, 6) = -0.29999999999999999 * X[7];
                J(7, 7) = -0.29999999999999999 * X[6] + 1.0;
                J(7, 8) = 0;
                J(7, 9) = 0;
                J(8, 0) = -3.0 * X[1];
                J(8, 1) = -3.0 * X[0];
                J(8, 2) = -X[3] * X[4] * std::pow(X[5], 2);
                J(8, 3) = -X[2] * X[4] * std::pow(X[5], 2);
                J(8, 4) = -X[2] * X[3] * std::pow(X[5], 2);
                J(8, 5) = -2.0 * X[2] * X[3] * X[4] * X[5] + 2.0;
                J(8, 6) = 0;
                J(8, 7) = 0;
                J(8, 8) = 1.0;
                J(8, 9) = 0;
                J(9, 0) = 0;
                J(9, 1) = 0;
                J(9, 2) = -0.29999999999999999 * X[4];
                J(9, 3) = 0;
                J(9, 4) = -0.29999999999999999 * X[2];
                J(9, 5) = 0;
                J(9, 6) = 0;
                J(9, 7) = 0;
                J(9, 8) = 0;
                J(9, 9) = 8.0 * X[9] + 1.0;
            }
        };

        Eigen::Matrix<double, 10, 1> X;
        X << 8.234, -5.46, 1.123, 0.8343, 5.555, 18.234, -2.46, 0.123, 10.8343, -4.685;
        Eigen::Matrix<double, 10, 1> F;
        Eigen::Matrix<double, 10, 10> J;
        functor fn;
        int iter_newton = newton::newton_solver(X, fn);
        fn(X, F, J);
        THEN("find a solution") {
            CAPTURE(iter_newton);
            CAPTURE(X);
            REQUIRE(iter_newton > 0);
            REQUIRE(F.norm() < max_error_norm);
        }
    }
}
