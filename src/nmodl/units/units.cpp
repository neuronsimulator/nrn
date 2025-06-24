/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <algorithm>
#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "units.hpp"
#include "utils/fmt.h"

/**
 * \file
 * \brief Units processing while being processed from lexer and parser
 */

namespace nmodl {
namespace units {

Prefix::Prefix(std::string name, const std::string& factor) {
    if (name.back() == '-') {
        name.pop_back();
    }
    prefix_name = name;
    prefix_factor = std::stod(factor);
}

void Unit::add_unit(const std::string& name) {
    unit_name = name;
}

void Unit::add_base_unit(const std::string& name) {
    // name = "*[a-j]*" which is a base unit
    const auto dim_name = name[1];
    const int dim_no = dim_name - 'a';
    assert(dim_no >= 0 && dim_no < unit_dimensions.size());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    unit_dimensions[dim_no] = 1;
    add_nominator_unit(name);
}

void Unit::add_nominator_double(const std::string& double_string) {
    unit_factor = parse_double(double_string);
}

void Unit::add_nominator_dims(const std::array<int, MAX_DIMS>& dimensions) {
    std::transform(unit_dimensions.begin(),
                   unit_dimensions.end(),
                   dimensions.begin(),
                   unit_dimensions.begin(),
                   std::plus<int>());
}

void Unit::add_denominator_dims(const std::array<int, MAX_DIMS>& dimensions) {
    std::transform(unit_dimensions.begin(),
                   unit_dimensions.end(),
                   dimensions.begin(),
                   unit_dimensions.begin(),
                   std::minus<int>());
}

void Unit::add_nominator_unit(const std::string& nom) {
    nominator.push_back(nom);
}

void Unit::add_nominator_unit(const std::shared_ptr<std::vector<std::string>>& nom) {
    nominator.insert(nominator.end(), nom->begin(), nom->end());
}

void Unit::add_denominator_unit(const std::string& denom) {
    denominator.push_back(denom);
}

void Unit::add_denominator_unit(const std::shared_ptr<std::vector<std::string>>& denom) {
    denominator.insert(denominator.end(), denom->begin(), denom->end());
}

void Unit::mul_factor(const double double_factor) {
    unit_factor *= double_factor;
}

void Unit::add_fraction(const std::string& nominator, const std::string& denominator) {
    double nom = parse_double(nominator);
    double denom = parse_double(denominator);
    unit_factor = nom / denom;
}

/**
 * Double numbers in the \c nrnunits.lib file are defined in the form [.0-9]+[-+][0-9]+
 * To make sure they are parsed in the correct way and similarly to the NEURON parser
 * we convert this string to a string compliant to scientific notation to be able to be parsed
 * from std::stod(). To do that we have to add an `e` in case there is `-+` in the number
 *string if it doesn't exist.
 */
double Unit::parse_double(std::string double_string) {
    auto pos = double_string.find_first_of("+-", 1);  // start at 1 pos, because we don't care with
                                                      // the front sign
    if (pos != std::string::npos && double_string.find_last_of("eE", pos) == std::string::npos) {
        double_string.insert(pos, "e");
    }
    return std::stod(double_string);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void UnitTable::calc_nominator_dims(const std::shared_ptr<Unit>& unit, std::string nominator_name) {
    double nominator_prefix_factor = 1.0;
    int nominator_power = 1;

    // if the nominator is DOUBLE, divide it from the unit factor
    if (nominator_name.front() >= '1' && nominator_name.front() <= '9') {
        unit->mul_factor(1 / std::stod(nominator_name));
        return;
    }

    std::string nom_name = nominator_name;
    auto nominator = table.find(nominator_name);

    // if the nominator_name is not in the table, check if there are any prefixes or power
    if (nominator == table.end()) {
        int changed_nominator_name = 1;

        while (changed_nominator_name) {
            changed_nominator_name = 0;
            for (const auto& it: prefixes) {
                auto res = std::mismatch(it.first.begin(),
                                         it.first.end(),
                                         nominator_name.begin(),
                                         nominator_name.end());
                if (res.first == it.first.end()) {
                    changed_nominator_name = 1;
                    nominator_prefix_factor *= it.second;
                    nominator_name.erase(nominator_name.begin(),
                                         nominator_name.begin() +
                                             static_cast<std::ptrdiff_t>(it.first.size()));
                }
            }
        }
        // if the nominator is only a prefix, just multiply the prefix factor with the unit factor
        if (nominator_name.empty()) {
            for (const auto& it: prefixes) {
                auto res = std::mismatch(it.first.begin(), it.first.end(), nom_name.begin());
                if (res.first == it.first.end()) {
                    unit->mul_factor(it.second);
                    return;
                }
            }
        }

        // if the nominator_back is a UNIT_POWER save the power to be able
        // to calculate the correct factor and dimensions later
        char nominator_back = nominator_name.back();
        if (nominator_back >= '2' && nominator_back <= '9') {
            nominator_power = nominator_back - '0';
            nominator_name.pop_back();
        }

        nominator = table.find(nominator_name);

        // delete "s" char for plural of the nominator_name
        if (nominator == table.end()) {
            if (nominator_name.back() == 's') {
                nominator_name.pop_back();
            }
            nominator = table.find(nominator_name);
        }
    }

    // if the nominator is still not found in the table then output error
    // else multiply its factor to the unit factor and calculate unit's dimensions
    if (nominator == table.end()) {
        std::string ss = fmt::format("Unit {} not defined!", nominator_name);
        throw std::runtime_error(ss);
    } else {
        for (int i = 0; i < nominator_power; i++) {
            unit->mul_factor(nominator_prefix_factor * nominator->second->get_factor());
            unit->add_nominator_dims(nominator->second->get_dimensions());
        }
    }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void UnitTable::calc_denominator_dims(const std::shared_ptr<Unit>& unit,
                                      std::string denominator_name) {
    double denominator_prefix_factor = 1.0;
    int denominator_power = 1;

    // if the denominator is DOUBLE, divide it from the unit factor
    if (denominator_name.front() >= '1' && denominator_name.front() <= '9') {
        unit->mul_factor(std::stod(denominator_name));
        return;
    }

    std::string denom_name = denominator_name;
    auto denominator = table.find(denominator_name);

    // if the denominator_name is not in the table, check if there are any prefixes or power
    if (denominator == table.end()) {
        bool changed_denominator_name = true;

        while (changed_denominator_name) {
            changed_denominator_name = false;
            for (const auto& it: prefixes) {
                auto res = std::mismatch(it.first.begin(),
                                         it.first.end(),
                                         denominator_name.begin(),
                                         denominator_name.end());
                if (res.first == it.first.end()) {
                    changed_denominator_name = true;
                    denominator_prefix_factor *= it.second;
                    denominator_name.erase(denominator_name.begin(),
                                           denominator_name.begin() +
                                               static_cast<std::ptrdiff_t>(it.first.size()));
                }
            }
        }
        // if the denominator is only a prefix, just multiply the prefix factor with the unit factor
        if (denominator_name.empty()) {
            for (const auto& it: prefixes) {
                auto res = std::mismatch(it.first.begin(), it.first.end(), denom_name.begin());
                if (res.first == it.first.end()) {
                    unit->mul_factor(it.second);
                    return;
                }
            }
        }

        // if the denominator_back is a UNIT_POWER save the power to be able
        // to calculate the correct factor and dimensions later
        char denominator_back = denominator_name.back();
        if (denominator_back >= '2' && denominator_back <= '9') {
            denominator_power = denominator_back - '0';
            denominator_name.pop_back();
        }

        denominator = table.find(denominator_name);

        // delete "s" char for plural of the denominator_name
        if (denominator == table.end()) {
            if (denominator_name.back() == 's') {
                denominator_name.pop_back();
            }
            denominator = table.find(denominator_name);
        }
    }

    if (denominator == table.end()) {
        std::string ss = fmt::format("Unit {} not defined!", denominator_name);
        throw std::runtime_error(ss);
    } else {
        for (int i = 0; i < denominator_power; i++) {
            unit->mul_factor(1.0 / (denominator_prefix_factor * denominator->second->get_factor()));
            unit->add_denominator_dims(denominator->second->get_dimensions());
        }
    }
}

void UnitTable::insert(const std::shared_ptr<Unit>& unit) {
    // if the unit is already in the table throw error because
    // redefinition of a unit is not allowed
    if (table.find(unit->get_name()) != table.end()) {
        std::stringstream ss_unit_string;
        ss_unit_string << fmt::format("{:g} {}",
                                      unit->get_factor(),
                                      fmt::join(unit->get_nominator_unit(), ""));
        if (!unit->get_denominator_unit().empty()) {
            ss_unit_string << fmt::format("/{}", fmt::join(unit->get_denominator_unit(), ""));
        }
        throw std::runtime_error(fmt::format("Redefinition of units ({}) to {} is not allowed.",
                                             unit->get_name(),
                                             ss_unit_string.str()));
    }
    // check if the unit is a base unit and
    // then add it to the base units vector
    auto unit_nominator = unit->get_nominator_unit();
    auto only_base_unit_nominator =
        unit_nominator.size() == 1 && unit_nominator.front().size() == 3 &&
        (unit_nominator.front().front() == '*' && unit_nominator.front().back() == '*');
    if (only_base_unit_nominator) {
        // base_units_names[i] = "*i-th base unit*" (ex. base_units_names[0] = "*a*")
        auto const index = unit_nominator.front()[1] - 'a';
        assert(index >= 0 && index < base_units_names.size());
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        base_units_names[index] = unit->get_name();
        table.insert({unit->get_name(), unit});
        return;
    }
    // calculate unit's dimensions based on its nominator and denominator
    for (const auto& it: unit->get_nominator_unit()) {
        calc_nominator_dims(unit, it);
    }
    for (const auto& it: unit->get_denominator_unit()) {
        calc_denominator_dims(unit, it);
    }
    table.insert({unit->get_name(), unit});
}

void UnitTable::insert_prefix(const std::shared_ptr<Prefix>& prfx) {
    prefixes.insert({prfx->get_name(), prfx->get_factor()});
}

void UnitTable::print_units_sorted(std::ostream& units_details) const {
    std::vector<std::pair<std::string, std::shared_ptr<Unit>>> sorted_elements(table.begin(),
                                                                               table.end());
    std::sort(sorted_elements.begin(), sorted_elements.end());
    for (const auto& it: sorted_elements) {
        units_details << fmt::format("{} {:g}: {}\n",
                                     it.first,
                                     it.second->get_factor(),
                                     fmt::join(it.second->get_dimensions(), " "));
    }
}

void UnitTable::print_base_units(std::ostream& base_units_details) const {
    bool first_print = true;
    for (const auto& it: base_units_names) {
        if (!it.empty()) {
            if (first_print) {
                first_print = false;
                base_units_details << it;
            } else {
                base_units_details << ' ' << it;
            }
        }
    }
    base_units_details << '\n';
}

}  // namespace units
}  // namespace nmodl
