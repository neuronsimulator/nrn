/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \dir
 * \brief Parser implementations
 */

#include <string>
#include <unordered_map>

#include "ast/ast.hpp"
#include "lexer/nmodl_lexer.hpp"
#include "parser/nmodl_driver.hpp"
#include "utils/file_library.hpp"


/// encapsulates everything related to NMODL code generation framework
namespace nmodl {
/// encapsulate lexer and parsers implementations
namespace parser {

/**
 * \defgroup parser Parser Implementation
 * \brief All parser and driver classes implementation
 *
 *
 *
 * \addtogroup parser
 * \{
 */

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
 * \todo Lexer, parser and ast member variables are used inside lexer/
 * parser instances. The local instaces are created inside parse_stream
 * and hence the pointers are no longer valid except ast. Need better
 * way to handle this.
 *
 * \todo Stream name is not used as it will need better support as
 * location object used in scanner takes string pointer which could
 * be invalid when we copy location object.
 */
class NmodlDriver {
  private:
    /// all macro defined in the mod file
    std::unordered_map<std::string, int> defined_var;

    /// enable debug output in the flex scanner
    bool trace_scanner = false;

    /// enable debug output in the bison parser
    bool trace_parser = false;

    /// print messages from lexer/parser
    bool verbose = false;

    /// root of the ast
    std::shared_ptr<ast::Program> astRoot = nullptr;

    /// The file library for IMPORT directives
    FileLibrary library{FileLibrary::default_instance()};

    /// The list of open files, and the location of the request.
    /// \a nullptr is pushed as location for the top NMODL file
    std::unordered_map<std::string, const location*> open_files;

  public:
    /// file or input stream name (used by scanner for position), see todo
    std::string stream_name;

    NmodlDriver() = default;
    NmodlDriver(bool strace, bool ptrace);

    /// add macro definition and it's value (DEFINE keyword of nmodl)
    void add_defined_var(const std::string& name, int value);

    /// check if particular text is defined as macro
    bool is_defined_var(const std::string& name) const;

    /// return variable's value defined as macro (always an integer)
    int get_defined_var_value(const std::string& name) const;

    std::shared_ptr<ast::Program> parse_stream(std::istream& in);

    /// parser nmodl provided as string (used for testing)
    std::shared_ptr<ast::Program> parse_string(const std::string& input);

    /**
     * \brief parse NMODL file
     * \param filename path to the file to parse
     * \param loc optional location when \a filename is dictated
     * by an `INCLUDE` NMODL directive.
     */
    std::shared_ptr<ast::Program> parse_file(const std::string& filename,
                                             const location* loc = nullptr);
    //// parse file specified in nmodl include directive
    std::shared_ptr<ast::Include> parse_include(const std::string& filename, const location& loc);

    void set_verbose(bool b) {
        verbose = b;
    }

    bool is_verbose() const noexcept {
        return verbose;
    }

    /// return previously parsed AST otherwise nullptr
    const std::shared_ptr<ast::Program>& get_ast() const noexcept {
        return astRoot;
    }

    /// set new ast root
    void set_ast(ast::Program* node) noexcept {
        astRoot.reset(node);
    }

    /**
     * Emit a parsing error
     * \throw std::runtime_error
     */
    static void parse_error(const location& location, const std::string& message);

    /**
     * Emit a parsing error. Takes additionally a Lexer instance to print code context
     * \throw std::runtime_error
     */
    static void parse_error(const NmodlLexer& scanner,
                            const location& location,
                            const std::string& message);

    /**
     * Ensure \a file argument given to the INCLUDE directive is valid:
     *    - between double-quotes
     *    - not empty ""
     *
     * \return unquoted string
     */
    static std::string check_include_argument(const location& location,
                                              const std::string& filename);
};

/** \} */  // end of parser

}  // namespace parser
}  // namespace nmodl
