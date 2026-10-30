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
#include "utils/test_utils.hpp"
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

/// Text of one generated function, from its signature up to the next
/// `static void` definition. Namespace indentation means that marker is not
/// at column 0.
std::string generated_function(const std::string& cpp, const std::string& name) {
    const auto signature = "static void " + name + "(";
    const auto start = cpp.find(signature);
    REQUIRE(start != std::string::npos);
    const auto next_fn = cpp.find("static void ", start + signature.size());
    if (next_fn == std::string::npos) {
        return cpp.substr(start);
    }
    const auto line = cpp.rfind('\n', next_fn);
    if (line == std::string::npos || line <= start) {
        return cpp.substr(start);
    }
    return cpp.substr(start, line - start);
}

SCENARIO("NEURON backend emits BEFORE/AFTER as per-instance callbacks", "[codegen][ba]") {
    GIVEN("one block of each accepted kind, in source order") {
        std::string nmodl = R"(
            NEURON {
                SUFFIX ba1
                RANGE x
            }
            ASSIGNED {
                x
            }
            PROCEDURE take_v() {
                x = v
                VERBATIM
                return ret_take_v;
                ENDVERBATIM
            }
            BEFORE BREAKPOINT { x = v }
            AFTER SOLVE { x = v }
            BEFORE INITIAL { x = v }
            AFTER INITIAL { x = v }
            BEFORE STEP { take_v() }
        )";
        std::string cpp = transpile(nmodl);
        const auto body = generated_function(cpp, "nrn_before_after_0_ba1");
        const auto step = generated_function(cpp, "nrn_before_after_4_ba1");

        THEN("hoc_reg_ba uses the nocmodl type integers") {
            REQUIRE_THAT(cpp,
                         ContainsSubstring("hoc_reg_ba(mech_type, nrn_before_after_0_ba1, 11);"));
            REQUIRE_THAT(cpp,
                         ContainsSubstring("hoc_reg_ba(mech_type, nrn_before_after_1_ba1, 22);"));
            REQUIRE_THAT(cpp,
                         ContainsSubstring("hoc_reg_ba(mech_type, nrn_before_after_2_ba1, 13);"));
            REQUIRE_THAT(cpp,
                         ContainsSubstring("hoc_reg_ba(mech_type, nrn_before_after_3_ba1, 23);"));
            REQUIRE_THAT(cpp,
                         ContainsSubstring("hoc_reg_ba(mech_type, nrn_before_after_4_ba1, 14);"));
        }
        THEN("the callback is nrn_bamech_t and assigns v_unused from the node") {
            REQUIRE_THAT(cpp, ContainsSubstring("for (int id = 0; id < nodecount"));
            REQUIRE_THAT(body,
                         ContainsSubstring(
                             "static void nrn_before_after_0_ba1(Node* _nd, Datum* _ppvar, Datum* "
                             "_thread, NrnThread* nt, Memb_list* _ml_arg, size_t id, const "
                             "_nrn_model_sorted_token& _sorted_token)"));
            REQUIRE_THAT(body, !ContainsSubstring("nrn_before_after_0_ba1(const"));
            REQUIRE_THAT(body, !ContainsSubstring("for (int id = 0; id < nodecount"));
            REQUIRE_THAT(body, !ContainsSubstring("double v"));
            REQUIRE_THAT(body, !ContainsSubstring("auto* _thread"));
            REQUIRE_THAT(body,
                         ContainsSubstring(
                             "inst.v_unused[id] = node_data.node_voltages[node_id];"));
            REQUIRE_THAT(step,
                         ContainsSubstring(
                             "take_v_ba1(_lmc, inst, node_data, id, _ppvar, _thread, nt);"));
        }
    }
}

SCENARIO("Two BEFORE STEP blocks are both registered", "[codegen][ba]") {
    GIVEN("a mod file with two BEFORE STEP blocks") {
        std::string nmodl = R"(
            NEURON {
                SUFFIX ba2
                RANGE x
            }
            ASSIGNED { x }
            BEFORE STEP { x = 1 }
            BEFORE STEP { x = 2 }
        )";
        std::string cpp = transpile(nmodl);

        THEN("each block gets its own callback and the same type code") {
            REQUIRE_THAT(cpp,
                         ContainsSubstring("hoc_reg_ba(mech_type, nrn_before_after_0_ba2, 14);"));
            REQUIRE_THAT(cpp,
                         ContainsSubstring("hoc_reg_ba(mech_type, nrn_before_after_1_ba2, 14);"));
            REQUIRE(cpp.find("static void nrn_before_after_0_ba2(Node*") != std::string::npos);
            REQUIRE(cpp.find("static void nrn_before_after_1_ba2(Node*") != std::string::npos);
        }
    }
}

