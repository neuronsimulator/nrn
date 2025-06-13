#include "MCellRan4RNG.hpp"

MCellRan4::MCellRan4(std::uint32_t ihigh, std::uint32_t ilow) {
    ++cnt_;
    ilow_ = ilow;
    ihigh_ = ihigh;
    if (ihigh_ == 0) {
        ihigh_ = cnt_;
        ihigh_ = (std::uint32_t) asLong();
    }
    orig_ = ihigh_;
}
MCellRan4::~MCellRan4() {}

std::uint32_t MCellRan4::cnt_ = 0;
