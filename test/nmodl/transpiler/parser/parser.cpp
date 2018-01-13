#define CATCH_CONFIG_MAIN

#include <string>

#include "catch/catch.hpp"
#include "parser/nmodl_driver.hpp"
#include "input/nmodl_constructs.h"

//=============================================================================
// Parser tests
//=============================================================================

bool is_valid_construct(const std::string& construct) {
    nmodl::Driver driver;
    return driver.parse_string(construct);
}

SCENARIO("NMODL can define macros using DEFINE keyword") {
    GIVEN("A valid macro definition") {
        WHEN("DEFINE NSTEP 6") {
            THEN("parser accepts without an error") {
                REQUIRE(is_valid_construct("DEFINE NSTEP 6"));
            }
        }
        WHEN("DEFINE   NSTEP   6") {
            THEN("parser accepts without an error") {
                REQUIRE(is_valid_construct("DEFINE   NSTEP   6"));
            }
        }
    }

    GIVEN("A macro with nested definition is not supported") {
        WHEN("DEFINE SIX 6 DEFINE NSTEP SIX") {
            THEN("parser throws an error") {
                REQUIRE_THROWS_WITH(is_valid_construct("DEFINE SIX 6 DEFINE NSTEP SIX"),
                                    Catch::Contains("unexpected INVALID_TOKEN"));
            }
        }
    }

    GIVEN("A invalid macro definition with float value") {
        WHEN("DEFINE NSTEP 6.0") {
            THEN("parser throws an exception") {
                REQUIRE_THROWS_WITH(is_valid_construct("DEFINE NSTEP 6.0"),
                                    Catch::Contains("unexpected REAL"));
            }
        }
    }

    GIVEN("A invalid macro definition with name and without value") {
        WHEN("DEFINE NSTEP") {
            THEN("parser throws an exception") {
                REQUIRE_THROWS_WITH(is_valid_construct("DEFINE NSTEP"),
                                    Catch::Contains("expecting INTEGER"));
            }
        }
    }

    GIVEN("A invalid macro definition with name and value as a name") {
        WHEN("DEFINE NSTEP SIX") {
            THEN("parser throws an exception") {
                REQUIRE_THROWS_WITH(is_valid_construct("DEFINE NSTEP SIX"),
                                    Catch::Contains("expecting INTEGER"));
            }
        }
    }

    GIVEN("A invalid macro definition without name but with value") {
        WHEN("DEFINE 6") {
            THEN("parser throws an exception") {
                REQUIRE_THROWS_WITH(is_valid_construct("DEFINE 6"),
                                    Catch::Contains("expecting NAME"));
            }
        }
    }
}

SCENARIO("Macros can be used anywhere in NMODL program") {
    std::string nmodl_text = R"(
            DEFINE NSTEP 6
            PARAMETER {
                amp[NSTEP] (mV)
            }
        )";
    WHEN("macro is used in parameter definition") {
        THEN("parser accepts without an error") {
            REQUIRE(is_valid_construct(nmodl_text));
        }
    }
}

SCENARIO("Parser test for valid NMODL grammar constructs") {
    for (const auto& construct : nmodl_valid_constructs) {
        auto test_case = construct.second;
        GIVEN(test_case.name) {
            THEN("Parser successfully parses : " + test_case.nmodl_text) {
                REQUIRE(is_valid_construct(test_case.nmodl_text));
            }
        }
    }
}

SCENARIO("Parser test for invalid NMODL grammar constructs") {
    for (const auto& construct : nmdol_invalid_constructs) {
        auto test_case = construct.second;
        GIVEN(test_case.name) {
            THEN("Parser throws an exception while parsing : " + test_case.nmodl_text) {
                REQUIRE_THROWS(is_valid_construct(test_case.nmodl_text));
            }
        }
    }
}
