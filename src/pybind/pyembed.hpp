/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "pybind11/embed.h"

namespace nmodl {
namespace pybind_wrappers {


struct PythonExecutor {
    virtual ~PythonExecutor() {}

    virtual void operator()() = 0;
};


struct SolveLinearSystemExecutor: public PythonExecutor {
    // input
    std::vector<std::string> eq_system;
    std::vector<std::string> state_vars;
    std::set<std::string> vars;
    bool small_system;
    bool elimination;
    // This is used only if elimination is true. It gives the root for the tmp variables
    std::string tmp_unique_prefix;
    std::set<std::string> function_calls;
    // output
    // returns a vector of solutions, i.e. new statements to add to block:
    std::vector<std::string> solutions;
    // and a vector of new local variables that need to be declared in the block:
    std::vector<std::string> new_local_vars;
    // may also return a python exception message:
    std::string exception_message;
    // executor function
    virtual void operator()() override;
};


struct SolveNonLinearSystemExecutor: public PythonExecutor {
    // input
    std::vector<std::string> eq_system;
    std::vector<std::string> state_vars;
    std::set<std::string> vars;
    std::set<std::string> function_calls;
    // output
    // returns a vector of solutions, i.e. new statements to add to block:
    std::vector<std::string> solutions;
    // may also return a python exception message:
    std::string exception_message;

    // executor function
    virtual void operator()() override;
};


struct DiffeqSolverExecutor: public PythonExecutor {
    // input
    std::string node_as_nmodl;
    std::string dt_var;
    std::set<std::string> vars;
    bool use_pade_approx;
    std::set<std::string> function_calls;
    std::string method;
    // output
    // returns  solution, i.e. new statement to add to block:
    std::string solution;
    // may also return a python exception message:
    std::string exception_message;

    // executor function
    virtual void operator()() override;
};


struct AnalyticDiffExecutor: public PythonExecutor {
    // input
    std::vector<std::string> expressions;
    std::set<std::string> used_names_in_block;
    // output
    // returns  solution, i.e. new statement to add to block:
    std::string solution;
    // may also return a python exception message:
    std::string exception_message;

    // executor function
    virtual void operator()() override;
};


SolveLinearSystemExecutor* create_sls_executor_func();
SolveNonLinearSystemExecutor* create_nsls_executor_func();
DiffeqSolverExecutor* create_des_executor_func();
AnalyticDiffExecutor* create_ads_executor_func();
void destroy_sls_executor_func(SolveLinearSystemExecutor* exec);
void destroy_nsls_executor_func(SolveNonLinearSystemExecutor* exec);
void destroy_des_executor_func(DiffeqSolverExecutor* exec);
void destroy_ads_executor_func(AnalyticDiffExecutor* exec);

void initialize_interpreter_func();
void finalize_interpreter_func();

struct pybind_wrap_api {
    decltype(&initialize_interpreter_func) initialize_interpreter;
    decltype(&finalize_interpreter_func) finalize_interpreter;
    decltype(&create_sls_executor_func) create_sls_executor;
    decltype(&create_nsls_executor_func) create_nsls_executor;
    decltype(&create_des_executor_func) create_des_executor;
    decltype(&create_ads_executor_func) create_ads_executor;
    decltype(&destroy_sls_executor_func) destroy_sls_executor;
    decltype(&destroy_nsls_executor_func) destroy_nsls_executor;
    decltype(&destroy_des_executor_func) destroy_des_executor;
    decltype(&destroy_ads_executor_func) destroy_ads_executor;
};


/**
 * A singleton class handling access to the pybind_wrap_api struct
 *
 * This class manages the runtime loading of the libpython so/dylib file and the python binding
 * wrapper library and provides access to the API wrapper struct that can be used to access the
 * pybind11 embedded python functionality.
 */
class EmbeddedPythonLoader {
  public:
    /**
     * Construct (if not already done) and get the only instance of this class
     *
     * @return the EmbeddedPythonLoader singleton instance
     */
    static EmbeddedPythonLoader& get_instance() {
        static EmbeddedPythonLoader instance;

        return instance;
    }

    EmbeddedPythonLoader(const EmbeddedPythonLoader&) = delete;
    void operator=(const EmbeddedPythonLoader&) = delete;


    /**
     * Get a pointer to the pybind_wrap_api struct
     *
     * Get access to the container struct for the pointers to the functions in the wrapper library.
     * @return a pybind_wrap_api pointer
     */
    const pybind_wrap_api* api();

    ~EmbeddedPythonLoader() {
        unload();
    }

  private:
    pybind_wrap_api* wrappers = nullptr;

    void* pylib_handle = nullptr;
    void* pybind_wrapper_handle = nullptr;

    bool have_wrappers();
    void load_libraries();
    void populate_symbols();
    void unload();

    EmbeddedPythonLoader() {
        if (!have_wrappers()) {
            load_libraries();
            populate_symbols();
        }
    }
};


pybind_wrap_api init_pybind_wrap_api() noexcept;

}  // namespace pybind_wrappers
}  // namespace nmodl
