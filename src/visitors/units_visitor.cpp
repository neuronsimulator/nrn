/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "visitors/units_visitor.hpp"
#include "utils/string_utils.hpp"

#include "ast/all.hpp"

/**
 * \file
 * \brief AST Visitor to parse the ast::UnitDefs and ast::FactorDefs from the mod file
 * by the Units Parser used to parse the \c nrnunits.lib file
 */

namespace nmodl {
namespace visitor {

void UnitsVisitor::visit_program(ast::Program& node) {
    units_driver.parse_file(units_dir);
    node.visit_children(*this);
}

/**
 * \details units::Unit definition is based only on pre-defined units, parse only the
 * new unit and the pre-defined units. <br>
 * Example:
 * \code
 *      (nA)    = (nanoamp) => nA  nanoamp)
 * \endcode
 * The ast::UnitDef is converted to a string that is able to be parsed by the
 * unit parser which was used for parsing the \c nrnunits.lib file.
 * On \c nrnunits.lib constant "1" is defined as "fuzz", so it must be converted.
 */
void UnitsVisitor::visit_unit_def(ast::UnitDef& node) {
    std::ostringstream ss;
    /*
     * In nrnunits.lib file "1" is defined as "fuzz", so there
     * must be a conversion to be able to parse "1" as unit
     */
    if (node.get_unit2()->get_node_name() == "1") {
        ss << node.get_unit1()->get_node_name() << '\t';
        ss << UNIT_FUZZ;
    } else {
        ss << node.get_unit1()->get_node_name() << '\t' << node.get_unit2()->get_node_name();
    }

    // Parse the generated string for the defined unit using the units::UnitParser
    units_driver.parse_string(ss.str());
}

/**
 * \details The new unit definition is based on a factor combined with
 * units or other defined units.
 * In the first case the factor saved to the ast::FactorDef node and
 * printed to \c .cpp file is the one defined on the modfile. The factor
 * and the dimensions saved to the units::UnitTable are based on the
 * factor and the units defined in the modfile, so this factor will be
 * calculated based on the base units of the units::UnitTable. <br>
 * Example:
 * \code
 *      R = 8.314 (volt-coul/degC))
 * \endcode
 * In the second case, the factor and the dimensions that are inserted
 * to the units::UnitTable are based on the ast::FactorDef::unit1, like
 * in MOD2C.
 * \b However, the factor that is saved in the ast::FactorDef and printed
 * in the \c .cpp file is the factor of the ast::FactorDef::unit1 divided
 * by the factor of ast::FactorDef::unit2. <br>
 * Example:
 * \code
 *      R = (mole k) (mV-coulomb/degC)
 * \endcode
 * `unit1` is `mole k` and unit2 is `mV-coulomb/degC` <br>
 * To parse the units defined in modfiles there are stringstreams
 * created that are passed to the string parser, to be parsed by the
 * unit parser used for parsing the \c nrnunits.lib file, which takes
 * care of all the units calculations.
 */
void UnitsVisitor::visit_factor_def(ast::FactorDef& node) {
    std::ostringstream ss;
    const auto node_has_value_defined_in_modfile = node.get_value() != nullptr;
    if (node_has_value_defined_in_modfile) {
        /*
         * In nrnunits.lib file "1" is defined as "fuzz", so
         * there must be a conversion to be able to parse "1" as unit
         */
        if (node.get_unit1()->get_node_name() == "1") {
            ss << node.get_node_name() << "\t" << node.get_value()->eval() << " ";
            ss << UNIT_FUZZ;
        } else {
            ss << node.get_node_name() << "\t" << node.get_value()->eval() << " ";
            ss << node.get_unit1()->get_node_name();
        }
        // Parse the generated string for the defined unit using the units::UnitParser
        units_driver.parse_string(ss.str());
    } else {
        std::ostringstream ss_unit1, ss_unit2;
        std::string unit1_name, unit2_name;
        /*
         * In nrnunits.lib file "1" is defined as "fuzz", so
         * there must be a conversion to be able to parse "1" as unit
         */
        if (node.get_unit1()->get_node_name() == "1") {
            unit1_name = UNIT_FUZZ;
        } else {
            unit1_name = node.get_unit1()->get_node_name();
        }
        if (node.get_unit2()->get_node_name() == "1") {
            unit2_name = UNIT_FUZZ;
        } else {
            unit2_name = node.get_unit2()->get_node_name();
        }
        /*
         * Create dummy unit "node.get_node_name()_unit1" and parse
         * it to calculate its factor
         */
        ss_unit1 << node.get_node_name() << "_unit1\t" << unit1_name;
        units_driver.parse_string(ss_unit1.str());
        /*
         * Create dummy unit "node.get_node_name()_unit2" and parse
         * it to calculate its factor
         */
        ss_unit2 << node.get_node_name() << "_unit2\t" << unit2_name;
        units_driver.parse_string(ss_unit2.str());

        // Parse the generated string for the defined unit using the units::UnitParser
        ss << node.get_node_name() << "\t" << unit1_name;
        units_driver.parse_string(ss.str());

        /**
         * \note If the ast::FactorDef was made by using two units (second case),
         * the factors of both of them must be calculated based on the
         * units::UnitTable and then they must be divided to produce the unit's
         * factor that will be printed to the \c .cpp file. <br>
         * Example:
         * \code
         *      FARADAY = (faraday) (10000 coulomb)
         * \endcode
         * In the \c .cpp file the printed factor will be `9.64853090` but in the
         * units::UnitTable the factor of `FARADAY` will be `96485.30900000`
         */

        auto node_unit_name = node.get_node_name();
        auto unit1_factor = units_driver.table->get_unit(node_unit_name + "_unit1")->get_factor();
        auto unit2_factor = units_driver.table->get_unit(node_unit_name + "_unit2")->get_factor();

#ifdef USE_LEGACY_UNITS
        auto unit_factor = stringutils::to_string(unit1_factor / unit2_factor, "{:g}");
#else
        auto unit_factor = stringutils::to_string(unit1_factor / unit2_factor, "{:.18g}");
#endif

        auto double_value_ptr = std::make_shared<ast::Double>(ast::Double(unit_factor));
        node.set_value(std::move(double_value_ptr));
    }
}

}  // namespace visitor
}  // namespace nmodl
