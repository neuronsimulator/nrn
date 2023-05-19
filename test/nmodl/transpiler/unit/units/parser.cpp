/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <string>
#include <utility>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "config/config.h"
#include "parser/diffeq_driver.hpp"
#include "parser/unit_driver.hpp"
#include "test/unit/utils/test_utils.hpp"

//=============================================================================
// Parser tests
//=============================================================================

using namespace nmodl::test_utils;
using Catch::Matchers::ContainsSubstring;

// Driver is defined as global to store all the units inserted to it and to be
// able to define complex units based on base units
nmodl::parser::UnitDriver driver;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

bool is_valid_construct(const std::string& construct) {
    return driver.parse_string(construct);
}

std::string parse_string(const std::string& unit_definition) {
    nmodl::parser::UnitDriver correctness_driver;
    correctness_driver.parse_file(nmodl::NrnUnitsLib::get_path());
    correctness_driver.parse_string(unit_definition);
    std::stringstream ss;
    correctness_driver.table->print_units_sorted(ss);
    correctness_driver.table->print_base_units(ss);
    return ss.str();
}

SCENARIO("Unit parser accepting valid units definition", "[unit][parser]") {
    GIVEN("A base unit") {
        WHEN("Base unit is *a*") {
            THEN("parser accepts without an error") {
                REQUIRE(is_valid_construct("m   *a*\n"));
            }
        }
        WHEN("Base unit is *b*") {
            THEN("parser accepts without an error") {
                REQUIRE(is_valid_construct("kg\t\t\t*b*\n"));
            }
        }
        WHEN("Base unit is *d*") {
            THEN("parser accepts without an error") {
                REQUIRE(is_valid_construct("coul\t\t\t*d*\n"));
            }
        }
        WHEN("Base unit is *i*") {
            THEN("parser accepts without an error") {
                REQUIRE(is_valid_construct("erlang\t\t\t*i*\n"));
            }
        }
        WHEN("Base unit is *c*") {
            THEN("parser accepts without an error") {
                REQUIRE(is_valid_construct("sec\t\t\t*c*\n"));
            }
        }
    }
    GIVEN("A double number") {
        WHEN("Double number is writen like 3.14") {
            THEN("parser accepts without an error") {
                REQUIRE(is_valid_construct("pi\t\t\t3.14159265358979323846\n"));
            }
        }
        WHEN("Double number is writen like 1") {
            THEN("parser accepts without an error") {
                REQUIRE(is_valid_construct("fuzz\t\t\t1\n"));
            }
        }
        WHEN("Double number is negative") {
            THEN("parser accepts without an error") {
                REQUIRE(is_valid_construct("ckan\t\t\t-32.19976 m\n"));
            }
        }
    }
    GIVEN("A dimensionless constant") {
        WHEN("Constant expression is double / constant") {
            THEN("parser accepts without an error") {
                REQUIRE(is_valid_construct("radian\t\t\t.5 / pi\n"));
            }
        }
    }
    GIVEN("A power of another unit") {
        WHEN("Power of 2") {
            THEN("parser accepts without an error") {
                REQUIRE(is_valid_construct("steradian\t\tradian2\n"));
            }
        }
        WHEN("Power of 3") {
            THEN("parser accepts without an error") {
                REQUIRE(is_valid_construct("stere\t\t\tm3\n"));
            }
        }
    }
    GIVEN("Divisions and multiplications of units") {
        WHEN("Units are multiplied") {
            THEN("parser accepts without an error") {
                REQUIRE(is_valid_construct("degree\t\t\t1|180 pi-radian\n"));
            }
        }
        WHEN("There are both divisions and multiplications") {
            THEN("parser accepts without an error") {
                REQUIRE(is_valid_construct("newton\t\t\tkg-m/sec2\n"));
            }
        }
        WHEN("There is only division") {
            THEN("parser accepts without an error") {
                REQUIRE(is_valid_construct("dipotre\t\t\t/m\n"));
            }
        }
        WHEN("Nominator is unknown") {
            THEN("it throws") {
                REQUIRE_THROWS(parse_string("foo 1 pew/m\n"));
            }
        }
        WHEN("Denominator is unknown") {
            THEN("it throws") {
                REQUIRE_THROWS(parse_string("foo 1 m/pew\n"));
            }
        }
    }
    GIVEN("A double number and some units") {
        WHEN("Double number is multiplied by a power of 10 with division of multiple units") {
            THEN("parser accepts without an error") {
                REQUIRE(is_valid_construct("c\t\t\t2.99792458+8 m/sec fuzz\n"));
            }
        }
        WHEN("Double number is writen like .9") {
            THEN("parser accepts without an error") {
                REQUIRE(is_valid_construct("grade\t\t\t.9 degree\n"));
            }
        }
        WHEN("A 's' is added") {
            THEN("parser remove it to find the units") {
                REQUIRE_NOTHROW(parse_string("pew 1 m\nfoo 2 pews\n"));
                REQUIRE_NOTHROW(parse_string("pew 1 m\nfoo 2 /pews\n"));
            }
        }
        WHEN("No unit but only a prefix factor") {
            THEN("parser multiply the number by the factor") {
                std::string parsed_unit{};
                REQUIRE_NOTHROW(parsed_unit = parse_string("pew 1 1/milli"));
                REQUIRE_THAT(parsed_unit, ContainsSubstring("pew 0.001: 0 0 0 0 0 0 0 0 0 0"));
            }
        }
    }
    GIVEN("A fraction and some units") {
        WHEN("Fraction is writen like 1|2") {
            THEN("parser accepts without an error") {
                REQUIRE(is_valid_construct("ccs\t\t\t1|36 erlang\n"));
            }
        }
        WHEN("Fraction is writen like 1|8.988e9") {
            THEN("parser accepts without an error") {
                REQUIRE(is_valid_construct("statcoul\t\t1|2.99792458+9 coul\n"));
            }
        }
    }
}

