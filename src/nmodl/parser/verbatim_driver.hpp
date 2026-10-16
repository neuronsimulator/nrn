/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
