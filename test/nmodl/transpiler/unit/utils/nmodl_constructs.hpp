/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <map>
#include <string>
#include <vector>

namespace nmodl {
namespace test_utils {

/// represent nmodl test construct
struct NmodlTestCase {
    /// name of the test
    std::string name;

    /// input nmodl construct
    std::string input;

    /// expected nmodl output
    std::string output;

    /// \todo : add associated json (to use in visitor test)

    NmodlTestCase() = delete;

    NmodlTestCase(std::string name, std::string input)
        : name(name)
        , input(input)
        , output(input) {}

    NmodlTestCase(std::string name, std::string input, std::string output)
        : name(name)
        , input(input)
        , output(output) {}
};

/// represent differential equation test construct
struct DiffEqTestCase {
    /// name of the mod file
    std::string name;

    /// differential equation to solve
    std::string equation;

    /// expected solution
    std::string solution;

    /// solve method
    std::string method;
};

extern std::map<std::string, NmodlTestCase> const nmodl_invalid_constructs;
extern std::map<std::string, NmodlTestCase> const nmodl_valid_constructs;
extern std::vector<DiffEqTestCase> const diff_eq_constructs;

}  // namespace test_utils
}  // namespace nmodl
