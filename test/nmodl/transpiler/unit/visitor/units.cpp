/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include "ast/double.hpp"
#include "ast/factor_def.hpp"
#include "ast/program.hpp"
#include "parser/nmodl_driver.hpp"
#include "src/config/config.h"
#include "test/unit/utils/nmodl_constructs.hpp"
#include "test/unit/utils/test_utils.hpp"
#include "visitors/checkparent_visitor.hpp"
#include "visitors/units_visitor.hpp"

using namespace nmodl;
using namespace visitor;
using namespace test;
using namespace test_utils;

using nmodl::parser::NmodlDriver;

//=============================================================================
// Unit visitor tests
//=============================================================================

std::tuple<std::shared_ptr<ast::Program>, std::shared_ptr<units::UnitTable>> run_units_visitor(
    const std::string& text) {
    NmodlDriver driver;
    driver.parse_string(text);
    const auto& ast = driver.get_ast();

    // Parse nrnunits.lib file and the UNITS block of the mod file
    const std::string units_lib_path(NrnUnitsLib::get_path());
    UnitsVisitor units_visitor = UnitsVisitor(units_lib_path);

    units_visitor.visit_program(*ast);

    // Keep the UnitTable created from parsing unit file and UNITS
    // block of the mod file
    parser::UnitDriver units_driver = units_visitor.get_unit_driver();
    std::shared_ptr<units::UnitTable> unit_table = units_driver.table;

    // check that, after visitor rearrangement, parents are still up-to-date
    CheckParentVisitor().check_ast(*ast);

    return {ast, unit_table};
}


/**
 * @brief Returns all the \c UnitDef s of the \c ast::Program
 *
 * Visit AST to find all the ast::UnitDef nodes to print their
 * unit names, factors and dimensions as they are calculated in
 * the units::UnitTable
 * The \c UnitDef s are printed in the format:
 * \code
 * <unit_name> <unit_value>: <dimensions>
 * \endcode
 *
 * If the unit is constant then instead of the dimensions we print \c constant
 *
 * @arg ast \c ast::Program to look for \c UnitDef s
 * @arg unit_table \c units::UnitTable to look for the definitions of the units
 *
 * @return std::string Unit definitions
 */
std::string get_unit_definitions(const ast::Program& ast, const units::UnitTable& unit_table) {
    std::stringstream ss;
    const auto& unit_defs = collect_nodes(ast, {ast::AstNodeType::UNIT_DEF});

    for (const auto& unit_def: unit_defs) {
        auto unit_name = unit_def->get_node_name();
        unit_name.erase(remove_if(unit_name.begin(), unit_name.end(), isspace), unit_name.end());
        auto unit = unit_table.get_unit(unit_name);
        ss << fmt::format("{} {:g}:", unit_name, unit->get_factor());
        // Dimensions of the unit are printed to check that the units are successfully
        // parsed to the units::UnitTable
        int dimension_id = 0;
        auto constant = true;
        for (const auto& dimension: unit->get_dimensions()) {
            if (dimension != 0) {
                constant = false;
                ss << ' ' << unit_table.get_base_unit_name(dimension_id) << dimension;
            }
            dimension_id++;
        }
        if (constant) {
            ss << " constant";
        }
        ss << '\n';
    }
    return ss.str();
}

/**
 * @brief Returns all the \c FactorDef s of the \c ast::Program
 *
 * Visit AST to find all the ast::FactorDef nodes to print their
 * unit names and factors as they are calculated to be printed
 * to the generated .cpp file
 * The \c FactorDef s are printed in the format:
 * \code
 * <unit_name> <unit_value>
 * \endcode
 *
 * @arg ast \c ast::Program to look for \c FactorDef s
 *
 * @return std::string Factor definitions
 */
std::string get_factor_definitions(const ast::Program& ast) {
    std::stringstream ss;
    const auto& factor_defs = collect_nodes(ast, {ast::AstNodeType::FACTOR_DEF});
    for (const auto& factor_def: factor_defs) {
        const auto& factor_def_cast = std::dynamic_pointer_cast<const ast::FactorDef>(factor_def);
        ss << fmt::format("{} {}\n",
                          factor_def_cast->get_node_name(),
                          factor_def_cast->get_value()->get_value());
    }
    return ss.str();
}

