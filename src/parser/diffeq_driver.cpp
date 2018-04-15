#include <sstream>
#include <utility>

#include "lexer/diffeq_lexer.hpp"
#include "parser/diffeq_driver.hpp"
#include "utils/string_utils.hpp"

namespace diffeq {

    void Driver::parse_equation(const std::string& equation,
                                std::string& state,
                                std::string& rhs,
                                int& order) {
        auto parts = stringutils::split_string(equation, '=');
        state = stringutils::trim(parts[0]);
        rhs = stringutils::trim(parts[1]);

        /// expect prime on lhs, find order and remove quote
        order = std::count(state.begin(), state.end(), '\'');
        stringutils::remove_character(state, '\'');

        /// error if no prime in equation or not an assignment statement
        if (order == 0 || state.empty()) {
            throw std::runtime_error("Invalid equation, no prime on rhs? " + equation);
        }
    }

    std::string Driver::solve(const std::string& equation, std::string method, bool debug) {
        std::string state, rhs;
        int order = 0;
        bool cnexp_possible;
        parse_equation(equation, state, rhs, order);
        return solve_equation(state, order, rhs, method, cnexp_possible, debug);
    }

    std::string Driver::solve_equation(std::string& state,
                                       int order,
                                       std::string& rhs,
                                       std::string& method,
                                       bool& cnexp_possible,
                                       bool debug) {
        std::istringstream in(rhs);
        DiffEqContext context(state, order, rhs, method);
        Lexer scanner(&in);
        Parser parser(scanner, context);
        parser.parse();
        if (debug) {
            context.print();
        }
        return context.get_solution(cnexp_possible);
    }

    /// \todo : instead of using neuron like api, we need to refactor
    bool Driver::cnexp_possible(const std::string& equation, std::string& solution) {
        std::string state, rhs;
        int order = 0;
        bool cnexp_possible;
        std::string method = "cnexp";
        parse_equation(equation, state, rhs, order);
        solution = solve_equation(state, order, rhs, method, cnexp_possible);
        return cnexp_possible;
    }

}  // namespace diffeq
