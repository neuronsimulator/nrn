/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define Py_LIMITED_API 0x030C0000
#include <Python.h>

#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include <fmt/format.h>
#include <nanobind/eval.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/set.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/unordered_set.h>
#include <nanobind/stl/vector.h>

#include "codegen/codegen_naming.hpp"
#include "pybind/ode_py.hpp"
#include "pybind/pyembed.hpp"
#include "pybind/wrapper.hpp"
#include "utils/common_utils.hpp"

namespace fs = std::filesystem;
namespace nb = nanobind;

namespace nmodl {
namespace pybind_wrappers {

static bool s_owned_interpreter = false;

static void exec_in(const std::string& code, const nb::dict& locals) {
    nb::exec(nb::str(code.c_str()), locals, locals);
}

// This wrapper is used for obtaining better coverage in `ode.py`.
// Since we embed the `ode.py` as a string, there is no way to check what was covered via running
// pytest or similar. Instead, we use the coverage.py API directly.
static void run_python_script(const std::string& script, const nb::dict& locals) {
#ifdef NRN_ENABLE_COVERAGE
    const auto& suffix =
        nmodl::utils::generate_random_string(20, nmodl::utils::UseNumbersInString::WithoutNumbers);

    exec_in(fmt::format(R"(
import coverage
cov = coverage.Coverage(data_suffix='{}')
cov.start()
)",
                        suffix),
            locals);
    const auto& code_with_mapping = std::string("exec(compile(r'''" + ode_py + script + "''', '" +
                                                ode_py_path + "', 'exec'))");
    exec_in(code_with_mapping, locals);
#else
    exec_in(ode_py + script, locals);
#endif

#ifdef NRN_ENABLE_COVERAGE
    const auto& path = fs::current_path() / fmt::format("coverage_{}.xml", suffix);
    exec_in(fmt::format(R"(
cov.stop()
cov.save()
# Check if we have any coverage data
data = cov.get_data()
if data.measured_files():
    cov.xml_report(outfile='{}')
)",
                        path.string()),
            locals);
#endif
}

std::tuple<std::vector<std::string>, std::vector<std::string>, std::string>
call_solve_linear_system(const std::vector<std::string>& eq_system,
                         const std::vector<std::string>& state_vars,
                         const std::set<std::string>& vars,
                         bool small_system,
                         bool elimination,
                         const std::string& tmp_unique_prefix,
                         const std::set<std::string>& function_calls) {
    nb::dict locals;
    locals["eq_strings"] = eq_system;
    locals["state_vars"] = state_vars;
    locals["vars"] = vars;
    locals["small_system"] = small_system;
    locals["do_cse"] = elimination;
    locals["function_calls"] = function_calls;
    locals["tmp_unique_prefix"] = tmp_unique_prefix;
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
    run_python_script(script, locals);
    auto solutions = nb::cast<std::vector<std::string>>(locals["solutions"]);
    auto new_local_vars = nb::cast<std::vector<std::string>>(locals["new_local_vars"]);
    auto exception_message = nb::cast<std::string>(locals["exception_message"]);

    return {std::move(solutions), std::move(new_local_vars), std::move(exception_message)};
}


std::tuple<std::vector<std::string>, std::string> call_solve_nonlinear_system(
    const std::vector<std::string>& eq_system,
    const std::vector<std::string>& state_vars,
    const std::set<std::string>& vars,
    const std::set<std::string>& function_calls) {
    nb::dict locals;
    locals["equation_strings"] = eq_system;
    locals["state_vars"] = state_vars;
    locals["vars"] = vars;
    locals["function_calls"] = function_calls;
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
    exception_message = traceback.format_exc()
)";

    run_python_script(script, locals);
    auto solutions = nb::cast<std::vector<std::string>>(locals["solutions"]);
    auto exception_message = nb::cast<std::string>(locals["exception_message"]);

    return {std::move(solutions), std::move(exception_message)};
}


std::tuple<std::string, std::string> call_diffeq_solver(const std::string& node_as_nmodl,
                                                        const std::string& dt_var,
                                                        const std::set<std::string>& vars,
                                                        bool use_pade_approx,
                                                        const std::set<std::string>& function_calls,
                                                        const std::string& method) {
    nb::dict locals;
    locals["equation_string"] = node_as_nmodl;
    locals["dt_var"] = dt_var;
    locals["vars"] = vars;
    locals["use_pade_approx"] = use_pade_approx;
    locals["function_calls"] = function_calls;

    if (method == codegen::naming::EULER_METHOD) {
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

        run_python_script(script, locals);
    } else if (method == codegen::naming::CNEXP_METHOD) {
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

        run_python_script(script, locals);
    } else {
        return {};
    }
    auto solution = nb::cast<std::string>(locals["solution"]);
    auto exception_message = nb::cast<std::string>(locals["exception_message"]);

    return {std::move(solution), std::move(exception_message)};
}


std::tuple<std::string, std::string> call_analytic_diff(
    const std::vector<std::string>& expressions,
    const std::set<std::string>& used_names_in_block) {
    nb::dict locals;
    locals["expressions"] = expressions;
    locals["vars"] = used_names_in_block;
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

    run_python_script(script, locals);

    auto solution = nb::cast<std::string>(locals["solution"]);
    auto exception_message = nb::cast<std::string>(locals["exception_message"]);

    return {std::move(solution), std::move(exception_message)};
}

std::tuple<std::string, std::string> call_diff2c(
    const std::string& expression,
    const std::pair<std::string, std::optional<int>>& variable,
    const std::unordered_set<std::string>& indexed_vars) {
    std::string statements;
    for (const auto& var: indexed_vars) {
        statements += fmt::format("_allvars.append(sp.IndexedBase('{}', shape=[1]))\n", var);
    }
    auto [name, property] = variable;
    if (property.has_value()) {
        name = fmt::format("sp.IndexedBase('{}', shape=[1])", name);
        statements += fmt::format("_allvars.append({})", name);
    } else {
        name = fmt::format("'{}'", name);
    }
    nb::dict locals;
    locals["expression"] = expression;
    std::string script =
        fmt::format(R"(
_allvars = []
{}
variable = {}
exception_message = ""
try:
    solution = differentiate2c(expression,
                               variable,
                               _allvars,
               )
except Exception as e:
    # if we fail, fail silently and return empty string
    solution = ""
    exception_message = str(e)
)",
                    statements,
                    property.has_value() ? fmt::format("{}[{}]", name, property.value()) : name);

    run_python_script(script, locals);

    auto solution = nb::cast<std::string>(locals["solution"]);
    auto exception_message = nb::cast<std::string>(locals["exception_message"]);

    return {std::move(solution), std::move(exception_message)};
}

void initialize_interpreter_func() {
    if (!Py_IsInitialized()) {
        Py_InitializeEx(1);
        s_owned_interpreter = true;
    }
}

void finalize_interpreter_func() {
    if (s_owned_interpreter && Py_IsInitialized()) {
        if (PyErr_Occurred()) {
            PyErr_Clear();
        }
        Py_Finalize();
        s_owned_interpreter = false;
    }
}

extern "C" {
NMODL_EXPORT pybind_wrap_api nmodl_init_pybind_wrapper_api() noexcept {
    return {&nmodl::pybind_wrappers::initialize_interpreter_func,
            &nmodl::pybind_wrappers::finalize_interpreter_func,
            &call_solve_nonlinear_system,
            &call_solve_linear_system,
            &call_diffeq_solver,
            &call_analytic_diff,
            &call_diff2c};
}
}

}  // namespace pybind_wrappers
}  // namespace nmodl
