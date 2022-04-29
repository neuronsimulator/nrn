/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <iostream>


namespace nmodl {
namespace parser {

/**
 * @addtogroup parser
 * @{
 */

/**
 * \class VerbatimDriver
 * \brief Class that binds lexer and parser together for parsing VERBATIM block
 */
class VerbatimDriver {
  protected:
    void init_scanner();
    void destroy_scanner();

  public:
    void* scanner = nullptr;
    std::istream* is = nullptr;
    std::string* result = nullptr;

    VerbatimDriver(std::istream* is = &std::cin) {
        init_scanner();
        this->is = is;
    }

    virtual ~VerbatimDriver() {
        destroy_scanner();
        if (result) {
            delete result;
        }
    }
};

/** @} */  // end of parser

}  // namespace parser
}  // namespace nmodl


int Verbatim_parse(nmodl::parser::VerbatimDriver*);
