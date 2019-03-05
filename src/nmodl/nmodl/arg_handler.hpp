/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <string>
#include <vector>


namespace nmodl {

/**
 * \class ArgumentHandler
 * \brief Parser comamnd line arguments
 */

struct ArgumentHandler {
    /// input files for code generation
    std::vector<std::string> nmodl_files;

    /// host code generation backend
    std::string host_backend;

    /// device code generation backend
    std::string accel_backend;

    /// floating data type to use
    std::string dtype;

    /// memory layout to use
    std::string mlayout;

    /// enable SymPy analytic integration
    bool sympy;

    /// enable Pade approx in SymPy analytic integration
    bool pade_approx;

    /// enable nmodl level inlining
    bool inlining;

    /// enable inlining even if verbatim block exisits
    bool localize_with_verbatim;

    /// enable renaming local variables
    bool local_rename;

    /// enable conversion of RANGE variables to LOCAL
    bool localize;

    /// dump performance statistics
    bool perf_stats;

    /// show symbol table information
    bool show_symtab;

    /// generate nmodl from ast
    bool ast_to_nmodl;

    /// generate json from ast
    bool ast_to_json;

    /// enable verbose (todo: replace by log)
    bool verbose;

    /// enable renaming inside verbatim block
    bool verbatim_rename;

    /// directory for code generation
    std::string output_dir;

    /// directory for intermediate files from code generation
    std::string scratch_dir;

    ArgumentHandler(const int& argc, const char* argv[]);

    bool aos_memory_layout() {
        return mlayout == "AOS";
    }

    bool host_c_backend() {
        return host_backend == "SERIAL";
    }

    bool host_omp_backend() {
        return host_backend == "OPENMP";
    }

    bool host_acc_backend() {
        return host_backend == "OPENACC";
    }

    bool device_cuda_backend() {
        return accel_backend == "CUDA";
    }
};

}  // namespace nmodl
