/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file
 * \brief Implement utility functions for codegen visitors
 *
 */

#include <string>

namespace nmodl {
namespace codegen {
namespace utils {

/**
 * Handles the double constants format being printed in the generated code.
 *
 * It takes care of printing the values with the correct floating point precision
 * for each backend, similar to mod2c and Neuron.
 * This function can be called using as template `CodegenCVisitor` or
 * `CodegenIspcVisitor`.
 *
 * \param s_value The double constant as string
 * \return        The proper string to be printed in the generated file.
 */
template <typename T>
std::string format_double_string(const std::string& s_value);


/**
 * Handles the float constants format being printed in the generated code.
 *
 * It takes care of printing the values with the correct floating point precision
 * for each backend, similar to mod2c and Neuron.
 * This function can be called using as template `CodegenCVisitor` or
 * `CodegenIspcVisitor`.
 *
 * \param s_value The double constant as string
 * \return        The proper string to be printed in the generated file.
 */
template <typename T>
std::string format_float_string(const std::string& s_value);

}  // namespace utils
}  // namespace codegen
}  // namespace nmodl
