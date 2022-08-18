/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <iostream>

#include <vector>

#include "utils/perf_stat.hpp"
#include "utils/table_data.hpp"

namespace nmodl {
namespace utils {

PerfStat operator+(const PerfStat& first, const PerfStat& second) {
    PerfStat result;

    result.n_assign = first.n_assign + second.n_assign;

    result.n_add = first.n_add + second.n_add;
    result.n_sub = first.n_sub + second.n_sub;
    result.n_mul = first.n_mul + second.n_mul;
    result.n_div = first.n_div + second.n_div;

    result.n_exp = first.n_exp + second.n_exp;
    result.n_log = first.n_log + second.n_log;
    result.n_pow = first.n_pow + second.n_pow;

    result.n_ext_func_call = first.n_ext_func_call + second.n_ext_func_call;
    result.n_int_func_call = first.n_int_func_call + second.n_int_func_call;

    result.n_and = first.n_and + second.n_and;
    result.n_or = first.n_or + second.n_or;

    result.n_gt = first.n_gt + second.n_gt;
    result.n_lt = first.n_lt + second.n_lt;
    result.n_ge = first.n_ge + second.n_ge;
    result.n_le = first.n_le + second.n_le;
    result.n_ne = first.n_ne + second.n_ne;
    result.n_ee = first.n_ee + second.n_ee;

    result.n_not = first.n_not + second.n_not;
    result.n_neg = first.n_neg + second.n_neg;

    result.n_if = first.n_if + second.n_if;
    result.n_elif = first.n_elif + second.n_elif;

    result.n_global_read = first.n_global_read + second.n_global_read;
    result.n_global_write = first.n_global_write + second.n_global_write;
    result.n_unique_global_read = first.n_unique_global_read + second.n_unique_global_read;
    result.n_unique_global_write = first.n_unique_global_write + second.n_unique_global_write;

    result.n_local_read = first.n_local_read + second.n_local_read;
    result.n_local_write = first.n_local_write + second.n_local_write;

    result.n_constant_read = first.n_constant_read + second.n_constant_read;
    result.n_constant_write = first.n_constant_write + second.n_constant_write;
    result.n_unique_constant_read = first.n_unique_constant_read + second.n_unique_constant_read;
    result.n_unique_constant_write = first.n_unique_constant_write + second.n_unique_constant_write;
    return result;
}

void PerfStat::print(std::stringstream& stream) const {
    TableData table;
    table.headers = keys();
    table.rows.push_back(values());
    if (!title.empty()) {
        table.title = title;
    }
    table.print(stream);
}

std::vector<std::string> PerfStat::keys() {
    return {"+",       "-",       "x",          "/",          "exp",     "log",     "GM-R(T)",
            "GM-R(U)", "GM-W(T)", "GM-W(U)",    "CM-R(T)",    "CM-R(U)", "CM-W(T)", "CM-W(U)",
            "LM-R(T)", "LM-W(T)", "calls(ext)", "calls(int)", "compare", "unary",   "conditional"};
}

std::vector<std::string> PerfStat::values() const {
    std::vector<std::string> row;

    int compares = n_gt + n_lt + n_ge + n_le + n_ne + n_ee;
    int conditionals = n_if + n_elif;

    row.push_back(std::to_string(n_add));
    row.push_back(std::to_string(n_sub));
    row.push_back(std::to_string(n_mul));
    row.push_back(std::to_string(n_div));
    row.push_back(std::to_string(n_exp));
    row.push_back(std::to_string(n_log));

    row.push_back(std::to_string(n_global_read));
    row.push_back(std::to_string(n_unique_global_read));
    row.push_back(std::to_string(n_global_write));
    row.push_back(std::to_string(n_unique_global_write));

    row.push_back(std::to_string(n_constant_read));
    row.push_back(std::to_string(n_unique_constant_read));
    row.push_back(std::to_string(n_constant_write));
    row.push_back(std::to_string(n_unique_constant_write));

    row.push_back(std::to_string(n_local_read));
    row.push_back(std::to_string(n_local_write));

    row.push_back(std::to_string(n_ext_func_call));
    row.push_back(std::to_string(n_int_func_call));

    row.push_back(std::to_string(compares));
    row.push_back(std::to_string(n_not + n_neg));
    row.push_back(std::to_string(conditionals));

    return row;
}

}  // namespace utils
}  // namespace nmodl
