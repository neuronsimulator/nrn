/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <filesystem>
#include <fstream>
#include <sstream>

#include "lexer/nmodl_lexer.hpp"
#include "parser/nmodl_driver.hpp"
#include "utils/logger.hpp"

namespace fs = std::filesystem;

namespace nmodl {
namespace parser {

NmodlDriver::NmodlDriver(bool strace, bool ptrace)
    : trace_scanner(strace)
    , trace_parser(ptrace) {}

/// parse nmodl file provided as istream
std::shared_ptr<ast::Program> NmodlDriver::parse_stream(std::istream& in) {
    NmodlLexer scanner(*this, &in);
    NmodlParser parser(scanner, *this);

    parser_stream << "parser trace:" << std::endl;

    scanner.set_debug(trace_scanner);
    parser.set_debug_level(trace_parser);
    parser.set_debug_stream(parser_stream);

    parser.parse();

    logger->trace(parser_stream.str());

    return astRoot;
}

std::shared_ptr<ast::Program> NmodlDriver::parse_file(const fs::path& filename,
                                                      const location* loc) {
    if (fs::is_directory(filename)) {
        throw std::runtime_error("NMODL Parser Error : path " + filename.string() +
                                 " appears to be a directory, please provide a file instead");
    }
    std::ifstream in(filename);
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
    stream_name = filename.string();
    auto absolute_path = fs::absolute(filename);
    {
        if (filename.is_absolute()) {
            const auto path_prefix = filename.parent_path();
            library.push_current_directory(path_prefix);
            absolute_path = filename.string();
        } else if (!filename.has_parent_path()) {
            library.push_current_directory(fs::current_path());
        } else {
            const auto path_prefix = filename.parent_path();
            const auto path = fs::absolute(path_prefix);
            library.push_current_directory(path);
        }
    }

    open_files.emplace(absolute_path.string(), loc);
    parse_stream(in);
    open_files.erase(absolute_path.string());
    library.pop_current_directory();
    stream_name = current_stream_name;

    return astRoot;
}

std::shared_ptr<ast::Program> NmodlDriver::parse_string(const std::string& input) {
    std::istringstream iss(input);
    parse_stream(iss);
    return astRoot;
}

std::shared_ptr<ast::Include> NmodlDriver::parse_include(const fs::path& name,
                                                         const location& loc) {
    if (name.empty()) {
        parse_error(loc, "empty filename");
    }

    // Try to find directory containing the file to import
    const auto directory_path = fs::path{library.find_file(name)};

    // Complete path of file (directory + filename).
    auto absolute_path = name;

    if (!directory_path.empty()) {
        absolute_path = directory_path / name;
    }

    // Detect recursive inclusion.
    auto already_included = open_files.find(absolute_path.string());
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
        new ast::String(fmt::format("\"{}\"", name.string())));
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
    // output the logs if running with `trace` verbosity
    logger->trace(parser_stream.str());
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
