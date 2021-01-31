/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#pragma once

/**
 * \dir
 * \brief Global project configurations
 *
 * \file
 * \brief Version information
 */

#include <string>

namespace coreneuron {

/**
 * \brief Project version information
 */
struct version {
    /// git revision id
    static const std::string GIT_REVISION;

    /// project tagged version in the cmake
    static const std::string CORENEURON_VERSION;

    /// return version string (version + git id) as a string
    static std::string to_string() {
        return CORENEURON_VERSION + " " + GIT_REVISION;
    }
};

}  // namespace coreneuron
