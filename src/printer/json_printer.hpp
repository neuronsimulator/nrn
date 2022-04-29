/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::printer::JSONPrinter
 */

#include <fstream>
#include <iostream>
#include <stack>

#include "json/json.hpp"


namespace nmodl {
namespace printer {

using json = nlohmann::json;

/**
 * @addtogroup printer
 * @{
 */

/**
 * \class JSONPrinter
 * \brief Helper class for printing AST in JSON form
 *
 * We need to print AST in human readable format for debugging or visualization
 * of in memory structure.  This printer class provides simple interface to
 * construct JSON object from AST like data structures.  We use nlohmann's json
 * library which considerably simplify implementation.
 *
 * \todo We need to explicitly call `flush()` in order to get write/return
 *       results. We simply can't dump block in `popBlock()` because block itself will
 *       be part of other parent elements. Also we are writing results to file,
 *       `stringstream` and `cout`. And hence we can't simply reset/clear previously
 *       written text.
 */
class JSONPrinter {
  private:
    std::ofstream ofs;
    std::streambuf* sbuf = nullptr;

    /// common output stream for file, cout or stringstream
    std::shared_ptr<std::ostream> result;

    /// single (current) nmodl block / statement
    std::shared_ptr<json> block;

    /// stack that holds all parent blocks / statements
    std::stack<std::shared_ptr<json>> stack;

    /// true if need to print json in compact format
    bool compact = false;

    /// true if we need to expand keys i.e. add separate key/value
    /// pair for node name and it's children
    bool expand = false;

    /// json key for children
    const std::string child_key = "children";

  public:
    explicit JSONPrinter(const std::string& filename);

    /// By default dump output to std::cout
    JSONPrinter()
        : result(new std::ostream(std::cout.rdbuf())) {}

    // Dump output to stringstream
    explicit JSONPrinter(std::ostream& os)
        : result(new std::ostream(os.rdbuf())) {}

    ~JSONPrinter() {
        flush();
    }

    void push_block(const std::string& value, const std::string& key = "name");
    void add_node(std::string value, const std::string& key = "name");
    void add_block_property(std::string name, const std::string& value);
    void pop_block();
    void flush();

    /// print json in compact mode
    void compact_json(bool flag) {
        compact = flag;
    }

    void expand_keys(bool flag) {
        expand = flag;
    }
};

/** @} */  // end of printer

}  // namespace printer
}  // namespace nmodl
