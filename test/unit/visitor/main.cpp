/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#define CATCH_CONFIG_RUNNER

#include <catch/catch.hpp>

#include "pybind/pyembed.hpp"
#include "utils/logger.hpp"

using namespace nmodl;

int main(int argc, char* argv[]) {
    // initialize python interpreter once for entire catch executable
    nmodl::pybind_wrappers::EmbeddedPythonLoader::get_instance().api()->initialize_interpreter();
    // enable verbose logger output
    logger->set_level(spdlog::level::debug);
    // run all catch tests
    int result = Catch::Session().run(argc, argv);
    nmodl::pybind_wrappers::EmbeddedPythonLoader::get_instance().api()->finalize_interpreter();
    return result;
}
