/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <catch/catch.hpp>

#include "ast/program.hpp"
#include "parser/nmodl_driver.hpp"
#include "test/unit/utils/test_utils.hpp"
#include "visitors/checkparent_visitor.hpp"
#include "visitors/inline_visitor.hpp"
#include "visitors/localize_visitor.hpp"
#include "visitors/symtab_visitor.hpp"
#include "visitors/visitor_utils.hpp"

using namespace nmodl;
using namespace visitor;
using namespace test;
using namespace test_utils;

using ast::AstNodeType;
using nmodl::parser::NmodlDriver;

//=============================================================================
// Passes can run multiple times
//=============================================================================

void run_visitor_passes(const std::string& text) {
    NmodlDriver driver;
    const auto& ast = driver.parse_string(text);
    {
        SymtabVisitor v1;
        InlineVisitor v2;
        LocalizeVisitor v3;
        CheckParentVisitor v4(true);
        v1.visit_program(*ast);
        v2.visit_program(*ast);
        v3.visit_program(*ast);
        v4.check_ast(*ast);
        v4.check_ast(*ast);
        v1.visit_program(*ast);
        v1.visit_program(*ast);
        v4.check_ast(*ast);
        v2.visit_program(*ast);
        v3.visit_program(*ast);
        v2.visit_program(*ast);
        v4.check_ast(*ast);
    }
}


SCENARIO("Running visitor passes multiple times", "[visitor]") {
    GIVEN("A mod file") {
        std::string nmodl_text = R"(
            NEURON {
                RANGE tau
            }

            DERIVATIVE states {
                tau = 11.1
                exp(tau)
            }
        )";

        THEN("Passes can run multiple times") {
            std::string input = reindent_text(nmodl_text);
            REQUIRE_NOTHROW(run_visitor_passes(input));
        }
    }
}

//=============================================================================
// to_nmodl with excluding set of node types
//=============================================================================

SCENARIO("Sympy specific AST to NMODL conversion") {
    GIVEN("NMODL block with unit usage") {
        static const std::string nmodl = R"(
            BREAKPOINT {
                Pf_NMDA  =  (1/1.38) * 120 (mM) * 0.6
                VDCC = gca_bar_VDCC * 4(um2)*PI*3(1/um3)
                gca_bar_VDCC = 0.0372 (nS/um2)
            }
        )";

        static const std::string expected = R"(
            BREAKPOINT {
                Pf_NMDA = (1/1.38)*120*0.6
                VDCC = gca_bar_VDCC*4*PI*3
                gca_bar_VDCC = 0.0372
            }
        )";

        THEN("to_nmodl can ignore all units") {
            auto input = reindent_text(nmodl);
            NmodlDriver driver;
            const auto& ast = driver.parse_string(input);
            const auto& result = to_nmodl(ast, {AstNodeType::UNIT});
            REQUIRE(result == reindent_text(expected));
        }
    }
}
