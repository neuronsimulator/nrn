/*
 * Copyright 2024 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <catch2/catch_test_macros.hpp>

#include "ast/program.hpp"
#include "codegen/codegen_helper_visitor.hpp"
#include "parser/nmodl_driver.hpp"
#include "utils/test_utils.hpp"
#include "visitors/symtab_visitor.hpp"

using namespace nmodl;
using namespace visitor;
using namespace test_utils;

using ast::AstNodeType;
using nmodl::codegen::CodegenHelperVisitor;
using nmodl::codegen::CodegenInfo;
using nmodl::parser::NmodlDriver;

CodegenInfo compute_code_info(const std::string& text) {
    NmodlDriver driver;
    const auto& ast = driver.parse_string(text);

    /// construct symbol table and run codegen helper visitor
    SymtabVisitor().visit_program(*ast);
    CodegenHelperVisitor v;

    /// symbols/variables are collected in info object
    return v.analyze(*ast);
}


TEST_CASE("EXTERNAL variables") {
    std::string mod = R"(
NEURON {
  EXTERNAL gbl_foo, param_bar
}

ASSIGNED {
  foo
  gbl_foo
  param_bar
}
)";

    auto info = compute_code_info(mod);

    std::vector<std::string> actual;
    for (const auto& var: info.external_variables) {
        actual.push_back(var->get_name());
    }
    std::sort(actual.begin(), actual.end());

    std::vector<std::string> expected{"gbl_foo", "param_bar"};

    REQUIRE(actual == expected);
}
