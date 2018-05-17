#define CATCH_CONFIG_MAIN

#include <string>

#include "catch/catch.hpp"
#include "lexer/modtoken.hpp"
#include "lexer/nmodl_lexer.hpp"
#include "parser/nmodl_driver.hpp"

/// retrieve token from lexer
template <typename T>
void symbol_type(const std::string& name, T& value) {
    std::istringstream ss(name);
    std::istream& in = ss;

    nmodl::Driver driver;
    nmodl::Lexer scanner(driver, &in);

    nmodl::Parser::symbol_type sym = scanner.next_token();
    value = sym.value.as<T>();
}

/// test symbol type returned by lexer
TEST_CASE("Lexer symbol type tests", "[TokenPrinter]") {
    SECTION("Symbol type : name ast class test") {
        ast::name_ptr value = nullptr;

        {
            std::stringstream ss;
            symbol_type("text", value);
            ss << *(value->get_token());
            REQUIRE(ss.str() == "           text at [1.1-4] type 356");
            delete value;
        }

        {
            std::stringstream ss;
            symbol_type("  some_text", value);
            ss << *(value->get_token());
            REQUIRE(ss.str() == "      some_text at [1.3-11] type 356");
            delete value;
        }
    }

    SECTION("Symbol type : prime ast class test") {
        ast::primename_ptr value = nullptr;

        {
            std::stringstream ss;
            symbol_type("h'' = ", value);
            ss << *(value->get_token());
            REQUIRE(ss.str() == "            h'' at [1.1-3] type 362");
            REQUIRE(value->get_order() == 2);
            delete value;
        }
    }
}
