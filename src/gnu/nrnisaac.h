#pragma once

#include <cstdint>

void* nrnisaac_new();
void nrnisaac_delete(void* rng);
void nrnisaac_init(void* rng, unsigned long int seed);
double nrnisaac_dbl_pick(void* rng);
std::uint32_t nrnisaac_uint32_pick(void* rng);
