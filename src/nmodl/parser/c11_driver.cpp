#include <fstream>
#include <sstream>

#include "lexer/c11_lexer.hpp"
#include "parser/c11_driver.hpp"

namespace c11 {

    Driver::Driver(bool strace, bool ptrace) : trace_scanner(strace), trace_parser(ptrace) {
    }

    /// parse c file provided as istream
    bool Driver::parse_stream(std::istream& in) {
        Lexer scanner(*this, &in);
        Parser parser(scanner, *this);

        this->lexer = &scanner;
        this->parser = &parser;

        scanner.set_debug(trace_scanner);
        parser.set_debug_level(trace_parser);
        return (parser.parse() == 0);
    }

    //// parse c file
    bool Driver::parse_file(const std::string& filename) {
        std::ifstream in(filename.c_str());
        streamname = filename;

        if (!in.good()) {
            return false;
        }
        return parse_stream(in);
    }

    /// parser c provided as string (used for testing)
    bool Driver::parse_string(const std::string& input) {
        std::istringstream iss(input);
        return parse_stream(iss);
    }

    void Driver::error(const std::string& m, const class location& l) {
        std::cerr << l << " : " << m << std::endl;
    }

    void Driver::error(const std::string& m) {
        std::cerr << m << std::endl;
    }

    void Driver::process(std::string text) {
        tokens.push_back(text);
        // here we will query and look into symbol table or register callback
        // std::cout << text;
    }

    void Driver::scan_string(std::string& text) {
        std::istringstream in(text);
        Lexer scanner(*this, &in);
        Parser parser(scanner, *this);
        this->lexer = &scanner;
        this->parser = &parser;
        while (true) {
            auto sym = lexer->next_token();
            auto token = sym.token();
            if (token == Parser::token::END) {
                break;
            }
        }
    }

}  // namespace c11
