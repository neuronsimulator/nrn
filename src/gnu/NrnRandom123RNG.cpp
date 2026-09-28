#include "NrnRandom123RNG.hpp"

NrnRandom123::NrnRandom123(std::uint32_t id1, std::uint32_t id2, std::uint32_t id3) {
    s_ = nrnran123_newstream3(id1, id2, id3);
}

NrnRandom123::~NrnRandom123() {
    nrnran123_deletestream(s_);
}
