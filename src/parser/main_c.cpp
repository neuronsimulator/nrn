/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <fstream>
#include <iostream>

#include "parser/c11_driver.hpp"
#include "tclap/CmdLine.h"

/**
 * Standlone parser program for C. This demonstrate basic
 * usage of psrser and driver class.
 */

int main(int argc, const char* argv[]) {
    try {
        TCLAP::CmdLine cmd("C Parser: Standalone parser program for C");
        TCLAP::ValueArg<std::string> filearg("", "file", "C input file path", false, "", "string");

        cmd.add(filearg);
        cmd.parse(argc, argv);

        std::string filename = filearg.getValue();

        if (filename.empty()) {
            std::cerr << "Error : Pass input C file, see --help" << std::endl;
            return 1;
        }

        std::ifstream file(filename);

        if (!file) {
            throw std::runtime_error("Could not open file " + filename);
        }

        std::cout << "\n C Parser : Processing file : " << filename << std::endl;

        std::istream& in(file);

        /// driver object creates lexer and parser, just call parser method
        nmodl::parser::CDriver driver;

        driver.set_verbose(true);
        driver.parse_stream(in);

        // driver.parse_file(filename);

        std::cout << "----PARSING FINISHED----" << std::endl;
    } catch (TCLAP::ArgException& e) {
        std::cout << "Argument Error: " << e.error() << " for arg " << e.argId() << std::endl;
        return 1;
    }

    return 0;
}
