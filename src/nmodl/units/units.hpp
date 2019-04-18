#include <utility>

/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>


namespace nmodl {
namespace units {

/// Maximum number of dimensions of units (maximum number of base units)
static const int MAX_DIMS = 10;

/**
 * \class Unit
 * \brief Class that stores all the data of a Unit
 *
 * The Unit class encapsulates all the variables and containers that are related to
 * the definition of a unit. Those are:
 * - Unit factor
 * - Unit dimensions
 * - Unit name
 * - Nominator units
 * - Denominator units
 *
 * The unit factor is calculated based on the unit factor if it is stated and the
 * factors of the units that describe the units and are on the nominator or the
 * denominator of the unit. The calculation of the factor is done by the UnitTable
 * class, as it is needed to check the factors and dimensions of the units that
 * this unit is based upon and are stored into the UnitTable table that keeps all
 * the units parsed from a units file (nrnunits.lib by default) or the mod files.
 * The dimensions of the unit represent which basic units are used to define this
 * unit. They are represented by an array of ints of size MAX_DIMS. An array was
 * used, as the base units do not change based on the SI units. If a base unit is
 * used in this unit's definition then there is a 1 in the responding or else 0.
 * If a unit is on the nominator of the unit definition, then its factor is
 * multiplied by the unit's factor, while if it is on the denominator, its factor
 * is divided by the unit's factor. The dimensions of the nominator units are added
 * to the dimensions of the units and the dimensions of the denominator are
 * subtracted from the unit's dimensions. The Unit representation is designed based
 * on the units representation of the MOD2C parser.
 */
class Unit {
  private:
    /// Double factor of the Unit
    double unit_factor = 1.0;

    /// Array of MAX_DIMS size that keeps the Unit's dimensions
    std::array<int, MAX_DIMS> unit_dimensions = {0};

    /// Name of the Unit
    std::string unit_name;

    /// Vector of nominators of the Unit
    std::vector<std::string> nominator;

    /// Vector of denominators of the Unit
    std::vector<std::string> denominator;

  public:
    /// \name Ctor & dtor
    /// \{

    /// Default constructor of Unit
    Unit() = default;

    /// Constructor for simply creating a Unit with a given name
    explicit Unit(std::string name)
        : unit_name(std::move(name)) {}

    /// Constructor that instantiates a Unit with its factor, dimensions and name
    Unit(const double factor, std::array<int, MAX_DIMS> dimensions, std::string name)
        : unit_factor(factor)
        , unit_dimensions(dimensions)
        , unit_name(std::move(name)) {}

    /// \}

    /// Add unit name to the Unit
    void add_unit(std::string name);

    /// If the Unit is a base unit the dimensions of the Unit should be calculated
    /// based on the name of the base unit (ex. m   \*a\* => m has dimension 0)
    void add_base_unit(std::string name);

    /// Takes as argument a double as string, parses it as double and stores it to
    /// the Unit factor
    void add_nominator_double(std::string double_string);

    /// Add the dimensions of a nominator of the unit to the dimensions of the Unit
    void add_nominator_dims(std::array<int, MAX_DIMS> dimensions);

    /// Subtract the dimensions of a nominator of the unit to the dimensions of the Unit
    void add_denominator_dims(std::array<int, MAX_DIMS> dimensions);

    /// Add a unit to the vector of nominator strings of the Unit, so it can be processed
    /// later
    void add_nominator_unit(std::string nom);

    /// Add a vector of units to the vector of nominator strings of the Unit, so they can
    /// be processed later
    void add_nominator_unit(std::shared_ptr<std::vector<std::string>> nom);

    /// Add a unit to the vector of denominator strings of the Unit, so it can be processed
    /// later
    void add_denominator_unit(std::string denom);

    /// Add a vector of units to the vector of denominator strings of the Unit, so they can
    /// be processed later
    void add_denominator_unit(std::shared_ptr<std::vector<std::string>> denom);

    /// Multiply Unit's factor with a double factor
    void mul_factor(double double_factor);

    /// Parse a fraction given as string and store the result to the factor of the Unit
    void add_fraction(const std::string& fraction_string);

    /// Parse a double number given as string. The double can be positive or negative and
    /// have all kinds of representations
    double parse_double(std::string double_string);

    /// Getter for the vector of nominators of the Unit
    std::vector<std::string> get_nominator_unit() const {
        return nominator;
    }

