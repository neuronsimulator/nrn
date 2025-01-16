#pragma once

#include <cstdint>

#include "nrnran123.h"
#include "RNG.h"

class NrnRandom123: public RNG {
  public:
    NrnRandom123(std::uint32_t id1, std::uint32_t id2, std::uint32_t id3 = 0);
    ~NrnRandom123();
    std::uint32_t asLong() {
        return nrnran123_ipick(s_);
    }
    double asDouble() {
        return nrnran123_dblpick(s_);
    }
    void reset() {
        nrnran123_setseq(s_, 0, 0);
    }
    nrnran123_State* s_;
};
