/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "ast/program.hpp"
#include "codegen/codegen_neuron_cpp_visitor.hpp"
#include "parser/nmodl_driver.hpp"
#include "test/unit/utils/test_utils.hpp"
#include "visitors/function_callpath_visitor.hpp"
#include "visitors/inline_visitor.hpp"
#include "visitors/neuron_solve_visitor.hpp"
#include "visitors/solve_block_visitor.hpp"
#include "visitors/symtab_visitor.hpp"

using Catch::Matchers::ContainsSubstring;

using namespace nmodl;
using namespace visitor;
using namespace codegen;

using nmodl::parser::NmodlDriver;
using nmodl::test_utils::reindent_text;
using symtab::syminfo::NmodlType;

/// Helper for creating C codegen visitor
std::shared_ptr<CodegenNeuronCppVisitor> create_neuron_cpp_visitor(
    const std::shared_ptr<ast::Program>& ast,
    const std::string& /* text */,
    std::stringstream& ss) {
    /// construct symbol table
    SymtabVisitor().visit_program(*ast);

    /// run all necessary pass
    InlineVisitor().visit_program(*ast);
    NeuronSolveVisitor().visit_program(*ast);
    SolveBlockVisitor().visit_program(*ast);
    FunctionCallpathVisitor().visit_program(*ast);

    /// create C code generation visitor
    auto cv = std::make_shared<CodegenNeuronCppVisitor>("_test", ss, "double", false);
    cv->setup(*ast);
    return cv;
}

SCENARIO("Check whether PROCEDURE and FUNCTION need setdata call", "[codegen][needsetdata]") {
    GIVEN("mod file with GLOBAL and RANGE variables used in FUNC and PROC") {
        std::string input_nmodl = R"(
            NEURON {
                SUFFIX test
                RANGE x
                GLOBAL s
            }
            PARAMETER {
                s = 2
            }
            ASSIGNED {
                x
            }
            PROCEDURE a() {
                x = get_42()
            }
            FUNCTION b() {
                a()
            }
            FUNCTION get_42() {
                get_42 = 42
            }
        )";
        const auto& ast = NmodlDriver().parse_string(input_nmodl);
        std::stringstream ss;
        auto cvisitor = create_neuron_cpp_visitor(ast, input_nmodl, ss);
        cvisitor->visit_program(*ast);
        const auto symtab = ast->get_symbol_table();
        THEN("use_range_ptr_var property is added to needed FUNC and PROC") {
            auto use_range_ptr_var_funcs = symtab->get_variables_with_properties(
                NmodlType::use_range_ptr_var);
            REQUIRE(use_range_ptr_var_funcs.size() == 2);
            const auto a = symtab->lookup("a");
            REQUIRE(a->has_any_property(NmodlType::use_range_ptr_var));
            const auto b = symtab->lookup("b");
            REQUIRE(b->has_any_property(NmodlType::use_range_ptr_var));
            const auto get_42 = symtab->lookup("get_42");
            REQUIRE(!get_42->has_any_property(NmodlType::use_range_ptr_var));
        }
    }
}
