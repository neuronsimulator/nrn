/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstring>

#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

#include "nmodl/utils/logger.hpp"
#include "pybind/pyembed.hpp"

using namespace nmodl;

static bool listing_tests(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (std::strncmp(argv[i], "--list-", 7) == 0) {
            return true;
        }
    }
    return false;
}

int main(int argc, char* argv[]) {
    const bool list_only = listing_tests(argc, argv);
    if (!list_only) {
        nmodl::pybind_wrappers::EmbeddedPythonLoader::get_instance().api().initialize_interpreter();
    }
    logger->set_level(spdlog::level::debug);
    int result = Catch::Session().run(argc, argv);
    if (!list_only) {
        nmodl::pybind_wrappers::EmbeddedPythonLoader::get_instance().api().finalize_interpreter();
    }
    return result;
}
