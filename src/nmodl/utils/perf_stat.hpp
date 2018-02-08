#ifndef _NMODL_PERF_STAT_HPP_
#define _NMODL_PERF_STAT_HPP_

#include <sstream>

/**
 * \class PerfStat
 * \brief Helper class to collect performance statistics
 *
 * For code generation it is useful to know the performance
 * characteristics of every block in nmodl. The PerfStat class
 * groups performance characteristics of a single block in
 * nmodl.
 */

class PerfStat {
  public:
    /// name for pretty-printing
    std::string title;

    /// write ops
    int assign_count = 0;

    /// basic ops (<= 1 cycle)
    int add_count = 0;
    int sub_count = 0;
    int mul_count = 0;

    /// expensive ops
    int div_count = 0;

    /// expensive functions : commonly
    /// used functions in mod files
    int exp_count = 0;
    int log_count = 0;
    int pow_count = 0;
    /// could be external math funcs
    /// or mod functions (before inlining)
    int func_call_count = 0;

    /// bitwise ops
    int and_count = 0;
    int or_count = 0;

    /// comparisons ops
    int gt_count = 0;
    int lt_count = 0;
    int ge_count = 0;
    int le_count = 0;
    int ne_count = 0;
    int ee_count = 0;

    /// unary ops
    int not_count = 0;
    int neg_count = 0;

    /// conditional ops
    int if_count = 0;
    int elif_count = 0;

    /// expensive : typically access to dynamically allocated memory
    int global_read_count = 0;
    int global_write_count = 0;

    /// cheap : typically local variables in mod file means registers
    int local_read_count = 0;
    int local_write_count = 0;

    /// could be optimized : access to variables that could be read-only
    /// in this case write counts are typically from initialization
    int constant_read_count = 0;
    int constant_write_count = 0;

    friend PerfStat operator+(const PerfStat& first, const PerfStat& second);
    void print(std::stringstream& stream);

    std::vector<std::string> keys();
    std::vector<std::string> values();
};

#endif
