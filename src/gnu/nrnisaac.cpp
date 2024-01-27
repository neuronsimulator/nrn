#include <cstdint>
#include "nrnisaac.h"
#include "isaac64.h"

using RNG = struct isaac64_state;

void* nrnisaac_new() {
    return new RNG;
}

void nrnisaac_delete(void* v) {
    delete static_cast<RNG*>(v);
}

void nrnisaac_init(void* v, unsigned long int seed) {
    isaac64_init(static_cast<RNG*>(v), seed);
}

double nrnisaac_dbl_pick(void* v) {
    return isaac64_dbl32(static_cast<RNG*>(v));
}

std::uint32_t nrnisaac_uint32_pick(void* v) {
    return isaac64_uint32(static_cast<RNG*>(v));
}
