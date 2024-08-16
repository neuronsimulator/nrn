/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <catch2/catch_test_macros.hpp>

#include "ast/program.hpp"
#include "codegen/codegen_helper_visitor.hpp"
#include "codegen/codegen_info.hpp"
#include "parser/nmodl_driver.hpp"
#include "visitors/kinetic_block_visitor.hpp"
#include "visitors/neuron_solve_visitor.hpp"
#include "visitors/solve_block_visitor.hpp"
#include "visitors/steadystate_visitor.hpp"
#include "visitors/symtab_visitor.hpp"

using namespace nmodl;
using namespace visitor;
using namespace codegen;

using nmodl::parser::NmodlDriver;

//=============================================================================
// Helper for codegen related visitor
//=============================================================================
std::string run_codegen_helper_visitor(const std::string& text) {
    NmodlDriver driver;
    const auto& ast = driver.parse_string(text);

    /// construct symbol table and run codegen helper visitor
    SymtabVisitor().visit_program(*ast);
    CodegenHelperVisitor v;

    /// symbols/variables are collected in info object
    const auto& info = v.analyze(*ast);

    /// semicolon separated list of variables
    std::string variables;

    /// range variables in order of code generation
    for (const auto& var: info.range_parameter_vars) {
        variables += var->get_name() + ";";
    }
    for (const auto& var: info.range_assigned_vars) {
        variables += var->get_name() + ";";
    }
    for (const auto& var: info.range_state_vars) {
        variables += var->get_name() + ";";
    }
    for (const auto& var: info.assigned_vars) {
        variables += var->get_name() + ";";
    }

    return variables;
}

CodegenInfo run_codegen_helper_get_info(const std::string& text) {
    const auto& ast = NmodlDriver().parse_string(text);
    /// construct symbol table and run codegen helper visitor
    SymtabVisitor{}.visit_program(*ast);
    KineticBlockVisitor{}.visit_program(*ast);
    SymtabVisitor{}.visit_program(*ast);
    SteadystateVisitor{}.visit_program(*ast);
    SymtabVisitor{}.visit_program(*ast);
    NeuronSolveVisitor{}.visit_program(*ast);
    SolveBlockVisitor{}.visit_program(*ast);
    SymtabVisitor{true}.visit_program(*ast);

    CodegenHelperVisitor v;
    const auto info = v.analyze(*ast);

    return info;
}

SCENARIO("unusual / failing mod files", "[codegen][var_order]") {
    GIVEN("cal_mig.mod : USEION variables declared as RANGE") {
        std::string nmodl_text = R"(
            PARAMETER {
              gcalbar=.003 (mho/cm2)
              ki=.001 (mM)
              cai = 50.e-6 (mM)
              cao = 2 (mM)
              q10 = 5
              USEGHK=1
            }
            NEURON {
              SUFFIX cal
              USEION ca READ cai,cao WRITE ica
              RANGE gcalbar, cai, ica, gcal, ggk
              RANGE minf, tau
              GLOBAL USEGHK
            }
            STATE {
              m
            }
            ASSIGNED {
              ica (mA/cm2)
              gcal (mho/cm2)
              minf
              tau   (ms)
              ggk
            }
            DERIVATIVE state {
              rate(v)
              m' = (minf - m)/tau
            }
        )";

        THEN("ionic current variable declared as RANGE appears first") {
            std::string expected = "gcalbar;ica;gcal;minf;tau;ggk;m;cai;cao;";
            auto result = run_codegen_helper_visitor(nmodl_text);
            REQUIRE(result == expected);
        }
    }

    GIVEN("CaDynamics_E2.mod : USEION variables declared as STATE variable") {
        std::string nmodl_text = R"(
            NEURON  {
              SUFFIX CaDynamics_E2
              USEION ca READ ica WRITE cai
              RANGE decay, gamma, minCai, depth
            }

            PARAMETER   {
              gamma = 0.05 : percent of free calcium (not buffered)
              decay = 80 (ms) : rate of removal of calcium
              depth = 0.1 (um) : depth of shell
              minCai = 1e-4 (mM)
            }

            ASSIGNED {ica (mA/cm2)}

            STATE {
              cai (mM)
            }

            DERIVATIVE states   {
              cai' = -(10000)*(ica*gamma/(2*FARADAY*depth)) - (cai - minCai)/decay
            }
        )";

        THEN("ion state variable is ordered after parameter and assigned ionic current") {
            std::string expected = "gamma;decay;depth;minCai;ica;cai;";
            auto result = run_codegen_helper_visitor(nmodl_text);
            REQUIRE(result == expected);
        }
    }

    GIVEN("cadyn.mod : same USEION variables used for read as well as write") {
        std::string nmodl_text = R"(
            NEURON {
              SUFFIX cadyn
              USEION ca READ cai,ica WRITE cai
              RANGE ca
              GLOBAL depth,cainf,taur
            }

            PARAMETER {
              depth    = .1    (um)
              taur =  200 (ms)    : rate of calcium removal
              cainf   = 50e-6(mM) :changed oct2
              cai     (mM)
            }

            ASSIGNED {
              ica     (mA/cm2)
              drive_channel   (mM/ms)
            }

            STATE {
              ca      (mM)
            }

            BREAKPOINT {
              SOLVE state METHOD euler
            }

            DERIVATIVE state {
              ca' = drive_channel/18 + (cainf -ca)/taur*11
              cai = ca
            }
        )";

        THEN("ion variables are ordered correctly") {
            std::string expected = "ca;cai;ica;drive_channel;";
            auto result = run_codegen_helper_visitor(nmodl_text);
            REQUIRE(result == expected);
        }
    }
}