SCENARIO("POINT_PROCESS BEFORE STEP keeps the POINTER verbatim macro", "[codegen][ba]") {
    GIVEN("a point process whose BEFORE STEP uses _p_p") {
        std::string nmodl = R"(
            NEURON {
                POINT_PROCESS bapp
                POINTER p
                RANGE n
            }
            ASSIGNED { p n }
            BEFORE STEP {
                VERBATIM
                if (_p_p) {
                    *(double*)_p_p += 1.0;
                }
                ENDVERBATIM
            }
        )";
        std::string cpp = transpile(nmodl);
        const auto body = generated_function(cpp, "nrn_before_after_0_bapp");

        THEN("the callback receives instance pdata and exposes _p_p") {
            REQUIRE_THAT(cpp,
                         ContainsSubstring("hoc_reg_ba(mech_type, nrn_before_after_0_bapp, 14);"));
            REQUIRE_THAT(body, ContainsSubstring("Datum* _ppvar"));
            REQUIRE_THAT(body, ContainsSubstring("literal_value<void*>()"));
            REQUIRE_THAT(body,
                         ContainsSubstring(
                             "inst.v_unused[id] = node_data.node_voltages[node_id];"));
        }
    }
}

SCENARIO("ARTIFICIAL_CELL BEFORE STEP has no node voltage", "[codegen][ba]") {
    GIVEN("an artificial cell") {
        std::string nmodl = R"(
            NEURON {
                ARTIFICIAL_CELL baart
            }
            BEFORE STEP {
                LOCAL x
                x = 1
            }
        )";
        std::string cpp = transpile(nmodl);
        const auto body = generated_function(cpp, "nrn_before_after_0_baart");

        THEN("the callback is registered and does not read node_data") {
            REQUIRE_THAT(cpp,
                         ContainsSubstring("hoc_reg_ba(mech_type, nrn_before_after_0_baart, 14);"));
            REQUIRE_THAT(body, !ContainsSubstring("node_data"));
            REQUIRE_THAT(body, !ContainsSubstring("v_unused"));
            REQUIRE_THAT(body, !ContainsSubstring("double v"));
        }
    }
}

SCENARIO("LAG inside BEFORE STEP calls scopmath lag", "[codegen][ba]") {
    GIVEN("LAG v BY dur") {
        std::string nmodl = R"(
            NEURON {
                SUFFIX lagba
                RANGE x, dur
            }
            PARAMETER { dur = 1 }
            ASSIGNED { x }
            BEFORE STEP {
                LAG v BY dur
                x = lag_v_dur
            }
        )";
        std::string cpp = transpile(nmodl);
        const auto body = generated_function(cpp, "nrn_before_after_0_lagba");

        THEN("the lagged value is stored in lag_v_dur") {
            REQUIRE_THAT(body,
                         ContainsSubstring("static double lag_v_dur; lag_v_dur = "
                                           "*lag(&(inst.v_unused[id]), nt->_t, inst.dur[id], 0);"));
            REQUIRE_THAT(body, ContainsSubstring("inst.x[id] = lag_v_dur"));
        }
    }
}

SCENARIO("A mechanism without BEFORE/AFTER does not call hoc_reg_ba", "[codegen][ba]") {
    GIVEN("a mod file with only an INITIAL block") {
        std::string nmodl = R"(
            NEURON {
                SUFFIX noba
                RANGE x
            }
            ASSIGNED { x }
            INITIAL { x = 1 }
        )";
        std::string cpp = transpile(nmodl);

        THEN("registration is unchanged") {
            REQUIRE_THAT(cpp, !ContainsSubstring("hoc_reg_ba"));
            REQUIRE_THAT(cpp, !ContainsSubstring("nrn_before_after_"));
        }
    }
}
