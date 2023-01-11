#pragma once
#include "neuron/container/data_handle.hpp"
double* nrn_recalc_ptr(double* old);
/**
 * Placeholder that has been inserted where nrn_recalc_ptr used to be called.
 * The idea here is that nrn_recalc_ptr already showed us the various places
 * that potentially-invalidated pointers are stored, and that eventually it
 * might be useful to visit data_handle in those places to trigger a collapse
 * from "refers to an entry that got deleted" (prints as "died/N") to "null",
 * which would allow neuron::container::detail::garbage to safely be cleaned
 * up. This might not prove to be important. Eventually all places that use the
 * first signature should be migrated to use data handles instead of raw
 * pointers and therefore use the second signature.
 */
inline void nrn_forget_history(double*) {}
inline void nrn_forget_history(neuron::container::data_handle<double>& dh) {
    if (!dh) {
        dh = {};
    }
}
void nrn_register_recalc_ptr_callback(void (*f)());
