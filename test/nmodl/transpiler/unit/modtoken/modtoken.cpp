/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <memory.h>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "lexer/modtoken.hpp"
#include "lexer/nmodl_lexer.hpp"
#include "parser/nmodl_driver.hpp"
#include "test/unit/utils/test_utils.hpp"


/** @file
 *  @defgroup token_test Token Tests
 *  @ingroup token
 *  @brief All tests for @ref token_modtoken
 *  @{
 */

using namespace nmodl;
using nmodl::parser::NmodlDriver;
using nmodl::parser::NmodlLexer;
using LocationType = nmodl::parser::location;

template <typename T>
void symbol_type(const std::string& name, T& value) {
    std::istringstream ss(name);
    std::istream& in = ss;

    NmodlDriver driver;
    NmodlLexer scanner(driver, &in);

    auto sym = scanner.next_token();
    value = sym.value.as<T>();
}

TEST_CASE("NMODL Lexer returning valid ModToken object", "[token][modtoken]") {
    SECTION("test for ast::Name") {
        ast::Name value;
        {
            std::stringstream ss;
            symbol_type("text", value);
            ss << *(value.get_token());
            REQUIRE(ss.str() == "           text at [1.1-4] type 343");
        }

        {
            std::stringstream ss;
            symbol_type("  some_text", value);
            ss << *(value.get_token());
            REQUIRE(ss.str() == "      some_text at [1.3-11] type 343");
        }
    }

    SECTION("test for ast::Prime") {
        ast::PrimeName value;
        {
            std::stringstream ss;
            symbol_type("h'' = ", value);
            ss << *(value.get_token());
            REQUIRE(ss.str() == "            h'' at [1.1-3] type 350");
            REQUIRE(value.get_order()->eval() == 2);
        }
    }
}

TEST_CASE("Addition of two ModToken objects", "[token][modtoken]") {
    SECTION("adding two random strings") {
        ast::Name value;
        {
            std::stringstream ss;

            // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
            nmodl::parser::position adder1_begin(nullptr, 1, 1);
            nmodl::parser::position adder1_end(nullptr, 1, 5);
            LocationType adder1_location(adder1_begin, adder1_end);
            ModToken adder1("text", 1, adder1_location);

            nmodl::parser::position adder2_begin(nullptr, 2, 1);
            nmodl::parser::position adder2_end(nullptr, 2, 5);
            // NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
            LocationType adder2_location(adder2_begin, adder2_end);
            ModToken adder2("text", 2, adder2_location);

            ss << adder1;
            ss << " + ";
            ss << adder2;

            ModToken sum = adder1 + adder2;
            ss << " = " << sum;
            REQUIRE(ss.str() ==
                    "           text at [1.1-4] type 1 +            text at [2.1-4] type 2 =   "
                    "         text at [1.1-2.4] type 1");
        }
    }
}


/** @} */  // end of token_test
