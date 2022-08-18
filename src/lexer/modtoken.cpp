/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

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