SCENARIO("Parse UNITS block of mod files using Units Visitor", "[visitor][units]") {
    GIVEN("UNITS block with different cases of UNITS definitions") {
        static const std::string nmodl_text = R"(
            UNITS {
                (nA)    = (nanoamp)
                (mA)    = (milliamp)
                (mV)    = (millivolt)
                (uS)    = (microsiemens)
                (nS)    = (nanosiemens)
                (pS)    = (picosiemens)
                (umho)  = (micromho)
                (um)    = (micrometers)
                (mM)    = (milli/liter)
                (uM)    = (micro/liter)
                (msM) = (ms mM)
                (fAm) = (femto amp meter)
                (newmol) = (1)
                (M) = (1/liter)
                (uM1) = (micro M)
                (mA/cm2) = (nanoamp/cm2)
                (molar) = (1 / liter)
                (S ) = (siemens)
                (mse-1) = (1/millisec)
                (um3) = (liter/1e15)
                (molar1) = (/liter)
                (newdegK) = (degC)
                FARADAY1 = (faraday) (coulomb)
                FARADAY2 = (faraday) (kilocoulombs)
                FARADAY3 = (faraday) (10000 coulomb)
                pi      = (pi)      (1)
                R1       = (k-mole)  (joule/degC)
                R2 = 8.314 (volt-coul/degC)
                R3 = (mole k) (mV-coulomb/degC)
                R4 = 8.314 (volt-coul/degK)
                R5 = 8.314500000000001 (volt coul/kelvin)
                dummy1  = 123.45    (m 1/sec2)
                dummy2  = 123.45e3  (millimeters/sec2)
                dummy3  = 12345e-2  (m/sec2)
                KTOMV = 0.0853 (mV/degC)
                B = 0.26 (mM-cm2/mA-ms)
                TEMP = 25 (degC)
                toyfuzz = (1) (volt)
                numbertwo = 2 (1)
                oldJ = (R-mole) (1) : compute oldJ based on the value of R which is registered in the UnitTable
                R = 8 (joule/degC) : define a new value for R that should be visible only in the mod file
                J = (R-mole) (1) : recalculate J. It's value should be the same as oldJ because R shouldn't change in the UnitTable
                (myR) = (8 joule/degC) : Define my own R and mole and compute myRnew and myJ based on them
                (mymole) = (6+23)
                myRnew = (myR) (1)
                myJ = (myR-mymole) (1) 
            }
        )";

        static const std::string unit_definitions = R"(
        nA 1e-09: sec-1 coul1
        mA 0.001: sec-1 coul1
        mV 0.001: m2 kg1 sec-2 coul-1
        uS 1e-06: m-2 kg-1 sec1 coul2
        nS 1e-09: m-2 kg-1 sec1 coul2
        pS 1e-12: m-2 kg-1 sec1 coul2
        umho 1e-06: m-2 kg-1 sec1 coul2
        um 1e-06: m1
        mM 1: m-3
        uM 0.001: m-3
        msM 0.001: m-3 sec1
        fAm 1e-15: m1 sec-1 coul1
        newmol 1: constant
        M 1000: m-3
        uM1 0.001: m-3
        mA/cm2 1e-05: m-2 sec-1 coul1
        molar 1000: m-3
        S 1: m-2 kg-1 sec1 coul2
        mse-1 1000: sec-1
        um3 0.001: m3
        molar1 1000: m-3
        newdegK 1: K1
        myR 8: m2 kg1 sec-2 K-1
        mymole 6e+23: constant
        )";

        static const std::string factor_definitions = R"(
        FARADAY1 0x1.78e555060882cp+16
        FARADAY2 0x1.81f0fae775425p+6
        FARADAY3 0x1.34c0c8b92a9b7p+3
        pi 0x1.921fb54442d18p+1
        R1 0x1.0a1013e8990bep+3
        R2 8.314
        R3 0x1.03d3b37125759p+13
        R4 8.314
        R5 8.314500000000001
        dummy1 123.45
        dummy2 123.45e3
        dummy3 12345e-2
        KTOMV 0.0853
        B 0.26
        TEMP 25
        toyfuzz 1
        numbertwo 2
        oldJ 0x1.0912acba81b67p+82
        R 8
        J 0x1.0912acba81b67p+82
        myRnew 8
        myJ 0x1.fc3842bd1f072p+81
        )";

        THEN("Print the units that were added") {
            const std::string input(reindent_text(nmodl_text));
            auto expected_result_unit_definitions = reindent_text(unit_definitions);
            auto expected_result_factor_definitions = reindent_text(factor_definitions);
            const auto& [ast, unit_table] = run_units_visitor(input);
            const auto& generated_unit_definitions = get_unit_definitions(*ast, *unit_table);
            const auto& generated_factor_definitions = get_factor_definitions(*ast);
            auto reindented_result_unit_definitions = reindent_text(generated_unit_definitions);
            auto reindented_result_factor_definitions = reindent_text(generated_factor_definitions);
            REQUIRE(reindented_result_unit_definitions == expected_result_unit_definitions);
            REQUIRE(reindented_result_factor_definitions == expected_result_factor_definitions);
        }
    }
    GIVEN("UNITS block with Unit definition which is already defined") {
        static const std::string nmodl_text = R"(
            UNITS {
                (R) = (8 joule/degC)
            }
        )";
        THEN("Throw redefinition exception") {
            const std::string input(reindent_text(nmodl_text));
            REQUIRE_THROWS(run_units_visitor(input));
        }
    }
}
