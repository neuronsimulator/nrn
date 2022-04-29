/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::printer::NMODLPrinter
 */

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>

namespace nmodl {
namespace printer {

/**
 * @addtogroup printer
 * @{
 */

/**
 * \class NMODLPrinter
 * \brief Helper class for printing AST back to NMDOL test
 *
 * NmodlPrintVisitor transforms AST back to NMODL. This class provided common
 * functionality required by visitor to print nmodl ascii file.
 *
 * \todo Implement Printer as base class to avoid duplication code between
 *       JSONPrinter and NMODLPrinter.
 */
class NMODLPrinter {
  private:
    std::ofstream ofs;
    std::streambuf* sbuf = nullptr;
    std::shared_ptr<std::ostream> result;
    size_t indent_level = 0;

  public:
    NMODLPrinter()
        : result(new std::ostream(std::cout.rdbuf())) {}
    NMODLPrinter(std::ostream& stream)
        : result(new std::ostream(stream.rdbuf())) {}
    NMODLPrinter(const std::string& filename);

    ~NMODLPrinter() {
        ofs.close();
    }

    /// print whitespaces for indentation
    void add_indent();

    /// start of new block scope (i.e. start with "{")
    /// and increases indentation level
    void push_level();

    void add_element(const std::string&);
    void add_newline();

    /// end of current block scope (i.e. end with "}")
    /// and decreases indentation level
    void pop_level();
};

/** @} */  // end of printer

}  // namespace printer
}  // namespace nmodl
