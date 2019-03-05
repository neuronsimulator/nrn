/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <iostream>


namespace nmodl {
namespace parser {

class VerbatimContext {
  public:
    void* scanner = nullptr;
    std::istream* is = nullptr;
    std::string* result = nullptr;

    VerbatimContext(std::istream* is = &std::cin) {
        init_scanner();
        this->is = is;
    }

    virtual ~VerbatimContext() {
        destroy_scanner();
        if (result) {
            delete result;
        }
    }

  protected:
    void init_scanner();
    void destroy_scanner();
};

}  // namespace parser
}  // namespace nmodl


int Verbatim_parse(nmodl::parser::VerbatimContext*);