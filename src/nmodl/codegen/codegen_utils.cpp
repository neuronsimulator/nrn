/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "codegen/codegen_utils.hpp"

#include "codegen/codegen_c_visitor.hpp"
#include "codegen/codegen_ispc_visitor.hpp"

namespace nmodl {
namespace codegen {
namespace utils {
/**
 * \details We can directly print value but if user specify value as integer then
 * then it gets printed as an integer. To avoid this, we use below wrapper.
 * If user has provided integer then it gets printed as 1.0 (similar to mod2c
 * and neuron where ".0" is appended). Otherwise we print double variables as
 * they are represented in the mod file by user. If the value is in scientific
 * representation (1e+20, 1E-15) then keep it as it is.
 */
template <>
std::string format_double_string<CodegenCVisitor>(const std::string& s_value) {
    double value = std::stod(s_value);
    if (std::ceil(value) == value && s_value.find_first_of("eE") == std::string::npos) {
        return fmt::format("{:.1f}", value);
    }
    return s_value;
}


/**
 * \details In ISPC we have to explicitly append `d` to a floating point number
 * otherwise it is treated as float. A value stored in the AST can be in
 * scientific notation and hence we can't just append `d` to the string.
 * Hence, we have to transform the value into the ISPC compliant format by
 * replacing `e` and `E` with `d` to keep the same representation of the
 * number as in the cpp backend.
 */
template <>
std::string format_double_string<CodegenIspcVisitor>(const std::string& s_value) {
    std::string return_string = s_value;
    if (s_value.find_first_of("eE") != std::string::npos) {
        std::replace(return_string.begin(), return_string.end(), 'E', 'd');
        std::replace(return_string.begin(), return_string.end(), 'e', 'd');
    } else if (s_value.find('.') == std::string::npos) {
        return_string += ".0d";
    } else if (s_value.front() == '.') {
        return_string = '0' + return_string + 'd';
    } else {
        return_string += 'd';
    }
    return return_string;
}


template <>
std::string format_float_string<CodegenCVisitor>(const std::string& s_value) {
    float value = std::stof(s_value);
    if (std::ceil(value) == value && s_value.find_first_of("eE") == std::string::npos) {
        return fmt::format("{:.1f}", value);
    }
    return s_value;
}


/**
 * \details For float variables we don't have to do the conversion with changing `e` and
 * `E` with `f`, since the scientific notation numbers are already parsed as
 * floats by ISPC. Instead we need to take care of only appending `f` to the
 * end of floating point numbers, which is optional on ISPC.
 */
template <>
std::string format_float_string<CodegenIspcVisitor>(const std::string& s_value) {
    std::string return_string = s_value;
    if (s_value.find_first_of("Ee.") == std::string::npos) {
        return_string += ".0f";
    } else if (s_value.front() == '.' && s_value.find_first_of("Ee") == std::string::npos) {
        return_string = '0' + return_string + 'f';
    } else if (s_value.find_first_of("Ee") == std::string::npos) {
        return_string += 'f';
    }
    return return_string;
}


}  // namespace utils
}  // namespace codegen
}  // namespace nmodl
