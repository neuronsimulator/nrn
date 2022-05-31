/*************************************************************************
 * Copyright (C) 2019-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <catch2/catch.hpp>

#include "ast/program.hpp"
#include "codegen/codegen_ispc_visitor.hpp"
#include "config/config.h"
#include "parser/nmodl_driver.hpp"
#include "test/unit/utils/test_utils.hpp"
#include "visitors/symtab_visitor.hpp"
#include "visitors/units_visitor.hpp"

using namespace nmodl;
using namespace visitor;
using namespace codegen;
using namespace test_utils;

using nmodl::NrnUnitsLib;
using nmodl::parser::NmodlDriver;

//=============================================================================
// Helper for codegen related visitor
//=============================================================================
class IspcCodegenTestHelper {
    /**
     * Shared pointer to the CodegenIspcVisitor that is used throughout the test
     */
    std::shared_ptr<CodegenIspcVisitor> codegen_ispc_visitor;

    /**
     * Driver to parse the nmodl text
     */
    NmodlDriver driver;

    /**
     * Shared pointer of AST generated by the parsed NMODL text
     */
    std::shared_ptr<ast::Program> ast;

    /**
     * String buffer which saves the output after every call to CodegenIspcVisitor
     */
    std::stringbuf strbuf;

  public:
    /**
     * \brief Constructs the helper class for the unit tests of CodegenIspcVisitor
     *
     * Takes as argument the nmodl text and sets up the CodegenIspcVisitor.
     *
     * \param nmodl_text NMODL text to be parsed and printed
     *
     */
    IspcCodegenTestHelper(const std::string& nmodl_text) {
        ast = driver.parse_string(nmodl_text);

        /// directory where units lib file is located
        std::string units_dir(NrnUnitsLib::get_path());
        /// parse units of text
        UnitsVisitor(units_dir).visit_program(*ast);

        /// construct symbol table
        SymtabVisitor().visit_program(*ast);

        /// initialize CodegenIspcVisitor
        std::ostream oss(&strbuf);
        codegen_ispc_visitor =
            std::make_shared<CodegenIspcVisitor>("unit_test", oss, "double", false);
        codegen_ispc_visitor->setup(*ast);
    }

    /**
     * Get CodegenIspcVisitor to call its functions
     */
    std::shared_ptr<CodegenIspcVisitor> get_visitor() {
        return codegen_ispc_visitor;
    }

    /**
     * Returns the existing string and resets the buffer for next call
     */
    std::string get_string() {
        std::string return_string = strbuf.str();
        strbuf.str("");
        return return_string;
    }
};

std::string print_ispc_nmodl_constants(IspcCodegenTestHelper& ispc_codegen_test_helper) {
    /// print nmodl constants
    ispc_codegen_test_helper.get_visitor()->print_nmodl_constants();

    return ispc_codegen_test_helper.get_string();
}

std::string print_ispc_compute_functions(IspcCodegenTestHelper& ispc_codegen_test_helper) {
    /// print compute functions
    ispc_codegen_test_helper.get_visitor()->print_compute_functions();

    return ispc_codegen_test_helper.get_string();
}


SCENARIO("ISPC codegen", "[codegen][ispc]") {
    GIVEN("Simple mod file") {
        std::string nmodl_text = R"(
            TITLE UnitTest
            NEURON {
                SUFFIX unit_test
                RANGE a, b
            }
            ASSIGNED {
                a
                b
            }
            UNITS {
                FARADAY = (faraday) (coulomb)
            }
            INITIAL {
                a = 0.0
                b = 0.0
            }
            BREAKPOINT {
                a = b + FARADAY
            }
        )";

        std::string nmodl_constants_declaration = R"(
            /** constants used in nmodl */
            static const uniform double FARADAY = 96485.3321233100141d;
        )";

        std::string nrn_init_state_block = R"(
            /** initialize channel */
            export void nrn_init_unit_test(uniform unit_test_Instance* uniform inst, uniform NrnThread* uniform nt, uniform Memb_list* uniform ml, uniform int type) {
                uniform int nodecount = ml->nodecount;
                uniform int pnodecount = ml->_nodecount_padded;
                const int* uniform node_index = ml->nodeindices;
                double* uniform data = ml->data;
                const double* uniform voltage = nt->_actual_v;
                Datum* uniform indexes = ml->pdata;
                ThreadDatum* uniform thread = ml->_thread;

                int uniform start = 0;
                int uniform end = nodecount;
                foreach (id = start ... end) {
                    int node_id = node_index[id];
                    double v = voltage[node_id];
                    #if NRN_PRCELLSTATE
                    inst->v_unused[id] = v;
                    #endif
                    inst->a[id] = 0.0d;
                    inst->b[id] = 0.0d;
                }
            }


            /** update state */
            export void nrn_state_unit_test(uniform unit_test_Instance* uniform inst, uniform NrnThread* uniform nt, uniform Memb_list* uniform ml, uniform int type) {
                uniform int nodecount = ml->nodecount;
                uniform int pnodecount = ml->_nodecount_padded;
                const int* uniform node_index = ml->nodeindices;
                double* uniform data = ml->data;
                const double* uniform voltage = nt->_actual_v;
                Datum* uniform indexes = ml->pdata;
                ThreadDatum* uniform thread = ml->_thread;

                int uniform start = 0;
                int uniform end = nodecount;
                foreach (id = start ... end) {
                    int node_id = node_index[id];
                    double v = voltage[node_id];
                    #if NRN_PRCELLSTATE
                    inst->v_unused[id] = v;
                    #endif
                    inst->a[id] = inst->b[id] + FARADAY;
                }
            }
        )";

        IspcCodegenTestHelper ispc_codegen_test_helper(nmodl_text);
        THEN("Check that the nmodl constants and computer functions are printed correctly") {
            auto nmodl_constants_result = reindent_text(
                print_ispc_nmodl_constants(ispc_codegen_test_helper));
            REQUIRE(nmodl_constants_result == reindent_text(nmodl_constants_declaration));
            auto nmodl_init_cur_state = reindent_text(
                print_ispc_compute_functions(ispc_codegen_test_helper));
            REQUIRE(nmodl_init_cur_state == reindent_text(nrn_init_state_block));
        }
    }
}
