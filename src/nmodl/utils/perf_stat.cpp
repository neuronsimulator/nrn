#include <iostream>

#include <vector>

#include "utils/perf_stat.hpp"
#include "utils/table_data.hpp"

PerfStat operator+(const PerfStat& first, const PerfStat& second) {
    PerfStat result;

    result.assign_count = first.assign_count + second.assign_count;

    result.add_count = first.add_count + second.add_count;
    result.sub_count = first.sub_count + second.sub_count;
    result.mul_count = first.mul_count + second.mul_count;
    result.div_count = first.div_count + second.div_count;

    result.exp_count = first.exp_count + second.exp_count;
    result.log_count = first.log_count + second.log_count;
    result.pow_count = first.pow_count + second.pow_count;
    result.func_call_count = first.func_call_count + second.func_call_count;

    result.and_count = first.and_count + second.and_count;
    result.or_count = first.or_count + second.or_count;

    result.gt_count = first.gt_count + second.gt_count;
    result.lt_count = first.lt_count + second.lt_count;
    result.ge_count = first.ge_count + second.ge_count;
    result.le_count = first.le_count + second.le_count;
    result.ne_count = first.ne_count + second.ne_count;
    result.ee_count = first.ee_count + second.ee_count;

    result.not_count = first.not_count + second.not_count;
    result.neg_count = first.neg_count + second.neg_count;

    result.if_count = first.if_count + second.if_count;
    result.elif_count = first.elif_count + second.elif_count;

    result.global_read_count = first.global_read_count + second.global_read_count;
    result.global_write_count = first.global_write_count + second.global_write_count;
    result.local_read_count = first.local_read_count + second.local_read_count;
    result.local_write_count = first.local_write_count + second.local_write_count;

    result.constant_read_count = first.constant_read_count + second.constant_read_count;
    result.constant_write_count = first.constant_write_count + second.constant_write_count;
    return result;
}

void PerfStat::print(std::stringstream& stream) {
    TableData table;
    table.headers = keys();
    table.rows.push_back(values());
    if (!title.empty()) {
        table.title = title;
    }
    table.print(stream);
}

std::vector<std::string> PerfStat::keys() {
    return {"+",     "-",     "x",     "/",    "exp",     "GM(R)", "GM(W)",      "CM(R)",
            "CM(W)", "LM(R)", "LM(W)", "call", "compare", "unary", "conditional"};
}

std::vector<std::string> PerfStat::values() {
    std::vector<std::string> row;

    int compares = gt_count + lt_count + ge_count + le_count + ne_count + ee_count;
    int conditionals = if_count + elif_count;

    row.push_back(std::to_string(add_count));
    row.push_back(std::to_string(sub_count));
    row.push_back(std::to_string(mul_count));
    row.push_back(std::to_string(div_count));
    row.push_back(std::to_string(exp_count));

    row.push_back(std::to_string(global_read_count));
    row.push_back(std::to_string(global_write_count));
    row.push_back(std::to_string(constant_read_count));
    row.push_back(std::to_string(constant_write_count));
    row.push_back(std::to_string(local_read_count));
    row.push_back(std::to_string(local_write_count));
    row.push_back(std::to_string(func_call_count));

    row.push_back(std::to_string(compares));
    row.push_back(std::to_string(not_count + neg_count));
    row.push_back(std::to_string(conditionals));

    return row;
}
