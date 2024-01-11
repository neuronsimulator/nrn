#include <string.h>

#ifdef __cplusplus
#include <iostream>
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

// --- String Helpers (Neuron doesnt handle char* strings ? yet)

inline NrnResult nrn_function_call_s(const char* func_name, const char* v) {
    char* temp_str = strdup(v);
    NrnResult x = nrn_function_call(func_name, NRN_ARG_STR_PTR, &temp_str);
    free(temp_str);
    return x;
}

inline Object* nrn_object_new_s(const char* cls_name, const char* v) {
    char* temp_str = strdup(v);
    Object* x = nrn_object_new(cls_name, NRN_ARG_STR_PTR, &temp_str);
    free(temp_str);
    return x;
}

inline NrnResult nrn_method_call_s(Object* obj, const char* method_name, const char* v) {
    char* temp_str = strdup(v);
    NrnResult x = nrn_method_call(obj, method_name, NRN_ARG_STR_PTR, &temp_str);
    free(temp_str);
    return x;
}