SCENARIO("Check global variable setup", "[codegen][global_variables]") {
    GIVEN("SH_na8st.mod: modfile from reduced_dentate model") {
        std::string const nmodl_text{R"(
            NEURON {
                SUFFIX na8st
            }
            STATE { c1 c2 }
            BREAKPOINT {
                SOLVE kin METHOD derivimplicit
            }
            INITIAL {
                SOLVE kin STEADYSTATE derivimplicit
            }
            KINETIC kin {
                ~ c1 <-> c2 (a1, b1)
            }
        )"};
        NmodlDriver driver;
        const auto ast = driver.parse_string(nmodl_text);

        /// construct symbol table and run codegen helper visitor
        SymtabVisitor{}.visit_program(*ast);
        KineticBlockVisitor{}.visit_program(*ast);
        SymtabVisitor{}.visit_program(*ast);
        SteadystateVisitor{}.visit_program(*ast);
        SymtabVisitor{}.visit_program(*ast);
        NeuronSolveVisitor{}.visit_program(*ast);
        SolveBlockVisitor{}.visit_program(*ast);
        SymtabVisitor{true}.visit_program(*ast);

        CodegenHelperVisitor v;
        const auto info = v.analyze(*ast);
        // See https://github.com/BlueBrain/nmodl/issues/736
        THEN("Checking that primes_size and prime_variables_by_order have the expected size") {
            REQUIRE(info.primes_size == 2);
            REQUIRE(info.prime_variables_by_order.size() == 2);
        }
    }
}

CodegenInfo make_codegen_info(const std::string& text) {
    NmodlDriver driver;
    const auto& ast = driver.parse_string(text);

    SymtabVisitor().visit_program(*ast);
    CodegenHelperVisitor v;

    return v.analyze(*ast);
}

