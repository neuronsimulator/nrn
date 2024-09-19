/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "wrapper.hpp"

#include "codegen/codegen_naming.hpp"
#include "pybind/pyembed.hpp"

#include <pybind11/embed.h>
#include <pybind11/stl.h>

#include <set>
#include <vector>

#include "ode_py.hpp"

namespace py = pybind11;
using namespace py::literals;

namespace nmodl {
namespace pybind_wrappers {

std::tuple<std::vector<std::string>, std::vector<std::string>, std::string>
call_solve_linear_system(const std::vector<std::string>& eq_system,
                         const std::vector<std::string>& state_vars,
                         const std::set<std::string>& vars,
                         bool small_system,
                         bool elimination,
                         const std::string& tmp_unique_prefix,
                         const std::set<std::string>& function_calls) {
    const auto locals = py::dict("eq_strings"_a = eq_system,
                                 "state_vars"_a = state_vars,
                                 "vars"_a = vars,
                                 "small_system"_a = small_system,
                                 "do_cse"_a = elimination,
                                 "function_calls"_a = function_calls,
                                 "tmp_unique_prefix"_a = tmp_unique_prefix);
    std::string script = R"(
exception_message = ""
try:
    solutions, new_local_vars = solve_lin_system(eq_strings,
                                                 state_vars,
                                                 vars,
                                                 function_calls,
                                                 tmp_unique_prefix,
                                                 small_system,
                                                 do_cse)
except Exception as e:
    # if we fail, fail silently and return empty string
    import traceback
    solutions = [""]
    new_local_vars = [""]
    exception_message = traceback.format_exc()
)";

    py::exec(nmodl::pybind_wrappers::ode_py + script, locals);
    // returns a vector of solutions, i.e. new statements to add to block:
    auto solutions = locals["solutions"].cast<std::vector<std::string>>();
    // and a vector of new local variables that need to be declared in the block:
    auto new_local_vars = locals["new_local_vars"].cast<std::vector<std::string>>();
    // may also return a python exception message:
    auto exception_message = locals["exception_message"].cast<std::string>();

    return {std::move(solutions), std::move(new_local_vars), std::move(exception_message)};
}


std::tuple<std::vector<std::string>, std::string> call_solve_nonlinear_system(
    const std::vector<std::string>& eq_system,
    const std::vector<std::string>& state_vars,
    const std::set<std::string>& vars,
    const std::set<std::string>& function_calls) {
    const auto locals = py::dict("equation_strings"_a = eq_system,
                                 "state_vars"_a = state_vars,
                                 "vars"_a = vars,
                                 "function_calls"_a = function_calls);
    std::string script = R"(
exception_message = ""
try:
    solutions = solve_non_lin_system(equation_strings,
                                     state_vars,
                                     vars,
                                     function_calls)
except Exception as e:
    # if we fail, fail silently and return empty string
    import traceback
    solutions = [""]
    new_local_vars = [""]
    exception_message = traceback.format_exc()
)";

    py::exec(nmodl::pybind_wrappers::ode_py + script, locals);
    // returns a vector of solutions, i.e. new statements to add to block:
    auto solutions = locals["solutions"].cast<std::vector<std::string>>();
    // may also return a python exception message:
    auto exception_message = locals["exception_message"].cast<std::string>();

    return {std::move(solutions), std::move(exception_message)};
}


std::tuple<std::string, std::string> call_diffeq_solver(const std::string& node_as_nmodl,
                                                        const std::string& dt_var,
                                                        const std::set<std::string>& vars,
                                                        bool use_pade_approx,
                                                        const std::set<std::string>& function_calls,
                                                        const std::string& method) {
    const auto locals = py::dict("equation_string"_a = node_as_nmodl,
                                 "dt_var"_a = dt_var,
                                 "vars"_a = vars,
                                 "use_pade_approx"_a = use_pade_approx,
                                 "function_calls"_a = function_calls);

    if (method == codegen::naming::EULER_METHOD) {
        // replace x' = f(x) differential equation
        // with forwards Euler timestep:
        // x = x + f(x) * dt
        std::string script = R"(
exception_message = ""
try:
    solution = forwards_euler2c(equation_string, dt_var, vars, function_calls)
except Exception as e:
    # if we fail, fail silently and return empty string
    import traceback
    solution = ""
    exception_message = traceback.format_exc()
)";

        py::exec(nmodl::pybind_wrappers::ode_py + script, locals);
    } else if (method == codegen::naming::CNEXP_METHOD) {
        // replace x' = f(x) differential equation
        // with analytic solution for x(t+dt) in terms of x(t)
        // x = ...
        std::string script = R"(
exception_message = ""
try:
    solution = integrate2c(equation_string, dt_var, vars,
                           use_pade_approx)
except Exception as e:
    # if we fail, fail silently and return empty string
    import traceback
    solution = ""
    exception_message = traceback.format_exc()
)";

        py::exec(nmodl::pybind_wrappers::ode_py + script, locals);
    } else {
        // nothing to do, but the caller should know.
        return {};
    }
    auto solution = locals["solution"].cast<std::string>();
    auto exception_message = locals["exception_message"].cast<std::string>();

    return {std::move(solution), std::move(exception_message)};
}


std::tuple<std::string, std::string> call_analytic_diff(
    const std::vector<std::string>& expressions,
    const std::set<std::string>& used_names_in_block) {
    auto locals = py::dict("expressions"_a = expressions, "vars"_a = used_names_in_block);
    std::string script = R"(
exception_message = ""
try:
    rhs = expressions[-1].split("=", 1)[1]
    solution = differentiate2c(rhs,
                               "v",
                               vars,
                               expressions[:-1]
               )
except Exception as e:
    # if we fail, fail silently and return empty string
    import traceback
    solution = ""
    exception_message = traceback.format_exc()
)";

    py::exec(nmodl::pybind_wrappers::ode_py + script, locals);

    auto solution = locals["solution"].cast<std::string>();
    auto exception_message = locals["exception_message"].cast<std::string>();

    return {std::move(solution), std::move(exception_message)};
}


void initialize_interpreter_func() {
    pybind11::initialize_interpreter(true);
}

void finalize_interpreter_func() {
    pybind11::finalize_interpreter();
}

// Prevent mangling for easier `dlsym`.
extern "C" {
NMODL_EXPORT pybind_wrap_api nmodl_init_pybind_wrapper_api() noexcept {
    return {&nmodl::pybind_wrappers::initialize_interpreter_func,
            &nmodl::pybind_wrappers::finalize_interpreter_func,
            &call_solve_nonlinear_system,
            &call_solve_linear_system,
            &call_diffeq_solver,
            &call_analytic_diff};
}
}

}  // namespace pybind_wrappers
}  // namespace nmodl
