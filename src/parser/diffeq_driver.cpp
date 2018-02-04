#include <sstream>

#include "lexer/diffeq_lexer.hpp"
#include "parser/diffeq_driver.hpp"
#include "utils/string_utils.hpp"

namespace diffeq {

    std::string Driver::solve(std::string equation, std::string method, bool debug) {
        /// split equation into lhs and rhs (lhs is a state variable)
        auto parts = stringutils::split_string(equation, '=');
        auto state = stringutils::trim(parts[0]);
        auto rhs = stringutils::trim(parts[1]);

        /// expect prime on lhs, find order and remove quote
        int order = std::count(state.begin(), state.end(), '\'');
        stringutils::remove_character(state, '\'');

        /// error if no prime in equation or not a binary expression
        if (order == 0 || state.size() == 0) {
            throw std::runtime_error("Invalid equation, no prime on rhs? " + equation);
        }

        return solve_equation(state, order, rhs, method, debug);
    }

    std::string Driver::solve_equation(std::string& state,
                                       int order,
                                       std::string& rhs,
                                       std::string& method,
                                       bool debug) {
        std::istringstream in(rhs);
        DiffEqContext context(state, order, rhs, method);
        Lexer scanner(&in);
        Parser parser(scanner, context);
        parser.parse();

        if (debug) {
            context.print();
        }
        return context.get_solution();
    }

}  // namespace diffeq