SCENARIO("Unit parser accepting dependent/nested units definition", "[unit][parser]") {
    GIVEN("Parsed the nrnunits.lib file") {
        WHEN("Multiple units definitions based on the units defined in nrnunits.lib") {
            THEN("parser accepts the units correctly") {
                std::string units_definitions = R"(
                mV      millivolt
                mM      milli/liter
                mA      milliamp
                KTOMV   0.0853 mV/degC
                B       0.26 mM-cm2/mA-ms
                dummy1  .025 1/m2
                dummy2  1|4e+1 m/m3
                dummy3  25-3  / m2
                dummy4  -0.025 /m2
                dummy5  2.5 %
                newR       k-mole
                R1      8.314 volt-coul/degC
                R2      8314 mV-coul/degC
                )";
                std::string parsed_units = parse_string(reindent_text(units_definitions));
                REQUIRE_THAT(parsed_units, ContainsSubstring("mV 0.001: 2 1 -2 -1 0 0 0 0 0 0"));
                REQUIRE_THAT(parsed_units, ContainsSubstring("mM 1: -3 0 0 0 0 0 0 0 0 0"));
                REQUIRE_THAT(parsed_units, ContainsSubstring("mA 0.001: 0 0 -1 1 0 0 0 0 0 0"));
                REQUIRE_THAT(parsed_units,
                             ContainsSubstring("KTOMV 8.53e-05: 2 1 -2 -1 0 0 0 0 0 -1"));
                REQUIRE_THAT(parsed_units, ContainsSubstring("B 26: -1 0 0 -1 0 0 0 0 0 0"));
                REQUIRE_THAT(parsed_units, ContainsSubstring("dummy1 0.025: -2 0 0 0 0 0 0 0 0 0"));
                REQUIRE_THAT(parsed_units, ContainsSubstring("dummy2 0.025: -2 0 0 0 0 0 0 0 0 0"));
                REQUIRE_THAT(parsed_units, ContainsSubstring("dummy3 0.025: -2 0 0 0 0 0 0 0 0 0"));
                REQUIRE_THAT(parsed_units,
                             ContainsSubstring("dummy4 -0.025: -2 0 0 0 0 0 0 0 0 0"));
                REQUIRE_THAT(parsed_units, ContainsSubstring("dummy5 0.025: 0 0 0 0 0 0 0 0 0 0"));
                REQUIRE_THAT(parsed_units,
                             ContainsSubstring("newR 8.31446: 2 1 -2 0 0 0 0 0 0 -1"));
                REQUIRE_THAT(parsed_units, ContainsSubstring("R1 8.314: 2 1 -2 0 0 0 0 0 0 -1"));
                REQUIRE_THAT(parsed_units, ContainsSubstring("R2 8.314: 2 1 -2 0 0 0 0 0 0 -1"));
                REQUIRE_THAT(parsed_units,
                             ContainsSubstring("m kg sec coul candela dollar bit erlang K"));
            }
        }
    }
}
