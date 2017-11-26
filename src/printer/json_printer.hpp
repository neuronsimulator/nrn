#ifndef _JSON_PRINTER_HPP_
#define _JSON_PRINTER_HPP_

#include <stack>
#include <fstream>
#include <iostream>

#include "json/json.hpp"

using json = nlohmann::json;

/**
 * \class JSONPrinter
 * \brief Helper class for printing AST in JSON format
 *
 * We need to print AST in human readable format for
 * debugging or visualization of in memory structure.
 * This printer class provides simple interface to
 * construct JSON object from AST like data structures.
 * We use nlohmann's json library which considerably
 * simplify implementation.
 *
 * \todo : We need to explicitly call flush() in order
 * to get write/return results. We simply can't dump
 * block in popBlock() because block itself will be
 * part of other parent elements. Also we are writing
 * results to file, stringstream and cout. And hence
 * we can't simply reset/cler previously written text.
 */

class JSONPrinter {
  private:
    std::ofstream ofs;
    std::streambuf* sbuf = nullptr;

    /// common output stream for file, cout or stringstream
    std::shared_ptr<std::ostream> result_stream;

    /// single (current) nmodl block / statement
    std::shared_ptr<json> block;

    /// stack that holds all parent blocks / statements
    std::stack<std::shared_ptr<json>> stack;

  public:
    JSONPrinter();
    JSONPrinter(std::string filename);
    JSONPrinter(std::stringstream& ss);

    ~JSONPrinter() {
        flush();
    }

    void pushBlock(std::string);
    void addNode(std::string);
    void popBlock();
    void flush(int nspaces = 4);
};

#endif
