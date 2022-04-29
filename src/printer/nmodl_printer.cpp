/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

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
