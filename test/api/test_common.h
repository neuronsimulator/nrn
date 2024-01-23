#ifdef __cplusplus

#include <array>
#include <fstream>
#include <string>
#include <cmath>

constexpr double EPSILON = 0x1p-16;  // 1>>16 for double

/// Compare two doubles. Mitigate accumulated errors by updating a drift
inline bool almost_equal(double a, double b, double* drift) {
    double diff = b - a;
    bool is_ok = std::fabs(diff - *drift) < EPSILON;
    *drift = diff;
    return is_ok;
}

/// @brief Compared volatges from a csv file against raw vectors of tvec and voltage
bool compare_spikes(const char* ref_csv, double* tvec, double* vvec, long n_voltages) {
    std::ifstream ref_file(ref_csv);
    if (!ref_file.is_open()) {
        std::cerr << "Bad ref file: " << ref_csv << std::endl;
        return false;
    }

    std::string line;
    long cur_line = 0;
    double t_drift = 0.;
    double v_drift = 0.;

    while (std::getline(ref_file, line) && cur_line < n_voltages) {
        size_t end;
        auto tref = std::stod(line, &end);
        auto vref = std::strtof(line.c_str() + end + 1, NULL);
        // ref values have exponent around 0, so we compare absolute values
        // and our epsilon is relatively large
        if (!almost_equal(tvec[cur_line], tref, &t_drift)) {
            std::cerr << "DIFF at line " << cur_line << ": " << tvec[cur_line] << "!=" << tref
                      << std::endl;
            return false;
        }
        if (!almost_equal(vvec[cur_line], vref, &v_drift)) {
            std::cerr << "DIFF at line " << cur_line << ": " << vvec[cur_line] << "!=" << vref
                      << " (Drift=" << v_drift << ')' << std::endl;
            return false;
        }
        ++cur_line;
    }

    if (cur_line != n_voltages) {
        std::cerr << "Unexpected array length: " << cur_line << " (Ref: " << n_voltages << ')'
                  << std::endl;
        return false;
    }

    return true;
}

template <size_t T>
bool approximate(const std::array<double, T>& reference, Object* v) {
    long v_size = nrn_vector_capacity(v);
    double* v_values = nrn_vector_data(v);
    if (v_size != reference.size()) {
        std::cerr << "Bad array length: " << v_size << "!=" << reference.size() << std::endl;
        return false;
    }
    for (int i = 0; i < v_size; i++) {
        // Uncomment to create ref: Diplay EXACT doubles (no conversion to decimal)
        // printf("Vec[%d] %a\n", i, v_values[i]);
        if (std::fabs(v_values[i] - reference[i]) > EPSILON) {
            std::cerr << "DIFF at line " << i << ": " << v_values[i] << "!=" << reference[i]
                      << std::endl;
            return false;
        }
    }
    return true;
}
#endif
