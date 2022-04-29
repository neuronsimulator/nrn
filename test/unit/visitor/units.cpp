/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <catch/catch.hpp>

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

std::string run_units_visitor(const std::string& text) {
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

    std::stringstream ss;

    // Visit AST to find all the ast::UnitDef nodes to print their
    // unit names, factors and dimensions as they are calculated in
    // the units::UnitTable
    const auto& unit_defs = collect_nodes(*ast, {ast::AstNodeType::UNIT_DEF});

    for (const auto& unit_def: unit_defs) {
        auto unit_name = unit_def->get_node_name();
        unit_name.erase(remove_if(unit_name.begin(), unit_name.end(), isspace), unit_name.end());
        auto unit = units_driver.table->get_unit(unit_name);
        ss << std::fixed << std::setprecision(8) << unit->get_name() << ' ' << unit->get_factor()
           << ':';
        // Dimensions of the unit are printed to check that the units are successfully
        // parsed to the units::UnitTable
        int dimension_id = 0;
        auto constant = true;
        for (const auto& dimension: unit->get_dimensions()) {
            if (dimension != 0) {
                constant = false;
                ss << ' ' << units_driver.table->get_base_unit_name(dimension_id) << dimension;
            }
            dimension_id++;
        }
        if (constant) {
            ss << " constant";
        }
        ss << '\n';
    }

    // Visit AST to find all the ast::FactorDef nodes to print their
    // unit names, factors and dimensions as they are calculated to
    // be printed to the produced .cpp file
    const auto& factor_defs = collect_nodes(*ast, {ast::AstNodeType::FACTOR_DEF});
    for (const auto& factor_def: factor_defs) {
        auto unit = units_driver.table->get_unit(factor_def->get_node_name());
        ss << std::fixed << std::setprecision(8) << unit->get_name() << ' ';
        auto factor_def_class = std::dynamic_pointer_cast<const nmodl::ast::FactorDef>(factor_def);
        ss << factor_def_class->get_value()->eval() << ':';
        // Dimensions of the unit are printed to check that the units are successfully
        // parsed to the units::UnitTable
        int dimension_id = 0;
        auto constant = true;
        for (const auto& dimension: unit->get_dimensions()) {
            if (dimension != 0) {
                constant = false;
                ss << ' ' << units_driver.table->get_base_unit_name(dimension_id);
                ss << dimension;
            }
            dimension_id++;
        }
        if (constant) {
            ss << " constant";
        }
        ss << '\n';
    }

    // check that, after visitor rearrangement, parents are still up-to-date
    CheckParentVisitor().check_ast(*ast);

    return ss.str();
}


SCENARIO("Parse UNITS block of mod files using Units Visitor", "[visitor][units]") {
    GIVEN("UNITS block with different cases of units definitions") {
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
                (mol) = (1)
                (M) = (1/liter)
                (uM1) = (micro M)
                (mA/cm2) = (nanoamp/cm2)
                (molar) = (1 / liter)
                (S ) = (siemens)
                (mse-1) = (1/millisec)
                (um3) = (liter/1e15)
                (molar1) = (/liter)
                (degK) = (degC)
                FARADAY1 = (faraday) (coulomb)
                FARADAY2 = (faraday) (kilocoulombs)
                FARADAY3 = (faraday) (10000 coulomb)
                PI      = (pi)      (1)
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
            }
        )";

        static const std::string output_nmodl = R"(
        nA 0.00000000: sec-1 coul1
        mA 0.00100000: sec-1 coul1
        mV 0.00100000: m2 kg1 sec-2 coul-1
        uS 0.00000100: m-2 kg-1 sec1 coul2
        nS 0.00000000: m-2 kg-1 sec1 coul2
        pS 0.00000000: m-2 kg-1 sec1 coul2
        umho 0.00000100: m-2 kg-1 sec1 coul2
        um 0.00000100: m1
        mM 1.00000000: m-3
        uM 0.00100000: m-3
        msM 0.00100000: m-3 sec1
        fAm 0.00000000: m1 sec-1 coul1
        mol 1.00000000: constant
        M 1000.00000000: m-3
        uM1 0.00100000: m-3
        mA/cm2 0.00001000: m-2 sec-1 coul1
        molar 1000.00000000: m-3
        S 1.00000000: m-2 kg-1 sec1 coul2
        mse-1 1000.00000000: sec-1
        um3 0.00100000: m3
        molar1 1000.00000000: m-3
        degK 1.00000000: K1
        FARADAY1 96485.3321233100141: coul1
        FARADAY2 96.4853321233100161: coul1
        FARADAY3 9.64853321233100125: coul1
        PI 3.14159265358979312: constant
        R1 8.3144626181532395: m2 kg1 sec-2 K-1
        R2 8.314: m2 kg1 sec-2 K-1
        R3 8314.46261815323851: m2 kg1 sec-2 K-1
        R4 8.314: m2 kg1 sec-2 K-1
        R5 8.314500000000001: m2 kg1 sec-2 K-1
        dummy1 123.45: m1 sec-2
        dummy2 123.45e3: m1 sec-2
        dummy3 12345e-2: m1 sec-2
        KTOMV 0.0853: m2 kg1 sec-2 coul-1 K-1
        B 0.26: m-1 coul-1
        TEMP 25: K1
        )";

        THEN("Print the units that were added") {
            const std::string input(reindent_text(nmodl_text));
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_units_visitor(input);
            auto reindented_result = reindent_text(result);
            REQUIRE(reindented_result == expected_result);
        }
    }
}