TEST_CASE("Check ion write/read checks") {
    std::string input_nmodl = R"(
        NEURON {
            SUFFIX test
            USEION ca READ cai WRITE cai, eca
            USEION na WRITE nao, ena
            USEION K READ Ki, eK
            RANGE x
        }
        ASSIGNED {
          x
          cai
          eca
          nai
          nao
          ena
          Ki
        }
        INITIAL {
            x = cai
            cai = 42.0
            x = nao
            Ki = 42.0
        }
        BREAKPOINT {
            eca = 42.0
            x = ena
            eK = 42.0
        }
    )";

    auto info = make_codegen_info(input_nmodl);

    for (const auto& ion: info.ions) {
        if (ion.name == "ca") {
            REQUIRE(ion.is_conc_read());
            REQUIRE(ion.is_interior_conc_read());
            REQUIRE(!ion.is_exterior_conc_read());
            REQUIRE(!ion.is_rev_read());

            REQUIRE(ion.is_conc_written());
            REQUIRE(ion.is_interior_conc_written());
            REQUIRE(!ion.is_exterior_conc_written());
            REQUIRE(ion.is_rev_written());
        }
        if (ion.name == "na") {
            REQUIRE(!ion.is_conc_read());
            REQUIRE(!ion.is_interior_conc_read());
            REQUIRE(!ion.is_exterior_conc_read());
            REQUIRE(!ion.is_rev_read());

            REQUIRE(ion.is_conc_written());
            REQUIRE(!ion.is_interior_conc_written());
            REQUIRE(ion.is_exterior_conc_written());
            REQUIRE(ion.is_rev_written());
        }
        if (ion.name == "K") {
            REQUIRE(ion.is_conc_read());
            REQUIRE(ion.is_interior_conc_read());
            REQUIRE(!ion.is_exterior_conc_read());
            REQUIRE(ion.is_rev_read());

            REQUIRE(!ion.is_conc_written());
            REQUIRE(!ion.is_interior_conc_written());
            REQUIRE(!ion.is_exterior_conc_written());
            REQUIRE(!ion.is_rev_written());
        }
    }
}

SCENARIO("CVODE codegen") {
    GIVEN("a mod file with a single KINETIC block") {
        std::string input_nmodl = R"(
            STATE {
                x
            }
            KINETIC states {
                ~ x << (a*c/3.2)
            }
            BREAKPOINT {
                SOLVE states METHOD cnexp
            })";

        const auto& info = run_codegen_helper_get_info(input_nmodl);
        THEN("Emit CVODE") {
            REQUIRE(info.emit_cvode);
        }
    }
    GIVEN("a mod file with a single DERIVATIVE block") {
        std::string input_nmodl = R"(
            STATE {
                m
            }
            BREAKPOINT {
                SOLVE state METHOD derivimplicit
            }
            DERIVATIVE state {
               m' = 2 * m
            }
        )";
        const auto& info = run_codegen_helper_get_info(input_nmodl);

        THEN("Emit CVODE") {
            REQUIRE(info.emit_cvode);
        }
    }
    GIVEN("a mod file with a single PROCEDURE block solved with method `after_cvode`") {
        std::string input_nmodl = R"(
        BREAKPOINT {
            SOLVE state METHOD after_cvode
        }
        PROCEDURE state() {}
        )";

        const auto& info = run_codegen_helper_get_info(input_nmodl);

        THEN("Emit CVODE") {
            REQUIRE(info.emit_cvode);
        }
    }
    GIVEN("a mod file with a single PROCEDURE block NOT solved with method `after_cvode`") {
        std::string input_nmodl = R"(
        BREAKPOINT {
            SOLVE state METHOD cnexp
        }
        PROCEDURE state() {}
        )";

        const auto& info = run_codegen_helper_get_info(input_nmodl);

        THEN("Do not emit CVODE") {
            REQUIRE(!info.emit_cvode);
        }
    }
    GIVEN("a mod file with a DERIVATIVE and a KINETIC block") {
        std::string input_nmodl = R"(
            STATE {
                m
                x
            }
            BREAKPOINT {
                SOLVE der METHOD derivimplicit
                SOLVE kin METHOD cnexp
            }
            DERIVATIVE der {
               m' = 2 * m
            }
            KINETIC kin {
                ~ x << (a*c/3.2)
            }
        )";

        const auto& info = run_codegen_helper_get_info(input_nmodl);

        THEN("Do not emit CVODE") {
            REQUIRE(!info.emit_cvode);
        }
    }
    GIVEN("a mod file with a PROCEDURE and a DERIVATIVE block") {
        std::string input_nmodl = R"(
            STATE {
                m
            }
            BREAKPOINT {
                SOLVE der METHOD derivimplicit
                SOLVE func METHOD cnexp
            }
            DERIVATIVE der {
               m' = 2 * m
            }
            PROCEDURE func() {
            }
        )";

        const auto& info = run_codegen_helper_get_info(input_nmodl);

        THEN("Do not emit CVODE") {
            REQUIRE(!info.emit_cvode);
        }
    }
}
