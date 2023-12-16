#ifdef __cplusplus

#include <fstream>
#include <string>
#include <cmath>

constexpr double EPSILON = 1.0 / 1024;

bool almost_equal(double a, double b) {
    return std::fabs(a - b) < EPSILON;
}

/// @brief Compared volatges from a csv file against raw vectors of tvec and voltage
/// @param ref_csv
/// @param tvec
/// @param vvec
/// @param n_spikes
/// @return
bool compare_spikes(const char* ref_csv, double* tvec, double* vvec, long n_voltages) {
    std::ifstream ref_file(ref_csv);
    if (!ref_file.is_open()) {
        std::cerr << "Bad ref file: " << ref_csv << std::endl;
        return false;
    }

    std::string line;
    long cur_line = 0;

    while (std::getline(ref_file, line) && cur_line < n_voltages) {
        size_t end;
        auto tref = std::stod(line, &end);
        auto vref = std::strtof(line.c_str() + end + 1, NULL);
        // ref values have exponent around 0, so we compare absolute values
        // and our epsilon is relatively large
        if (!almost_equal(tvec[cur_line], tref)) {
            std::cerr << "DIFF at line " << cur_line << ": " << tvec[cur_line] << "!=" << tref
                      << std::endl;
            return false;
        }
        if (!almost_equal(vvec[cur_line], vref)) {
            std::cerr << "DIFF at line " << cur_line << ": " << vvec[cur_line] << "!=" << vref
                      << std::endl;
            return false;
        }
        ++cur_line;
    }

    if (cur_line != n_voltages) {
        std::cerr << "Unexpected array length: " << cur_line << " (Ref: " << cur_line << ')'
                  << std::endl;
        return false;
    }

    return true;
}
#endif