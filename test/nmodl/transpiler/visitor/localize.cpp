/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "catch/catch.hpp"

#include "parser/nmodl_driver.hpp"
#include "test/utils/test_utils.hpp"
#include "visitors/inline_visitor.hpp"
#include "visitors/localize_visitor.hpp"
#include "visitors/nmodl_visitor.hpp"
#include "visitors/symtab_visitor.hpp"

using namespace nmodl;
using namespace visitor;
using namespace test_utils;

using nmodl::parser::NmodlDriver;

//=============================================================================
// Localizer visitor tests
//=============================================================================

std::string run_localize_visitor(const std::string& text) {
    NmodlDriver driver;
    auto ast = driver.parse_string(text);
    SymtabVisitor().visit_program(ast.get());
    InlineVisitor().visit_program(ast.get());
    LocalizeVisitor().visit_program(ast.get());

    std::stringstream stream;
    NmodlPrintVisitor(stream).visit_program(ast.get());
    return stream.str();
}


SCENARIO("Localizer test with single global block", "[visitor][localizer]") {
    GIVEN("Single derivative block with variable definition") {
        std::string nmodl_text = R"(
            NEURON {
                RANGE tau
            }

            DERIVATIVE states {
                tau = 11.1
                exp(tau)
            }
        )";

        std::string output_nmodl = R"(
            NEURON {
                RANGE tau
            }

            DERIVATIVE states {
                LOCAL tau
                tau = 11.1
                exp(tau)
            }
        )";

        THEN("tau variable gets localized") {
            std::string input = reindent_text(nmodl_text);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_localize_visitor(input);
            REQUIRE(result == expected_result);
        }
    }
}

SCENARIO("Localizer test with use of verbatim block", "[visitor][localizer]") {
    GIVEN("Verbatim block usage in one of the global block") {
        std::string nmodl_text = R"(
            NEURON {
                RANGE tau
            }

            DERIVATIVE states {
                tau = 11.1
                exp(tau)
            }

            BREAKPOINT {
                VERBATIM ENDVERBATIM
            }
        )";

        std::string output_nmodl = R"(
            NEURON {
                RANGE tau
            }

            DERIVATIVE states {
                tau = 11.1
                exp(tau)
            }

            BREAKPOINT {
                VERBATIM ENDVERBATIM
            }
        )";

        THEN("Localization is disabled") {
            std::string input = reindent_text(nmodl_text);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_localize_visitor(input);
            REQUIRE(result == expected_result);
        }
    }
}


SCENARIO("Localizer test with multiple global blocks", "[visitor][localizer]") {
    GIVEN("Multiple global blocks with definition of variable") {
        std::string nmodl_text = R"(
            NEURON {
                RANGE tau, beta
            }

            INITIAL {
                LOCAL tau
                tau = beta
            }

            DERIVATIVE states {
                tau = 11.1
                exp(tau)
            }

            BREAKPOINT {
                IF (1) {
                    tau = beta
                } ELSE {
                    tau = 11
                }

            }
        )";

        std::string output_nmodl = R"(
            NEURON {
                RANGE tau, beta
            }

            INITIAL {
                LOCAL tau
                tau = beta
            }

            DERIVATIVE states {
                LOCAL tau
                tau = 11.1
                exp(tau)
            }

            BREAKPOINT {
                LOCAL tau
                IF (1) {
                    tau = beta
                } ELSE {
                    tau = 11
                }
            }
        )";

        THEN("Localization across multiple blocks is done") {
            std::string input = reindent_text(nmodl_text);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_localize_visitor(input);
            REQUIRE(result == expected_result);
        }
    }


    GIVEN("Two global blocks with definition and use of the variable") {
        std::string nmodl_text = R"(
            NEURON {
                RANGE tau
            }

            DERIVATIVE states {
                tau = 11.1
            }

            BREAKPOINT {
                IF (1) {
                    tau = 22
                } ELSE {
                    tau = exp(tau) + 11
                }

            }
        )";

        std::string output_nmodl = R"(
            NEURON {
                RANGE tau
            }

            DERIVATIVE states {
                tau = 11.1
            }

            BREAKPOINT {
                IF (1) {
                    tau = 22
                } ELSE {
                    tau = exp(tau)+11
                }
            }
        )";

        THEN("Localization is not done due to use of variable") {
            std::string input = reindent_text(nmodl_text);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_localize_visitor(input);
            REQUIRE(result == expected_result);
        }
    }
}
