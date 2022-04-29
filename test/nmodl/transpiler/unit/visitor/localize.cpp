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
#include "visitors/nmodl_visitor.hpp"
#include "visitors/symtab_visitor.hpp"

using namespace nmodl;
using namespace visitor;
using namespace test;
using namespace test_utils;

using nmodl::parser::NmodlDriver;

//=============================================================================
// Localizer visitor tests
//=============================================================================

std::string run_localize_visitor(const std::string& text) {
    NmodlDriver driver;
    const auto& ast = driver.parse_string(text);
    SymtabVisitor().visit_program(*ast);
    InlineVisitor().visit_program(*ast);
    LocalizeVisitor().visit_program(*ast);

    std::stringstream stream;
    NmodlPrintVisitor(stream).visit_program(*ast);

    // check that, after visitor rearrangement, parents are still up-to-date
    CheckParentVisitor().check_ast(*ast);

    return stream.str();
}


SCENARIO("Localizer test with single global block", "[visitor][localizer]") {
    GIVEN("Single derivative block with variable definition") {
        static const std::string nmodl_text = R"(
            NEURON {
                RANGE tau
            }

            DERIVATIVE states {
                tau = 11.1
                exp(tau)
            }
        )";

        static const std::string output_nmodl = R"(
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
            const std::string input = reindent_text(nmodl_text);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_localize_visitor(input);
            REQUIRE(result == expected_result);
        }
    }
}

SCENARIO("Localizer test with use of verbatim block", "[visitor][localizer]") {
    GIVEN("Verbatim block usage in one of the global block") {
        static const std::string nmodl_text = R"(
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

        static const std::string output_nmodl = R"(
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
            static const std::string input = reindent_text(nmodl_text);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_localize_visitor(input);
            REQUIRE(result == expected_result);
        }
    }
}


SCENARIO("Localizer test with multiple global blocks", "[visitor][localizer]") {
    GIVEN("Multiple global blocks with definition of variable") {
        static const std::string nmodl_text = R"(
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

        static const std::string output_nmodl = R"(
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
            const std::string input = reindent_text(nmodl_text);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_localize_visitor(input);
            REQUIRE(result == expected_result);
        }
    }


    GIVEN("Two global blocks with definition and use of the variable") {
        static const std::string nmodl_text = R"(
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

        static const std::string output_nmodl = R"(
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
            const std::string input = reindent_text(nmodl_text);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_localize_visitor(input);
            REQUIRE(result == expected_result);
        }
    }
}
