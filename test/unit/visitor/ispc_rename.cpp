/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "catch/catch.hpp"

#include "ast/program.hpp"
#include "parser/nmodl_driver.hpp"
#include "test/unit/utils/nmodl_constructs.hpp"
#include "visitors/ispc_rename_visitor.hpp"
#include "visitors/lookup_visitor.hpp"
#include "visitors/symtab_visitor.hpp"
#include "visitors/verbatim_visitor.hpp"

using namespace nmodl;
using namespace visitor;
using namespace test_utils;

using ast::AstNodeType;
using nmodl::parser::NmodlDriver;
using symtab::syminfo::NmodlType;

//=============================================================================
// IspcRename visitor tests
//=============================================================================

std::shared_ptr<ast::Program> run_ispc_rename_visitor(const std::string& text) {
    NmodlDriver driver;
    auto ast = driver.parse_string(text);

    IspcRenameVisitor(ast).visit_program(*ast);
    SymtabVisitor().visit_program(*ast);
    return ast;
}

std::vector<std::string> run_verbatim_visitor(const std::shared_ptr<ast::Program>& ast) {
    NmodlDriver driver;

    VerbatimVisitor v;
    v.visit_program(*ast);

    return v.verbatim_blocks();
}

SCENARIO("Rename variables that ISPC parses as double constants", "[visitor][ispcrename]") {
    GIVEN("mod file with variables with names that resemble for ISPC double constants") {
        std::string input_nmodl = R"(
            NEURON {
                SUFFIX test_ispc_rename
                RANGE d1, d2, var_d3, d4
            }
            ASSIGNED {
                d1
                d2
                var_d3
                d4
            }
            INITIAL {
                d1 = 1
                d2 = 2
                var_d3 = 3
            }
            PROCEDURE func () {
            VERBATIM d4 = 4; ENDVERBATIM
            }
        )";
        auto ast = run_ispc_rename_visitor(input_nmodl);
        auto verbatim_blocks = run_verbatim_visitor(ast);
        auto symtab = ast->get_symbol_table();
        THEN(
            "Variables that match the constant double presentation in ISPC are renamed to "
            "var_<original_name>") {
            /// check if var_d1 and var_d2 exist and replaced d1 and d2
            auto var_d1 = symtab->lookup("var_d1");
            REQUIRE(var_d1 != nullptr);
            auto d1 = symtab->lookup("d1");
            REQUIRE(d1 == nullptr);
            auto var_d2 = symtab->lookup("var_d2");
            REQUIRE(var_d2 != nullptr);
            auto d2 = symtab->lookup("d2");
            REQUIRE(d2 == nullptr);
            /// Check if VERBATIM block variable is renamed
            REQUIRE(verbatim_blocks.size() == 1);
            REQUIRE(verbatim_blocks.front() == " var_d4 = 4; ");
        }
        THEN("Variables that don't match the constant double presentation in ISPC stay the same") {
            /// check if var_d3 exists
            auto var_d3 = symtab->lookup("var_d3");
            REQUIRE(var_d3 != nullptr);
        }
    }
}
