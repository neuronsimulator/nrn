/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "catch/catch.hpp"

#include "parser/nmodl_driver.hpp"
#include "visitors/verbatim_visitor.hpp"

using namespace nmodl;
using namespace visitor;

using nmodl::parser::NmodlDriver;


//=============================================================================
// Verbatim visitor tests
//=============================================================================

std::vector<std::string> run_verbatim_visitor(const std::string& text) {
    NmodlDriver driver;
    auto ast = driver.parse_string(text);

    VerbatimVisitor v;
    v.visit_program(ast.get());
    return v.verbatim_blocks();
}

TEST_CASE("Parse VERBATIM block using Verbatim Visitor") {
    SECTION("Single Block") {
        std::string text = "VERBATIM int a; ENDVERBATIM";
        auto blocks = run_verbatim_visitor(text);

        REQUIRE(blocks.size() == 1);
        REQUIRE(blocks.front() == " int a; ");
    }

    SECTION("Multiple Blocks") {
        std::string text = "VERBATIM int a; ENDVERBATIM VERBATIM float b; ENDVERBATIM";
        auto blocks = run_verbatim_visitor(text);

        REQUIRE(blocks.size() == 2);
        REQUIRE(blocks[0] == " int a; ");
        REQUIRE(blocks[1] == " float b; ");
    }
}
