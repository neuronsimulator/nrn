/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "codegen/codegen_utils.hpp"

#include "codegen/codegen_cpp_visitor.hpp"

namespace nmodl {
namespace codegen {
namespace utils {
/**
 * \details We can directly print value but if user specify value as integer then
 * then it gets printed as an integer. To avoid this, we use below wrappers.
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


template <>
std::string format_float_string<CodegenCVisitor>(const std::string& s_value) {
    float value = std::stof(s_value);
    if (std::ceil(value) == value && s_value.find_first_of("eE") == std::string::npos) {
        return fmt::format("{:.1f}", value);
    }
    return s_value;
}
}  // namespace utils
}  // namespace codegen
}  // namespace nmodl
