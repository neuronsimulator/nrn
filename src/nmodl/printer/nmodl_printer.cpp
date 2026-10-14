/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "printer/nmodl_printer.hpp"
#include "utils/string_utils.hpp"

namespace nmodl {
namespace printer {

NMODLPrinter::NMODLPrinter(const std::string& filename) {
    if (filename.empty()) {
        throw std::runtime_error("Empty filename for NMODLPrinter");
    }

    ofs.open(filename.c_str());

    if (ofs.fail()) {
        auto msg = "Error while opening file '" + filename + "' for NMODLPrinter";
        throw std::runtime_error(msg);
    }

    sbuf = ofs.rdbuf();
    result = std::make_shared<std::ostream>(sbuf);
}

void NMODLPrinter::push_level() {
    indent_level++;
    *result << "{";
    add_newline();
}

void NMODLPrinter::add_indent() {
    *result << std::string(indent_level * 4, ' ');
}

void NMODLPrinter::add_element(const std::string& name) {
    *result << name << "";
}

void NMODLPrinter::add_newline() {
    *result << std::endl;
}

void NMODLPrinter::pop_level() {
    indent_level--;
    add_indent();
    *result << "}";
}

}  // namespace printer
}  // namespace nmodl
