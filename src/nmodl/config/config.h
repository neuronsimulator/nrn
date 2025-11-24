/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * \dir
 * \brief Global project configurations
 *
 * \file
 * \brief Version information and units file path
 */

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "utils/common_utils.hpp"

namespace nmodl {
namespace fs = std::filesystem;

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

    static const std::string_view embedded_nrnunits;

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
        err_msg << "Falling back to embedded nrnunits.lib\n";
        const auto& temp_dir = fs::temp_directory_path();

        const auto& embedded_path = temp_dir / "nrnunits.lib";
        const auto& lock_dir = embedded_path.string() + ".lock";
        while (not fs::create_directory(lock_dir)) {
            // Someone else has the lock, wait
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if (not fs::exists(embedded_path)) {
            auto file = std::ofstream(embedded_path);
            file << embedded_nrnunits;
        }
        fs::remove(lock_dir);
        return embedded_path;
    }
};

struct CMakeInfo {
    static const std::string SHARED_LIBRARY_SUFFIX;
};

}  // namespace nmodl
