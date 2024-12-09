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

    bool optimize_ion_vars = false;
    bool enable_cvode = true;

    /// create C code generation visitor
    auto cv = std::make_shared<CodegenNeuronCppVisitor>(
        "_test", ss, "double", optimize_ion_vars, enable_cvode);
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

std::string create_mod_file_write(const std::string& var) {
    std::string pattern = R"(
        NEURON {{
            SUFFIX test
            USEION ca WRITE {0}
        }}
        ASSIGNED {{
          {0}
        }}
        INITIAL {{
            {0} = 124.5
        }}
    )";

    return fmt::format(pattern, var);
}

std::string create_mod_file_read(const std::string& var) {
    std::string pattern = R"(
        NEURON {{
            SUFFIX test
            USEION ca READ {0}
            RANGE x
        }}
        ASSIGNED {{
          x
          {0}
        }}
        INITIAL {{
            x = {0}
        }}
    )";

    return fmt::format(pattern, var);
}

std::string transpile(const std::string& nmodl) {
    const auto& ast = NmodlDriver().parse_string(nmodl);
    std::stringstream ss;
    auto cvisitor = create_neuron_cpp_visitor(ast, nmodl, ss);
    cvisitor->visit_program(*ast);

    return ss.str();
}

SCENARIO("Write `cao`.", "[codegen]") {
    GIVEN("mod file that writes to `cao`") {
        std::string cpp = transpile(create_mod_file_write("cao"));

        THEN("it contains") {
            REQUIRE_THAT(cpp, ContainsSubstring("nrn_check_conc_write(_prop, ca_prop, 0);"));
            REQUIRE_THAT(cpp, !ContainsSubstring("nrn_check_conc_write(_prop, ca_prop, 1);"));
            REQUIRE_THAT(cpp, ContainsSubstring("nrn_promote(ca_prop, 3, 0);"));
            REQUIRE_THAT(cpp, ContainsSubstring("nrn_wrote_conc("));
        }
    }
}

SCENARIO("Write `cai`.", "[codegen]") {
    GIVEN("mod file that writes to `cai`") {
        std::string cpp = transpile(create_mod_file_write("cai"));

        THEN("it contains") {
            REQUIRE_THAT(cpp, !ContainsSubstring("nrn_check_conc_write(_prop, ca_prop, 0);"));
            REQUIRE_THAT(cpp, ContainsSubstring("nrn_check_conc_write(_prop, ca_prop, 1);"));
            REQUIRE_THAT(cpp, ContainsSubstring("nrn_promote(ca_prop, 3, 0);"));
            REQUIRE_THAT(cpp, ContainsSubstring("nrn_wrote_conc("));
        }
    }
}

SCENARIO("Write `eca`.", "[codegen]") {
    GIVEN("mod file that writes to `eca`") {
        std::string cpp = transpile(create_mod_file_write("eca"));

        THEN("it contains") {
            REQUIRE_THAT(cpp, !ContainsSubstring("nrn_check_conc_write(_prop,"));
            REQUIRE_THAT(cpp, ContainsSubstring("nrn_promote(ca_prop, 0, 3);"));
            REQUIRE_THAT(cpp, !ContainsSubstring("nrn_wrote_conc("));
        }
    }
}

SCENARIO("Read `cao`.", "[codegen]") {
    GIVEN("mod file that reads to `cao`") {
        std::string cpp = transpile(create_mod_file_read("cao"));

        THEN("it contains") {
            REQUIRE_THAT(cpp, !ContainsSubstring("nrn_check_conc_write(_prop,"));
            REQUIRE_THAT(cpp, ContainsSubstring("nrn_promote(ca_prop, 1, 0);"));
            REQUIRE_THAT(cpp, !ContainsSubstring("nrn_wrote_conc("));
        }
    }
}

SCENARIO("Read `cai`.", "[codegen]") {
    GIVEN("mod file that reads to `cai`") {
        std::string cpp = transpile(create_mod_file_read("cai"));

        THEN("it contains") {
            REQUIRE_THAT(cpp, !ContainsSubstring("nrn_check_conc_write(_prop,"));
            REQUIRE_THAT(cpp, ContainsSubstring("nrn_promote(ca_prop, 1, 0);"));
            REQUIRE_THAT(cpp, !ContainsSubstring("nrn_wrote_conc("));
        }
    }
}

SCENARIO("Read `eca`.", "[codegen]") {
    GIVEN("mod file that reads to `eca`") {
        std::string cpp = transpile(create_mod_file_read("eca"));

        THEN("it contains") {
            REQUIRE_THAT(cpp, !ContainsSubstring("nrn_check_conc_write(_prop,"));
            REQUIRE_THAT(cpp, ContainsSubstring("nrn_promote(ca_prop, 0, 1);"));
            REQUIRE_THAT(cpp, !ContainsSubstring("nrn_wrote_conc("));
        }
    }
}

SCENARIO("ARTIFICIAL_CELL with `net_send`") {
    GIVEN("a mod file") {
        std::string nmodl = R"(
            NEURON {
                ARTIFICIAL_CELL test
            }
            NET_RECEIVE(w) {
                net_send(t+1, 1)
            }
        )";
        std::string cpp = transpile(nmodl);

        THEN("it contains") {
            REQUIRE_THAT(cpp, ContainsSubstring("artcell_net_send"));
        }
    }
}

SCENARIO("ARTIFICIAL_CELL with `net_move`") {
    GIVEN("a mod file") {
        std::string nmodl = R"(
            NEURON {
                ARTIFICIAL_CELL test
            }
            NET_RECEIVE(w) {
                net_move(t+1)
            }
        )";
        std::string cpp = transpile(nmodl);

        THEN("it contains") {
            REQUIRE_THAT(cpp, ContainsSubstring("artcell_net_move"));
        }
    }
}
