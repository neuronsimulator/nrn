#pragma once
#include <cstdint>
#include <limits>

class RNG {
public:
    using result_type = std::uint32_t;

    RNG() = default;
    virtual ~RNG() = default;

    // Return a long-words word of random bits
    virtual result_type asLong() = 0;
    virtual void reset() = 0;

    // Return random bits converted to a double
    virtual double asDouble() = 0;

    static constexpr result_type min() {
        return std::numeric_limits<result_type>::min();
    }
    static constexpr result_type max() {
        return std::numeric_limits<result_type>::max();
    }
    result_type operator()() {
        return asLong();
    }
};
