#pragma once

#include <memory>

#include <Random123/philox.h>
#include <Random123/MicroURNG.hpp>

//
// Base class for Random Number Generators using Random123.
//
class RNG_random123 {
    r123::Philox4x32::ctr_type c;
    r123::Philox4x32::key_type k;
    std::shared_ptr<r123::MicroURNG<r123::Philox4x32>> longmurng;
public:
    RNG_random123();
    std::shared_ptr<r123::MicroURNG<r123::Philox4x32>> get_random_generator() {
        return longmurng;
    }
};
