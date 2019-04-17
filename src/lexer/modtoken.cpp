/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "lexer/modtoken.hpp"

namespace nmodl {

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
    stream << std::setw(15) << mt.name << " at [" << mt.position() << "]";
    return stream << " type " << mt.token;
}

}  // namespace nmodl