/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <catch2/catch.hpp>

#include "ast/program.hpp"
#include "parser/nmodl_driver.hpp"
#include "test/unit/utils/nmodl_constructs.hpp"
#include "visitors/global_var_visitor.hpp"
#include "visitors/nmodl_visitor.hpp"
#include "visitors/perf_visitor.hpp"
#include "visitors/symtab_visitor.hpp"

using namespace nmodl;
using namespace visitor;
using namespace test_utils;

using nmodl::parser::NmodlDriver;
using symtab::syminfo::NmodlType;

//=============================================================================
// GlobalToRange visitor tests
//=============================================================================

std::shared_ptr<ast::Program> run_global_to_var_visitor(const std::string& text) {
    NmodlDriver driver;
    auto ast = driver.parse_string(text);

    SymtabVisitor().visit_program(*ast);
    PerfVisitor().visit_program(*ast);
    GlobalToRangeVisitor(*ast).visit_program(*ast);
    SymtabVisitor().visit_program(*ast);
    return ast;
}

SCENARIO("GLOBAL to RANGE variable transformer", "[visitor][globaltorange]") {
    GIVEN("mod file with GLOBAL variables that are written") {
        std::string input_nmodl = R"(
            NEURON {
                SUFFIX test
                RANGE a, b
                GLOBAL x, y
            }
            ASSIGNED {
                x
            }
            BREAKPOINT {
                x = y
            }
        )";
        auto ast = run_global_to_var_visitor(input_nmodl);
        auto symtab = ast->get_symbol_table();
        THEN("GLOBAL variables that are written are turned to RANGE") {
            /// check for all RANGE variables : old ones + newly converted ones
            auto vars = symtab->get_variables_with_properties(NmodlType::range_var);
            REQUIRE(vars.size() == 3);

            /// x should be converted from GLOBAL to RANGE
            auto x = symtab->lookup("x");
            REQUIRE(x != nullptr);
            REQUIRE(x->has_any_property(NmodlType::range_var) == true);
            REQUIRE(x->has_any_property(NmodlType::global_var) == false);
        }
        THEN("GLOBAL variables that are read only remain GLOBAL") {
            auto vars = symtab->get_variables_with_properties(NmodlType::global_var);
            REQUIRE(vars.size() == 1);
            REQUIRE(vars[0]->get_name() == "y");
        }
    }
}
