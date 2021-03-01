/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/


#include <set>
#include <vector>

#include "pybind11/embed.h"
#include "pybind11/stl.h"

#include "codegen/codegen_naming.hpp"
#include "pybind/pyembed.hpp"

namespace py = pybind11;
using namespace py::literals;

namespace nmodl {
namespace pybind_wrappers {

void SolveLinearSystemExecutor::operator()() {
    const auto locals = py::dict("eq_strings"_a = eq_system,
                                 "state_vars"_a = state_vars,
                                 "vars"_a = vars,
                                 "small_system"_a = small_system,
                                 "do_cse"_a = elimination,
                                 "function_calls"_a = function_calls,
                                 "tmp_unique_prefix"_a = tmp_unique_prefix);
    py::exec(R"(
                from nmodl.ode import solve_lin_system
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
                    solutions = [""]
                    new_local_vars = [""]
                    exception_message = str(e)
                )",
             py::globals(),
             locals);
    // returns a vector of solutions, i.e. new statements to add to block:
    solutions = locals["solutions"].cast<std::vector<std::string>>();
    // and a vector of new local variables that need to be declared in the block:
    new_local_vars = locals["new_local_vars"].cast<std::vector<std::string>>();
    // may also return a python exception message:
    exception_message = locals["exception_message"].cast<std::string>();
}


void SolveNonLinearSystemExecutor::operator()() {
    const auto locals = py::dict("equation_strings"_a = eq_system,
                                 "state_vars"_a = state_vars,
                                 "vars"_a = vars,
                                 "function_calls"_a = function_calls);
    py::exec(R"(
                from nmodl.ode import solve_non_lin_system
                exception_message = ""
                try:
                    solutions = solve_non_lin_system(equation_strings,
                                                     state_vars,
                                                     vars,
                                                     function_calls)
                except Exception as e:
                    # if we fail, fail silently and return empty string
                    solutions = [""]
                    new_local_vars = [""]
                    exception_message = str(e)
                )",
             py::globals(),
             locals);
    // returns a vector of solutions, i.e. new statements to add to block:
    solutions = locals["solutions"].cast<std::vector<std::string>>();
    // may also return a python exception message:
    exception_message = locals["exception_message"].cast<std::string>();
}

void DiffeqSolverExecutor::operator()() {
    const auto locals = py::dict("equation_string"_a = node_as_nmodl,
                                 "dt_var"_a = dt_var,
                                 "vars"_a = vars,
                                 "use_pade_approx"_a = use_pade_approx,
                                 "function_calls"_a = function_calls);

    if (method == codegen::naming::EULER_METHOD) {
        // replace x' = f(x) differential equation
        // with forwards Euler timestep:
        // x = x + f(x) * dt
        py::exec(R"(
                from nmodl.ode import forwards_euler2c
                exception_message = ""
                try:
                    solution = forwards_euler2c(equation_string, dt_var, vars, function_calls)
                except Exception as e:
                    # if we fail, fail silently and return empty string
                    solution = ""
                    exception_message = str(e)
            )",
                 py::globals(),
                 locals);
    } else if (method == codegen::naming::CNEXP_METHOD) {
        // replace x' = f(x) differential equation
        // with analytic solution for x(t+dt) in terms of x(t)
        // x = ...
        py::exec(R"(
                from nmodl.ode import integrate2c
                exception_message = ""
                try:
                    solution = integrate2c(equation_string, dt_var, vars,
                                           use_pade_approx)
                except Exception as e:
                    # if we fail, fail silently and return empty string
                    solution = ""
                    exception_message = str(e)
            )",
                 py::globals(),
                 locals);
    } else {
        // nothing to do, but the caller should know.
        return;
    }
    solution = locals["solution"].cast<std::string>();
    exception_message = locals["exception_message"].cast<std::string>();
}

void AnalyticDiffExecutor::operator()() {
    auto locals = py::dict("expressions"_a = expressions, "vars"_a = used_names_in_block);
    py::exec(R"(
                            from nmodl.ode import differentiate2c
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
                                solution = ""
                                exception_message = str(e)
                        )",
             py::globals(),
             locals);
    solution = locals["solution"].cast<std::string>();
    exception_message = locals["exception_message"].cast<std::string>();
}

SolveLinearSystemExecutor* create_sls_executor_func() {
    return new SolveLinearSystemExecutor();
}

SolveNonLinearSystemExecutor* create_nsls_executor_func() {
    return new SolveNonLinearSystemExecutor();
}

DiffeqSolverExecutor* create_des_executor_func() {
    return new DiffeqSolverExecutor();
}

AnalyticDiffExecutor* create_ads_executor_func() {
    return new AnalyticDiffExecutor();
}

void destroy_sls_executor_func(SolveLinearSystemExecutor* exec) {
    delete exec;
}

void destroy_nsls_executor_func(SolveNonLinearSystemExecutor* exec) {
    delete exec;
}

void destroy_des_executor_func(DiffeqSolverExecutor* exec) {
    delete exec;
}

void destroy_ads_executor_func(AnalyticDiffExecutor* exec) {
    delete exec;
}

void initialize_interpreter_func() {
    pybind11::initialize_interpreter(true);
    const auto python_path_cstr = std::getenv("PYTHONPATH");
    if (python_path_cstr) {
        pybind11::module::import("sys").attr("path").cast<pybind11::list>().insert(
            0, python_path_cstr);
    }
}

void finalize_interpreter_func() {
    pybind11::finalize_interpreter();
}

pybind_wrap_api init_pybind_wrap_api() noexcept {
    return {
        &nmodl::pybind_wrappers::initialize_interpreter_func,
        &nmodl::pybind_wrappers::finalize_interpreter_func,
        &nmodl::pybind_wrappers::create_sls_executor_func,
        &nmodl::pybind_wrappers::create_nsls_executor_func,
        &nmodl::pybind_wrappers::create_des_executor_func,
        &nmodl::pybind_wrappers::create_ads_executor_func,
        &nmodl::pybind_wrappers::destroy_sls_executor_func,
        &nmodl::pybind_wrappers::destroy_nsls_executor_func,
        &nmodl::pybind_wrappers::destroy_des_executor_func,
        &nmodl::pybind_wrappers::destroy_ads_executor_func,
    };
}

}  // namespace pybind_wrappers
}  // namespace nmodl


__attribute__((visibility("default"))) nmodl::pybind_wrappers::pybind_wrap_api nmodl_wrapper_api =
    nmodl::pybind_wrappers::init_pybind_wrap_api();
