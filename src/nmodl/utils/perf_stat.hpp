/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file
 * \brief Implement class for performance statistics
 */

#include <sstream>
#include <string>
#include <vector>

namespace nmodl {
namespace utils {

/**
 * @addtogroup utils
 * @{
 */

/**
 * \struct PerfStat
 * \brief Helper class to collect performance statistics
 *
 * For code generation it is useful to know the performance
 * characteristics of every block in nmodl. The PerfStat class
 * groups performance characteristics of a single block in
 * nmodl.
 */
struct PerfStat {
    /// name for pretty-printing
    std::string title;

    /// write ops
    int n_assign = 0;

    /// basic ops (<= 1 cycle)
    int n_add = 0;
    int n_sub = 0;
    int n_mul = 0;

    /// expensive ops
    int n_div = 0;

    /// expensive functions : commonly
    /// used functions in mod files
    int n_exp = 0;
    int n_log = 0;
    int n_pow = 0;

    /// could be external math funcs
    int n_ext_func_call = 0;

    /// mod functions (before/after inlining)
    int n_int_func_call = 0;

    /// bitwise ops
    int n_and = 0;
    int n_or = 0;

    /// comparisons ops
    int n_gt = 0;
    int n_lt = 0;
    int n_ge = 0;
    int n_le = 0;
    int n_ne = 0;
    int n_ee = 0;

    /// unary ops
    int n_not = 0;
    int n_neg = 0;

    /// conditional ops
    int n_if = 0;
    int n_elif = 0;

    /// expensive : typically access to dynamically allocated memory
    int n_global_read = 0;
    int n_global_write = 0;
    int n_unique_global_read = 0;
    int n_unique_global_write = 0;

    /// cheap : typically local variables in mod file means registers
    int n_local_read = 0;
    int n_local_write = 0;

    /// could be optimized : access to variables that could be read-only
    /// in this case write counts are typically from initialization
    int n_constant_read = 0;
    int n_constant_write = 0;
    int n_unique_constant_read = 0;
    int n_unique_constant_write = 0;

    friend PerfStat operator+(const PerfStat& first, const PerfStat& second);

    void print(std::stringstream& stream);

    std::vector<std::string> keys() const;

    std::vector<std::string> values() const;
};

/** @} */  // end of utils

}  // namespace utils
}  // namespace nmodl
