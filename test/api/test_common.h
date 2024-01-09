#ifdef __cplusplus

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
inline bool compare_spikes(const char* ref_csv, double* tvec, double* vvec, long n_voltages) {
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

inline bool approximate(const std::initializer_list<double>& reference, Object* v) {
    long v_size = nrn_vector_size(v);
    double* v_values = nrn_vector_data(v);
    if (v_size != reference.size()) {
        std::cerr << "Bad array length: " << v_size << "!=" << reference.size() << std::endl;
        return false;
    }
    auto* ref = reference.begin();
    for (int i = 0; i < v_size; i++, ref++) {
        // Uncomment to create ref: Diplay EXACT doubles (no conversion to decimal)
        // printf("Vec[%d] %a\n", i, v_values[i]);
        if (std::fabs(v_values[i] - *ref) > EPSILON) {
            std::cerr << "DIFF at line " << i << ": " << v_values[i] << "!=" << *ref << std::endl;
            return false;
        }
    }
    return true;
}
#endif

// --- String Helpers (Neuron doesnt handle char* strings ? yet)

inline double nrn_function_call_s(const char* func_name, const char* v) {
    char* temp_str = strdup(v);
    auto x = nrn_function_call(func_name, NRN_ARG_STR_PTR, &temp_str);
    free(temp_str);
    return x;
}

inline Object* nrn_object_new_s(const char* cls_name, const char* v) {
    char* temp_str = strdup(v);
    auto x = nrn_object_new(cls_name, NRN_ARG_STR_PTR, &temp_str);
    free(temp_str);
    return x;
}

inline decltype(auto) nrn_method_call_s(Object* obj, const char* method_name, const char* v) {
    char* temp_str = strdup(v);
    auto x = nrn_method_call(obj, method_name, NRN_ARG_STR_PTR, &temp_str);
    free(temp_str);
    return x;
}
