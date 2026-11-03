/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

namespace nmodl {
namespace pybind_wrappers {


void initialize_interpreter_func();
void finalize_interpreter_func();

std::tuple<std::vector<std::string>, std::vector<std::string>, std::string>
call_solve_linear_system(const std::vector<std::string>& eq_system,
                         const std::vector<std::string>& state_vars,
                         const std::set<std::string>& vars,
                         bool small_system,
                         bool elimination,
                         const std::string& tmp_unique_prefix,
                         const std::set<std::string>& function_calls);

std::tuple<std::vector<std::string>, std::string> call_solve_nonlinear_system(
    const std::vector<std::string>& eq_system,
    const std::vector<std::string>& state_vars,
    const std::set<std::string>& vars,
    const std::set<std::string>& function_calls);

std::tuple<std::string, std::string> call_diffeq_solver(const std::string& node_as_nmodl,
                                                        const std::string& dt_var,
                                                        const std::set<std::string>& vars,
                                                        bool use_pade_approx,
                                                        const std::set<std::string>& function_calls,
                                                        const std::string& method);

std::tuple<std::string, std::string> call_analytic_diff(
    const std::vector<std::string>& expressions,
    const std::set<std::string>& used_names_in_block);


/// \brief Differentiates an expression with respect to a variable
/// \param expression The expression we want to differentiate
/// \param variable   The name of the independent variable we are differentiating against
/// \param index_vars A set of array (indexable) variables that appear in \ref expression
/// \return The tuple (solution, exception)
std::tuple<std::string, std::string> call_diff2c(
    const std::string& expression,
    const std::pair<std::string, std::optional<int>>& variable,
    const std::unordered_set<std::string>& indexed_vars = {});

struct pybind_wrap_api {
    decltype(&initialize_interpreter_func) initialize_interpreter;
    decltype(&finalize_interpreter_func) finalize_interpreter;
    decltype(&call_solve_nonlinear_system) solve_nonlinear_system;
    decltype(&call_solve_linear_system) solve_linear_system;
    decltype(&call_diffeq_solver) diffeq_solver;
    decltype(&call_analytic_diff) analytic_diff;
    decltype(&call_diff2c) diff2c;
};

#ifdef _WIN32
#define NMODL_EXPORT
#else
#define NMODL_EXPORT __attribute__((visibility("default")))
#endif

extern "C" {
pybind_wrap_api nmodl_init_pybind_wrapper_api() noexcept;
}


}  // namespace pybind_wrappers
}  // namespace nmodl
