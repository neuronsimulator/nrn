/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \dir
 * \brief Code printer implementations
 *
 * \file
 * \brief \copybrief nmodl::printer::CodePrinter
 */
#include <fmt/format.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string_view>

namespace nmodl {
/// implementation of various printers
namespace printer {

/**
 * @defgroup printer Code Printers
 * @brief Printers for translating AST to different forms
 * @{
 */

/**
 * \class CodePrinter
 * \brief Helper class for printing C/C++ code
 *
 * This class provides common functionality required by code
 * generation visitor to print C/C++/Cuda code.
 */
class CodePrinter {
  private:
    std::ofstream ofs;
    std::streambuf* sbuf = nullptr;
    std::shared_ptr<std::ostream> result;
    size_t indent_level = 0;
    const int NUM_SPACES = 4;

  public:
    CodePrinter()
        : result(std::make_shared<std::ostream>(std::cout.rdbuf())) {}

    CodePrinter(std::ostream& stream)
        : result(std::make_shared<std::ostream>(stream.rdbuf())) {}

    CodePrinter(const std::string& filename);

    ~CodePrinter() {
        ofs.close();
    }

    /// print whitespaces for indentation
    void add_indent();

    /// start a block scope without indentation (i.e. "{\n")
    void start_block();

    /// start a block scope with an expression (i.e. "[indent][expression] {\n")
    void start_block(std::string&& expression);

    /// end a block and immediately start a new one (i.e. "[indent-1]} [expression] {\n")
    void restart_block(std::string const& expression);

    void add_text(const std::string&);

    void add_line(const std::string&, int num_new_lines = 1);

    /// fmt_line(x, y, z) is just shorthand for add_line(fmt::format(x, y, z))
    template <typename... Args>
    void fmt_line(Args&&... args) {
        add_line(fmt::format(std::forward<Args>(args)...));
    }

    /// fmt_start_block(args...) is just shorthand for start_block(fmt::format(args...))
    template <typename... Args>
    void fmt_start_block(Args&&... args) {
        start_block(fmt::format(std::forward<Args>(args)...));
    }

    /// fmt_text(args...) is just shorthand for add_text(fmt::format(args...))
    template <typename... Args>
    void fmt_text(Args&&... args) {
        add_text(fmt::format(std::forward<Args>(args)...));
    }

    void add_multi_line(const std::string&);

    void add_newline(int n = 1);

    void increase_indent() {
        indent_level++;
    }

    void decrease_indent() {
        indent_level--;
    }

    /// end of current block scope (i.e. end with "}")
    void end_block(int num_newlines = 0);

    /// end a block with `suffix` before the newline(s) (i.e. [indent]}[suffix]\n*num_newlines)
    void end_block(std::string_view suffix, std::size_t num_newlines = 1);

    int indent_spaces() {
        return NUM_SPACES * indent_level;
    }
};

/** @} */  // end of printer

}  // namespace printer
}  // namespace nmodl
