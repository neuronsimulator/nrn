/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file
 * \brief Implement generic table data structure
 */

#include <sstream>
#include <vector>

#include "utils/string_utils.hpp"


namespace nmodl {
namespace utils {

/**
 * @addtogroup utils
 * @{
 */

/**
 * \class TableData
 * \brief Class to construct and pretty-print tabular data
 *
 * This class is used to construct and print tables (like
 * nmodl::symtab::SymbolTable and performance tables).
 */
struct TableData {
    using TableRowType = std::vector<std::string>;

    /// title of the table
    std::string title;

    /// top header/keys
    TableRowType headers;

    /// data
    std::vector<TableRowType> rows;

    /// alignment for every column of data rows
    std::vector<stringutils::text_alignment> alignments;

    void print(int indent = 0) const;

    void print(std::ostream& stream, int indent = 0) const;
};

/** @} */  // end of utils

}  // namespace utils
}  // namespace nmodl
