#pragma once
#include <cstdint>

class RNG {
public:
    // Return a long-words word of random bits
    virtual std::uint32_t asLong() = 0;
    virtual void reset() = 0;

    // Return random bits converted to a double
    virtual double asDouble() = 0;
};
