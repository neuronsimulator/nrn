/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <fstream>
#include <sstream>

#include "lexer/nmodl_lexer.hpp"
#include "parser/nmodl_driver.hpp"
#include "utils/logger.hpp"

namespace nmodl {
namespace parser {

NmodlDriver::NmodlDriver(bool strace, bool ptrace)
    : trace_scanner(strace)
    , trace_parser(ptrace) {}

/// parse nmodl file provided as istream
std::shared_ptr<ast::Program> NmodlDriver::parse_stream(std::istream& in) {
    NmodlLexer scanner(*this, &in);
    NmodlParser parser(scanner, *this);

    scanner.set_debug(trace_scanner);
    parser.set_debug_level(trace_parser);
    parser.parse();
    return astRoot;
}

std::shared_ptr<ast::Program> NmodlDriver::parse_file(const std::string& filename,
                                                      const location* loc) {
    std::ifstream in(filename.c_str());
    if (!in.good()) {
        std::ostringstream oss;
        if (loc == nullptr) {
            oss << "NMODL Parser Error : ";
        }
        oss << "can not open file : " << filename;
        if (loc != nullptr) {
            parse_error(*loc, oss.str());
        } else {
            throw std::runtime_error(oss.str());
        }
    }

    auto current_stream_name = stream_name;
    stream_name = filename;
    auto absolute_path = utils::cwd() + utils::pathsep + filename;
    {
        const auto last_slash = filename.find_last_of(utils::pathsep);
        if (utils::file_is_abs(filename)) {
            const auto path_prefix = filename.substr(0, last_slash + 1);
            library.push_current_directory(path_prefix);
            absolute_path = filename;
        } else if (last_slash == std::string::npos) {
            library.push_current_directory(utils::cwd());
        } else {
            const auto path_prefix = filename.substr(0, last_slash + 1);
            const auto path = utils::cwd() + utils::pathsep + path_prefix;
            library.push_current_directory(path);
        }
    }

    open_files.emplace(absolute_path, loc);
    parse_stream(in);
    open_files.erase(absolute_path);
    library.pop_current_directory();
    stream_name = current_stream_name;

    return astRoot;
}

std::shared_ptr<ast::Program> NmodlDriver::parse_string(const std::string& input) {
    std::istringstream iss(input);
    parse_stream(iss);
    return astRoot;
}

std::shared_ptr<ast::Include> NmodlDriver::parse_include(const std::string& name,
                                                         const location& loc) {
    if (name.empty()) {
        parse_error(loc, "empty filename");
    }

    // Try to find directory containing the file to import
    const auto directory_path = library.find_file(name);

    // Complete path of file (directory + filename).
    std::string absolute_path = name;

    if (!directory_path.empty()) {
        absolute_path = directory_path + std::string(1, utils::pathsep) + name;
    }

    // Detect recursive inclusion.
    auto already_included = open_files.find(absolute_path);
    if (already_included != open_files.end()) {
        std::ostringstream oss;
        oss << name << ": recursive inclusion.\n";
        if (already_included->second != nullptr) {
            oss << *already_included->second << ": initial inclusion was here.";
        }
        parse_error(loc, oss.str());
    }

    std::shared_ptr<ast::Program> program;
    program.swap(astRoot);

    parse_file(absolute_path, &loc);

    program.swap(astRoot);
    auto filename_node = std::shared_ptr<ast::String>(
        new ast::String(std::string(1, '"') + name + std::string(1, '"')));
    return std::shared_ptr<ast::Include>(new ast::Include(filename_node, program->get_blocks()));
}

void NmodlDriver::add_defined_var(const std::string& name, int value) {
    defined_var[name] = value;
}

bool NmodlDriver::is_defined_var(const std::string& name) const {
    return !(defined_var.find(name) == defined_var.end());
}

int NmodlDriver::get_defined_var_value(const std::string& name) const {
    const auto var_it = defined_var.find(name);
    if (var_it != defined_var.end()) {
        return var_it->second;
    }
    throw std::runtime_error("Trying to get undefined macro / define :" + name);
}

void NmodlDriver::parse_error(const location& location, const std::string& message) {
    std::ostringstream oss;
    oss << "NMODL Parser Error : " << message << " [Location : " << location << "]";
    throw std::runtime_error(oss.str());
}

void NmodlDriver::parse_error(const NmodlLexer& scanner,
                              const location& location,
                              const std::string& message) {
    std::ostringstream oss;
    oss << "NMODL Parser Error : " << message << " [Location : " << location << "]";
    oss << scanner.get_curr_line() << '\n';
    oss << std::string(location.begin.column - 1, '-');
    oss << "^\n";
    throw std::runtime_error(oss.str());
}

std::string NmodlDriver::check_include_argument(const location& location,
                                                const std::string& filename) {
    if (filename.empty()) {
        parse_error(location, "empty filename in INCLUDE directive");
    } else if (filename.front() != '"' && filename.back() != '"') {
        parse_error(location, "filename may start and end with \" character");
    } else if (filename.size() == 3) {
        parse_error(location, "filename is empty");
    }
    return filename.substr(1, filename.size() - 2);
}

}  // namespace parser
}  // namespace nmodl
