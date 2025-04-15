/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lexer/modtoken.hpp"

namespace nmodl {

using LocationType = nmodl::parser::location;

std::string ModToken::position() const {
    std::stringstream ss;
    if (external) {
        ss << "EXTERNAL";
    } else if (start_line() == 0) {
        ss << "UNKNOWN";
    } else {
        ss << pos;
    }
    return ss.str();
}

std::ostream& operator<<(std::ostream& stream, const ModToken& mt) {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    stream << std::setw(15) << mt.name << " at [" << mt.position() << "]";
    return stream << " type " << mt.token;
}

ModToken operator+(ModToken const& adder1, ModToken const& adder2) {
    LocationType sum_pos = adder1.pos + adder2.pos;
    ModToken sum(adder1.name, adder1.token, sum_pos);

    return sum;
}

}  // namespace nmodl
