#define CATCH_CONFIG_MAIN

#include <string>

#include "catch/catch.hpp"
#include "symtab/symbol_properties.hpp"

using namespace symtab::details;

extern bool has_property(const SymbolInfo& obj, NmodlInfo property);

//=============================================================================
// Symbol properties tests
//=============================================================================

TEST_CASE("Symbol Properties") {

    SECTION("Properties Operation 1") {
        SymbolInfo properties;
        REQUIRE(to_string(properties) == "");

        properties |= NmodlInfo::local_var;
        REQUIRE(to_string(properties) == "local");

        properties |= NmodlInfo::write_ion_var;
        REQUIRE(to_string(properties) == "local write_ion");

        REQUIRE(has_property(properties, NmodlInfo::local_var) == true);
        REQUIRE(has_property(properties, NmodlInfo::global_var) == false);
    }

    SECTION("Properties Operation 2") {
        SymbolInfo prop1 = (NmodlInfo::local_var | NmodlInfo::global_var);
        SymbolInfo prop2 = NmodlInfo::local_var;

        REQUIRE(prop1 != prop2);

        prop2 |= NmodlInfo::global_var;

        REQUIRE(prop1 == prop2);
    }

}