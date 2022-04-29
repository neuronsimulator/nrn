/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/


#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "units.hpp"

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

void Unit::add_fraction(const std::string& fraction_string) {
    double nom, denom;
    std::string nominator;
    std::string denominator;
    std::string::const_iterator it;
    for (it = fraction_string.begin(); it != fraction_string.end() && *it != '|'; ++it) {
        nominator.push_back(*it);
    }
    // pass "|" char
    ++it;
    for (auto itm = it; itm != fraction_string.end(); ++itm) {
        denominator.push_back(*itm);
    }
    nom = parse_double(nominator);
    denom = parse_double(denominator);
    unit_factor = nom / denom;
}

double Unit::parse_double(std::string double_string) {
    long double d_number;
    double d_magnitude;
    std::string s_number;
    std::string s_magnitude;
    std::string::const_iterator it;
    // if double is positive sign = 1 else sign = -1
    // to be able to be multiplied by the d_number
    int sign = 1;
    if (double_string.front() == '-') {
        sign = -1;
        double_string.erase(double_string.begin());
    }
    // if *it reached an exponent related char, then the whole double number is read
    for (it = double_string.begin();
         it != double_string.end() && *it != 'e' && *it != '+' && *it != '-';
         ++it) {
        s_number.push_back(*it);
    }
    // then read the magnitude of the double number
    for (auto itm = it; itm != double_string.end(); ++itm) {
        if (*itm != 'e') {
            s_magnitude.push_back(*itm);
        }
    }
    d_number = std::stold(s_number);
    if (s_magnitude.empty()) {
        d_magnitude = 0.0;
    } else {
        d_magnitude = std::stod(s_magnitude);
    }
    return static_cast<double>(d_number * powl(10.0, d_magnitude) * sign);
}

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
                auto res = std::mismatch(it.first.begin(), it.first.end(), nominator_name.begin());
                if (res.first == it.first.end()) {
                    changed_nominator_name = 1;
                    nominator_prefix_factor *= it.second;
                    nominator_name.erase(nominator_name.begin(),
                                         nominator_name.begin() + it.first.size());
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
        std::stringstream ss;
        ss << "Unit " << nominator_name << " not defined!" << std::endl;
        throw std::runtime_error(ss.str());
    } else {
        for (int i = 0; i < nominator_power; i++) {
            unit->mul_factor(nominator_prefix_factor * nominator->second->get_factor());
            unit->add_nominator_dims(nominator->second->get_dimensions());
        }
    }
}

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
        int changed_denominator_name = 1;

        while (changed_denominator_name) {
            changed_denominator_name = 0;
            for (const auto& it: prefixes) {
                auto res =
                    std::mismatch(it.first.begin(), it.first.end(), denominator_name.begin());
                if (res.first == it.first.end()) {
                    changed_denominator_name = 1;
                    denominator_prefix_factor *= it.second;
                    denominator_name.erase(denominator_name.begin(),
                                           denominator_name.begin() + it.first.size());
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
        std::stringstream ss;
        ss << "Unit " << denominator_name << " not defined!" << std::endl;
        throw std::runtime_error(ss.str());
    } else {
        for (int i = 0; i < denominator_power; i++) {
            unit->mul_factor(1.0 / (denominator_prefix_factor * denominator->second->get_factor()));
            unit->add_denominator_dims(denominator->second->get_dimensions());
        }
    }
}

void UnitTable::insert(const std::shared_ptr<Unit>& unit) {
    // check if the unit is a base unit and
    // then add it to the base units vector
    auto unit_nominator = unit->get_nominator_unit();
    auto only_base_unit_nominator =
        unit_nominator.size() == 1 && unit_nominator.front().size() == 3 &&
        (unit_nominator.front().front() == '*' && unit_nominator.front().back() == '*');
    if (only_base_unit_nominator) {
        // base_units_names[i] = "*i-th base unit*" (ex. base_units_names[0] = "*a*")
        base_units_names[unit_nominator.front()[1] - 'a'] = unit->get_name();
        // if  unit is found in table replace it
        auto find_unit_name = table.find(unit->get_name());
        if (find_unit_name == table.end()) {
            table.insert({unit->get_name(), unit});
        } else {
            table.erase(unit->get_name());
            table.insert({unit->get_name(), unit});
        }
        return;
    }
    // calculate unit's dimensions based on its nominator and denominator
    for (const auto& it: unit->get_nominator_unit()) {
        calc_nominator_dims(unit, it);
    }
    for (const auto& it: unit->get_denominator_unit()) {
        calc_denominator_dims(unit, it);
    }
    // if unit is not in the table simply insert it, else replace with it with
    // new definition
    auto find_unit_name = table.find(unit->get_name());
    if (find_unit_name == table.end()) {
        table.insert({unit->get_name(), unit});
    } else {
        table.erase(unit->get_name());
        table.insert({unit->get_name(), unit});
    }
}

void UnitTable::insert_prefix(const std::shared_ptr<Prefix>& prfx) {
    prefixes.insert({prfx->get_name(), prfx->get_factor()});
}

void UnitTable::print_units() const {
    for (const auto& it: table) {
        std::cout << std::fixed << std::setprecision(8) << it.first << ' '
                  << it.second->get_factor() << ':';
        for (const auto& dims: it.second->get_dimensions()) {
            std::cout << ' ' << dims;
        }
        std::cout << '\n';
    }
}

void UnitTable::print_base_units() const {
    int first_print = 1;
    for (const auto& it: base_units_names) {
        if (!it.empty()) {
            if (first_print) {
                first_print = 0;
                std::cout << it;
            } else {
                std::cout << ' ' << it;
            }
        }
    }
    std::cout << '\n';
}

void UnitTable::print_units_sorted(std::ostream& units_details) const {
    std::vector<std::pair<std::string, std::shared_ptr<Unit>>> sorted_elements(table.begin(),
                                                                               table.end());
    std::sort(sorted_elements.begin(), sorted_elements.end());
    for (const auto& it: sorted_elements) {
        units_details << std::fixed << std::setprecision(8) << it.first << ' '
                      << it.second->get_factor() << ':';
        for (const auto& dims: it.second->get_dimensions()) {
            units_details << ' ' << dims;
        }
        units_details << '\n';
    }
}

void UnitTable::print_base_units(std::ostream& base_units_details) const {
    int first_print = 1;
    for (const auto& it: base_units_names) {
        if (!it.empty()) {
            if (first_print) {
                first_print = 0;
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
