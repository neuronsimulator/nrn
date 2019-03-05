/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <sstream>
#include <vector>

#include "utils/string_utils.hpp"


namespace nmodl {

/**
 * \class TableData
 * \brief Class to construct and pretty-print tabular data
 *
 * This class is used to construct and print tables (like symbol
 * table and performance tables).
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
    std::vector<text_alignment> alignments;

    void print(int indent = 0);
    void print(std::stringstream& stream, int indent = 0);
};

}  // namespace nmodl