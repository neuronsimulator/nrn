/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \dir
 * \brief Global project configurations
 *
 * \file
 * \brief Version information and units file path
 */

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "utils/common_utils.hpp"

namespace nmodl {

/**
 * \brief Project version information
 */
struct Version {
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
 * \brief Information of units database i.e. `nrnunits.lib`
 */
struct NrnUnitsLib {
    /// paths where nrnunits.lib can be found
    static std::vector<std::string> NRNUNITSLIB_PATH;

    /**
     * Return path of units database file
     */
    static std::string get_path() {
        // first look for NMODLHOME env variable
        if (const char* nmodl_home = std::getenv("NMODLHOME")) {
            auto path = std::string(nmodl_home) + "/share/nmodl/nrnunits.lib";
            NRNUNITSLIB_PATH.emplace(NRNUNITSLIB_PATH.begin(), path);
        }

        // check paths in order and return if found
        for (const auto& path: NRNUNITSLIB_PATH) {
            std::ifstream f(path.c_str());
            if (f.good()) {
                return path;
            }
        }
        std::ostringstream err_msg;
        err_msg << "Could not find nrnunits.lib in any of:\n";
        for (const auto& path: NRNUNITSLIB_PATH) {
            err_msg << path << "\n";
        }
        throw std::runtime_error(err_msg.str());
    }
};

}  // namespace nmodl