    /// Getter for the vector of denominators of the Unit
    std::vector<std::string> get_denominator_unit() const {
        return denominator;
    }

    /// Getter for the name of the Unit
    std::string get_name() const {
        return unit_name;
    }

    /// Getter for the double factor of the Unit
    double get_factor() const {
        return unit_factor;
    }

    /// Getter for the array of Unit's dimensions
    std::array<int, MAX_DIMS> get_dims() const {
        return unit_dimensions;
    }
};

/**
 * \class Prefix
 * \brief Class that stores all the data of a prefix
 *
 * Prefixes of units can also be defined in the units file. Those
 * prefixes are then checked during a unit's insertion to the UnitTable
 * to multiply their factor to the unit's factor.
 *
 */
class Prefix {
  private:
    /// Prefix's double factor
    double prefix_factor = 1;

    /// Prefix's name
    std::string prefix_name;

  public:
    /// \name Ctor & dtor
    /// \{

    /// Default constructor for Prefix
    Prefix() = default;

    /// Constructor that instantiates a Prefix with its name and factor
    Prefix(std::string name, std::string factor);

    /// \}

    /// Getter for the name of the Prefix
    std::string get_name() const {
        return prefix_name;
    }

    /// Getter for the factor of the Prefix
    double get_factor() const {
        return prefix_factor;
    }
};

/**
 * \class UnitTable
 * \brief Class that stores all the units, prefixes and names of base units used
 *
 * The UnitTable encapsulates all the containers that are needed to store all the
 * Units defined. Those containers are:
 * - A table (hash map) that stores all the Units
 * - A hash map that stores all the Prefixes
 * - An array to store the base units names
 *
 * The names and the design is based on the corresponding ones of MOD2C.
 * The table is a hash map that uses as key the name of the Unit and as objects
 * a shared_ptr to a Unit. This is needed as the parser passes to the UnitTable
 * shared_ptrs, so that there is no need to allocate more space for the units or
 * take care of the deallocation of the memory, which is done automatically by
 * the smart pointer.
 * A shared_ptr to a UnitTable is kept by the UnitDriver, to be able to populate the
 * UnitTable by using multiple sources (files and strings).
 * The UnitTable takes care of inserting Units, Prefixes and base units to the table
 * calculating or the needed factors and dimensions.
 *
 */
class UnitTable {
  private:
    /// Hash map that stores all the Units
    std::unordered_map<std::string, std::shared_ptr<Unit>> table;

    /// Hash map that stores all the Prefixes
    std::unordered_map<std::string, double> prefixes;

    /// Hash map that stores all the base units' names
    std::array<std::string, MAX_DIMS> base_units_names;

  public:
    /// Default constructor for UnitTable
    UnitTable() = default;

    /// Calculate unit's dimensions based on its nominator unit named nominator_name which is
    /// stored in the UnitTable's table
    void calc_nominator_dims(std::shared_ptr<Unit> unit, std::string nominator_name);

    /// Calculate unit's dimensions based on its denominator unit named denominator_name which is
    /// stored in the UnitTable's table
    void calc_denominator_dims(std::shared_ptr<Unit> unit, std::string denominator_name);

    /// Insert a unit to the UnitTable table and calculate its dimensions and factor based on the
    /// previously stored units in the UnitTable
    /// The unit can be a normal unit or unit based on just a base unit. In the latter case, the
    /// base unit is also added to the base_units_name array of UnitTable
    void insert(std::shared_ptr<Unit> unit);

    /// Insert a prefix to the prefixes of the UnitTable
    void insert_prefix(std::shared_ptr<Prefix> prfx);

    /// Get the unit_name of the UnitTable's table
    std::shared_ptr<Unit> get_unit(const std::string& unit_name) {
        return table[unit_name];
    }

    /// Print the details of the units that are stored in the UnitTable
    void print_units() const;

    /// Print the details of the units that are stored in the UnitTable
    /// to the stringstream units_details in ascending order to be printed
    /// in tests in specific order
    void print_units_sorted(std::stringstream& units_details);

    /// Print the base units that are stored in the UnitTable
    void print_base_units() const;

    /// Print the base units that are stored in the UnitTable to the
    /// stringstream base_units_details
    void print_base_units(std::stringstream& base_units_details);
};

}  // namespace units
}  // namespace nmodl
