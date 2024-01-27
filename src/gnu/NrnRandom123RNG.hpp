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
NrnRandom123::NrnRandom123(std::uint32_t id1, std::uint32_t id2, std::uint32_t id3) {
    s_ = nrnran123_newstream3(id1, id2, id3);
}
NrnRandom123::~NrnRandom123() {
    nrnran123_deletestream(s_);
}
