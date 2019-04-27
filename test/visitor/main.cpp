/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#define CATCH_CONFIG_RUNNER

#include "catch/catch.hpp"
#include "utils/logger.hpp"
#include <pybind11/embed.h>

using namespace nmodl;

int main(int argc, char* argv[]) {
    // initialize python interpreter once for entire catch executable
    pybind11::scoped_interpreter guard{};
    // enable verbose logger output
    logger->set_level(spdlog::level::debug);
    // run all catch tests
    int result = Catch::Session().run(argc, argv);
    return result;
}
