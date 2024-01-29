#include "Isaac64RNG.hpp"

uint32_t Isaac64::cnt_ = 0;

Isaac64::Isaac64(std::uint32_t seed) {
    if (cnt_ == 0) {
        cnt_ = 0xffffffff;
    }
    --cnt_;
    seed_ = seed;
    if (seed_ == 0) {
        seed_ = cnt_;
    }
    rng_ = nrnisaac_new();
    reset();
}

Isaac64::~Isaac64() {
    nrnisaac_delete(rng_);
}
