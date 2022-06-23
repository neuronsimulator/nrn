#include <RNG_random123.h>

static char initialized = 0;

RNG_random123::RNG_random123() {
    if (!initialized) {
        initialized = 1;
        c = {{}};
        k = {{}};
        longmurng = std::make_unique<r123::MicroURNG<r123::Philox4x32>>(c.incr(), k);
    }
};
