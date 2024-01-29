#pragma once

#include <cstdint>

#include "RNG.h"
#include "nrnisaac.h"

class Isaac64: public RNG {
  public:
    Isaac64(std::uint32_t seed = 0);
    ~Isaac64();
    std::uint32_t asLong() {
        return nrnisaac_uint32_pick(rng_);
    }
    void reset() {
        nrnisaac_init(rng_, seed_);
    }
    double asDouble() {
        return nrnisaac_dbl_pick(rng_);
    }
    std::uint32_t seed() {
        return seed_;
    }
    void seed(std::uint32_t s) {
        seed_ = s;
        reset();
    }

  private:
    std::uint32_t seed_;
    void* rng_;
    static std::uint32_t cnt_;
};
