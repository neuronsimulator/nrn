#ifndef _JSON_PRINTER_HPP_
#define _JSON_PRINTER_HPP_

#include <fstream>
#include <iostream>
#include <stack>

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
 * we can't simply reset/clear previously written text.
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

  public:
    JSONPrinter(const std::string& filename);

    /// By default dump output to std::cout
    JSONPrinter() : result(new std::ostream(std::cout.rdbuf())) {
    }

    // Dump output to stringstream
    JSONPrinter(std::ostream& os) : result(new std::ostream(os.rdbuf())) {
    }

    ~JSONPrinter() {
        flush();
    }

    void push_block(const std::string& name);
    void add_node(std::string value, const std::string& name = "value");
    void pop_block();
    void flush();

    /// print json in compact mode
    void compact_json(bool flag) {
        compact = flag;
    }
};

#endif
