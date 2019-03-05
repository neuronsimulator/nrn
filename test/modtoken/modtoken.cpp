/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#define CATCH_CONFIG_MAIN

#include <string>

#include "catch/catch.hpp"
#include "lexer/modtoken.hpp"
#include "lexer/nmodl_lexer.hpp"
#include "parser/nmodl_driver.hpp"

using namespace nmodl;
using nmodl::parser::NmodlDriver;
using nmodl::parser::NmodlLexer;

/// retrieve token from lexer
template <typename T>
void symbol_type(const std::string& name, T& value) {
    std::istringstream ss(name);
    std::istream& in = ss;

    NmodlDriver driver;
    NmodlLexer scanner(driver, &in);

    auto sym = scanner.next_token();
    value = sym.value.as<T>();
}

/// test symbol type returned by lexer
TEST_CASE("Lexer symbol type tests", "[TokenPrinter]") {
    SECTION("Symbol type : name ast class test") {
        ast::Name value;

        {
            std::stringstream ss;
            symbol_type("text", value);
            ss << *(value.get_token());
            REQUIRE(ss.str() == "           text at [1.1-4] type 356");
        }

        {
            std::stringstream ss;
            symbol_type("  some_text", value);
            ss << *(value.get_token());
            REQUIRE(ss.str() == "      some_text at [1.3-11] type 356");
        }
    }

    SECTION("Symbol type : prime ast class test") {
        ast::PrimeName value;

        {
            std::stringstream ss;
            symbol_type("h'' = ", value);
            ss << *(value.get_token());
            REQUIRE(ss.str() == "            h'' at [1.1-3] type 362");
            REQUIRE(value.get_order()->eval() == 2);
        }
    }
}
