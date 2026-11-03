#ifdef __cplusplus

#include <array>
#include <fstream>
#include <string>
#include <cmath>

constexpr double EPSILON = 0x1p-16;  // 1>>16 for double

template <size_t T>
bool approximate(const std::array<double, T>& reference, Object* v) {
    long v_size = nrn_vector_capacity(v);
    const double* v_values = nrn_vector_data(v);
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
