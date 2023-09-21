/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
    std::unique_ptr<std::ostream> result;
    size_t indent_level = 0;
    const size_t NUM_SPACES = 4;

  public:
    CodePrinter()
        : result(std::make_unique<std::ostream>(std::cout.rdbuf())) {}

    CodePrinter(std::ostream& stream)
        : result(std::make_unique<std::ostream>(stream.rdbuf())) {}

    CodePrinter(const std::string& filename);

    ~CodePrinter() {
        ofs.close();
    }

    /// print whitespaces for indentation
    void add_indent();

    /// start a block scope without indentation (i.e. "{\n")
    void push_block();

    /// start a block scope with an expression (i.e. "[indent][expression] {\n")
    void push_block(const std::string& expression);

    /// end a block and immediately start a new one (i.e. "[indent-1]} [expression] {\n")
    void chain_block(std::string const& expression);

    template <typename... Args>
    void add_text(Args&&... args) {
        (operator<<(*result, args), ...);
    }

    template <typename... Args>
    void add_line(Args&&... args) {
        add_indent();
        add_text(std::forward<Args>(args)...);
        add_newline(1);
    }

    /// fmt_line(x, y, z) is just shorthand for add_line(fmt::format(x, y, z))
    template <typename... Args>
    void fmt_line(Args&&... args) {
        add_line(fmt::format(std::forward<Args>(args)...));
    }

    /// fmt_push_block(args...) is just shorthand for push_block(fmt::format(args...))
    template <typename... Args>
    void fmt_push_block(Args&&... args) {
        push_block(fmt::format(std::forward<Args>(args)...));
    }

    /// fmt_text(args...) is just shorthand for add_text(fmt::format(args...))
    template <typename... Args>
    void fmt_text(Args&&... args) {
        add_text(fmt::format(std::forward<Args>(args)...));
    }

    void add_multi_line(const std::string&);

    void add_newline(std::size_t n = 1);

    void increase_indent() {
        indent_level++;
    }

    void decrease_indent() {
        indent_level--;
    }

    /// end of current block scope (i.e. end with "}") and adds one NL character
    void pop_block();

    /// same as \a pop_block but control the number of NL characters (0 or more) with \a
    /// num_newlines parameter
    void pop_block_nl(std::size_t num_newlines = 0);

    /// end a block with `suffix` before the newline(s) (i.e. [indent]}[suffix]\n*num_newlines)
    void pop_block(const std::string_view& suffix, std::size_t num_newlines = 1);

    int indent_spaces() {
        return NUM_SPACES * indent_level;
    }
};

/** @} */  // end of printer

}  // namespace printer
}  // namespace nmodl
