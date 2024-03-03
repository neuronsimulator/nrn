#include "NrnRandom123RNG.hpp"

NrnRandom123::NrnRandom123(std::uint32_t id1, std::uint32_t id2, std::uint32_t id3) {
    s_ = nrnran123_newstream3(id1, id2, id3);
}

NrnRandom123::~NrnRandom123() {
    nrnran123_deletestream(s_);
}

std::uint32_t NrnRandom123::asLong() {
    return nrnran123_ipick(s_);
}

double NrnRandom123::asDouble() {
    return nrnran123_dblpick(s_);
}

void NrnRandom123::reset() {
    nrnran123_setseq(s_, 0, 0);
}

std::uint32_t NrnRandom123::get_seq() const {
    std::uint32_t seq;
    char which;
    nrnran123_getseq(s_, &seq, &which);
    return seq;
}

void NrnRandom123::set_seq(std::uint32_t seq) {
    nrnran123_setseq(s_, seq, 0);
}
