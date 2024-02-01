#pragma once

#include <cstdint>

#include "nrnran123.h"
#include "RNG.h"

class NrnRandom123: public RNG {
  public:
    NrnRandom123(std::uint32_t id1, std::uint32_t id2, std::uint32_t id3 = 0);
    ~NrnRandom123();
    std::uint32_t asLong() override;
    double asDouble() override;
    void reset() override;
    std::uint32_t get_seq() const;
    void set_seq(std::uint32_t seq);
    nrnran123_State* s_;
};
