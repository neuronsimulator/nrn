/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <iostream>
#include <numeric>

#include "utils/string_utils.hpp"
#include "utils/table_data.hpp"

namespace nmodl {
namespace utils {

/**
 *   Print table data in below shown format: title as first row (centrally aligned),
 *   second row is header for individual column (centrally aligned) and then all data
 *   rows (with associated alignments).
 *
 *   ----------------------------------------------------------------------------------------
 *   |       mBetaf [FunctionBlock IN NMODL_GLOBAL] POSITION : 109.1-8 SCOPE : LOCAL        |
 *   ----------------------------------------------------------------------------------------
 *   |    NAME     |    PROPERTIES     |    LOCATION     |    # READS     |    # WRITES     |
 *   ----------------------------------------------------------------------------------------
 *   | v           | argument          |          109.17 |              0 |               0 |
 *   ----------------------------------------------------------------------------------------
 */

void TableData::print(std::ostream& stream, int indent) const {
    const int PADDING = 1;

    /// not necessary to print empty table
    if (rows.empty() || headers.empty()) {
        return;
    }

    /// based on indentation level, spaces to prefix
    auto gutter = std::string(indent * 4, ' ');

    auto ncolumns = headers.size();
    std::vector<unsigned> col_width(ncolumns);

    /// alignment is optional, so fill remaining with right alignment
    auto all_alignments = alignments;
    all_alignments.reserve(ncolumns);
    for (unsigned i = alignments.size(); i < ncolumns; i++) {
        all_alignments.push_back(stringutils::text_alignment::center);
    }

    /// calculate space required for each column
    unsigned row_width = 0;
    for (unsigned i = 0; i < headers.size(); i++) {
        col_width[i] = headers[i].length() + PADDING;
        row_width += col_width[i];
    }

    /// if title is larger than headers then every column
    /// width needs to be scaled
    if (title.length() > row_width) {
        int extra_size = title.length() - row_width;
        int column_pad = extra_size / ncolumns;
        if ((extra_size % ncolumns) != 0) {
            column_pad++;
        }
        for (auto& column: col_width) {
            column += column_pad;
        }
    }

    /// check length of columns in each row to find max length required
    for (const auto& row: rows) {
        for (unsigned i = 0; i < row.size(); i++) {
            if (col_width[i] < (row[i].length()) + PADDING) {
                col_width[i] = row[i].length() + PADDING;
            }
        }
    }

    std::stringstream header;
    header << "| ";
    for (size_t i = 0; i < headers.size(); i++) {
        auto text =
            stringutils::align_text(headers[i], col_width[i], stringutils::text_alignment::center);
        header << text << " | ";
    }

    row_width = header.str().length();
    auto separator_line = std::string(row_width - 1, '-');

    /// title row
    if (!title.empty()) {
        auto fmt_title =
            stringutils::align_text(title, row_width - 3, stringutils::text_alignment::center);
        stream << '\n' << gutter << separator_line;
        stream << '\n' << gutter << '|' << fmt_title << '|';
    }

    /// header row
    stream << '\n' << gutter << separator_line;
    stream << '\n' << gutter << header.str();
    stream << '\n' << gutter << separator_line;

    /// data rows
    for (const auto& row: rows) {
        stream << '\n' << gutter << "| ";
        for (unsigned i = 0; i < row.size(); i++) {
            stream << stringutils::align_text(row[i], col_width[i], all_alignments[i]) << " | ";
        }
    }

    /// bottom separator line
    stream << '\n' << gutter << separator_line << '\n';
}

void TableData::print(int indent) const {
    print(std::cout, indent);
}

}  // namespace utils
}  // namespace nmodl
