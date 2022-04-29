/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/
#include "utils/string_utils.hpp"

#include <spdlog/spdlog.h>

#include <limits>
#include <string>

namespace nmodl {
namespace stringutils {

std::string to_string(double value, const std::string& format_spec) {
    // double containing integer value
    if (std::ceil(value) == value &&
        value < static_cast<double>(std::numeric_limits<long long>::max()) &&
        value > static_cast<double>(std::numeric_limits<long long>::min())) {
        return std::to_string(static_cast<long long>(value));
    }

    // actual float value
    return fmt::format(format_spec, value);
}

}  // namespace stringutils
}  // namespace nmodl
