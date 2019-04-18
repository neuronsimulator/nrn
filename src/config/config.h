/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <fstream>
#include <string>
#include <vector>

namespace nmodl {

/**
 * \brief Project version information
 */
struct version {
    /// git revision id
    static const std::string GIT_REVISION;

    /// project tagged version in the cmake
    static const std::string NMODL_VERSION;

    /// return version string (version + git id) as a string
    static std::string to_string() {
        return NMODL_VERSION + " " + GIT_REVISION;
    }
};

/**
 * \brief Information about unit specification with nrnunits.lib
 */
struct NrnUnitsLib {
    /// paths where nrnunits.lib can be found
    static const std::vector<std::string> NRNUNITSLIB_PATH;

    /// from possible paths, return one that exists
    static std::string get_path() {
        for (const auto& path: NRNUNITSLIB_PATH) {
            std::ifstream f(path.c_str());
            if (f.good()) {
                return path;
            }
        }
        throw std::runtime_error("Could not found nrnunits.lib");
    }
};

}  // namespace nmodl
