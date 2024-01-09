#ifdef __cplusplus

#include <fstream>
#include <string>
#include <cmath>

constexpr double EPSILON = 0x1p-16;  // 1>>16 for double

inline bool approximate(const std::initializer_list<double>& reference, Object* v) {
    long v_size = nrn_vector_size(v);
    const double* v_values = nrn_vector_data(v);
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
