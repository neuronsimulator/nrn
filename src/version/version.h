/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <string>

namespace nmodl {
struct version {
    static const std::string GIT_REVISION;
    static const std::string NMODL_VERSION;
    static std::string to_string() {
        return NMODL_VERSION + " " + GIT_REVISION;
    }
};
}  // namespace nmodl
