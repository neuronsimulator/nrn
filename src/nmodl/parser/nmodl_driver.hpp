/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <map>
#include <string>

#include "ast/ast.hpp"

/** The nmodl namespace encapsulates everything related to nmodl parsing
 * which includes lexer, parser, driver, keywords, token mapping etc. */
namespace nmodl {
namespace parser {

/**
 * \class NmodlDriver
 * \brief Class that binds all pieces together for parsing nmodl file
 *
 * Driver class bind components required for lexing, parsing and ast
 * generation from nmodl file. We create an instance of lexer, parser
 * and provides different methods to parse from file, stream or string.
 * The scanner also gets reference to driver object for two purposes :
 * scanner store/query the macro definitions into/from driver class
 * and erros can be propogated back to driver (not implemented yet).
 * Parser class also gets a reference to driver class as a parameter.
 * Parsing actions generate ast and it's pointer is stored in driver
 * class.
 *
 * \todo lexer, parser and ast member variables are used inside lexer/
 * parser instances. The local instaces are created inside parse_stream
 * and hence the pointers are no longer valid except ast. Need better
 * way to handle this.
 *
 * \todo stream name is not used as it will need better support as
 * location object used in scanner takes string pointer which could
 * be invalid when we copy location object.
 */

/// flex generated scanner class (extends base lexer class of flex)
class NmodlLexer;

/// parser class generated by bison
class NmodlParser;

class NmodlDriver {
  private:
    /// all macro defined in the mod file
    std::map<std::string, int> defined_var;

    /// enable debug output in the flex scanner
    bool trace_scanner = false;

    /// enable debug output in the bison parser
    bool trace_parser = false;

    /// pointer to the lexer instance being used
    NmodlLexer* lexer = nullptr;

    /// pointer to the parser instance being used
    NmodlParser* parser = nullptr;

    /// print messages from lexer/parser
    bool verbose = false;

  public:
    /// file or input stream name (used by scanner for position), see todo
    std::string streamname;

    /// root of the ast
    std::shared_ptr<ast::Program> astRoot = nullptr;

    NmodlDriver() = default;
    NmodlDriver(bool strace, bool ptrace);

    void error(const std::string& m, const class location& l);
    void error(const std::string& m);

    void add_defined_var(const std::string& name, int value);
    bool is_defined_var(const std::string& name);
    int get_defined_var_value(const std::string& name);

    bool parse_stream(std::istream& in);

    bool parse_string(const std::string& input);
    bool parse_file(const std::string& filename);

    void set_verbose(bool b) {
        verbose = b;
    }

    bool is_verbose() const {
        return verbose;
    }

    std::shared_ptr<ast::Program> ast() const {
        return astRoot;
    }
};

}  // namespace parser
}  // namespace nmodl
