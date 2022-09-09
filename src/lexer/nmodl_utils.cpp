/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <cstring>
#include <iostream>

#include "ast/ast.hpp"
#include "lexer/modtoken.hpp"
#include "lexer/nmodl_utils.hpp"
#include "lexer/token_mapping.hpp"
#include "utils/string_utils.hpp"

namespace nmodl {

using Parser = parser::NmodlParser;

/**
 * Create a symbol for ast::Double AST class
 *
 * @param value Double value parsed by lexer
 * @param pos Position of value in the mod file
 * @return Symbol for double value
 */
SymbolType double_symbol(const std::string& value, PositionType& pos) {
    ModToken token(value, Token::REAL, pos);
    ast::Double float_value(value);
    float_value.set_token(token);
    return Parser::make_REAL(float_value, pos);
}


/**
 * Create a symbol for ast::Integer AST
 *
 * ast::Integer class also represent macro definition and
 * hence could have associated text.
 *
 * @param value Integer value parsed by lexer
 * @param pos Position of value in the mod file
 * @param text associated macro for the value
 * @return Symbol for integer value
 */
SymbolType integer_symbol(int value, PositionType& pos, const char* text) {
    ast::Name* macro = nullptr;
    ModToken token(std::to_string(value), Token::INTEGER, pos);

    if (text != nullptr) {
        macro = new ast::Name(new ast::String(text));
        macro->set_token(token);
    }

    ast::Integer int_value(value, macro);
    int_value.set_token(token);
    return Parser::make_INTEGER(int_value, pos);
}


/**
 * Create symbol for ast::Name AST class
 *
 * @param text Text value parsed by lexer
 * @param pos Position of value in the mod file
 * @param type Token type returned by lexer (see parser::NmodlParser::token::yytokentype)
 * @return Symbol for name value
 *
 * \todo
 *      In addition to keywords and methods, there are also external
 *      definitions for math and neuron specific functions/variables.
 *      In the token we should mark those as external.
 */
SymbolType name_symbol(const std::string& text, PositionType& pos, TokenType type) {
    ModToken token(text, type, pos);
    ast::Name value(new ast::String(text));
    value.set_token(token);
    return Parser::make_NAME(value, pos);
}


/**
 * Create symbol for ast::Prime AST class
 *
 * ast::Prime has name as well as order. Text returned by lexer has single
 * quote (`'`) as an order. We count the order and remove quote from the text.
 *
 * @param text Prime variable name parsed by lexer
 * @param pos Position of text in the mod file
 * @return Symbol for prime variable
 */
SymbolType prime_symbol(std::string text, PositionType& pos) {
    ModToken token(text, Token::PRIME, pos);
    auto order = std::count(text.begin(), text.end(), '\'');
    stringutils::remove_character(text, '\'');

    auto prime_name = new ast::String(text);
    assert(order <= std::numeric_limits<int>::max());
    auto prime_order = new ast::Integer(static_cast<int>(order), nullptr);
    ast::PrimeName value(prime_name, prime_order);
    value.set_token(token);
    return Parser::make_PRIME(value, pos);
}


/**
 * Create symbol for ast::String AST class
 * @param text Text value parsed by lexer
 * @param pos Position of value in the mod file
 * @return Symbol for string value
 */
SymbolType string_symbol(const std::string& text, PositionType& pos) {
    ModToken token(text, Token::STRING, pos);
    ast::String value(text);
    value.set_token(token);
    return Parser::make_STRING(value, pos);
}


/**
 * Create symbol for AST class
 *
 * These tokens doesn't have specific value to pass to the parser. They are more
 * part of grammar.  Depending on the type of toke, we create appropriate
 * symbol. From lexer we pass token type (which is optional). This is required
 * for reaction parsing where we have "lexical context". Hence, if token type is
 * passed then we don't check/search for the token type.
 *
 * @param key Text parsed by lexer
 * @param pos Position of value in the mod file
 * @param type Token type returned by lexer (see parser::NmodlParser::token::yytokentype)
 * @return Symbol for given token
 */
SymbolType token_symbol(const std::string& key, PositionType& pos, TokenType type) {
    /// if token type is not passed, check if it is keyword or method
    if (type == Token::UNKNOWN) {
        if (is_keyword(key) || is_method(key)) {
            type = token_type(key);
        }
    }

    ModToken token(key, type, pos);

    switch (static_cast<int>(type)) {
    case Token::AFTER:
        return Parser::make_AFTER(token, pos);
    case Token::BBCOREPOINTER:
        return Parser::make_BBCOREPOINTER(token, pos);
    case Token::BEFORE:
        return Parser::make_BEFORE(token, pos);
    case Token::BREAKPOINT:
        return Parser::make_BREAKPOINT(token, pos);
    case Token::BY:
        return Parser::make_BY(token, pos);
    case Token::COMPARTMENT:
        return Parser::make_COMPARTMENT(token, pos);
    case Token::CONDUCTANCE:
        return Parser::make_CONDUCTANCE(token, pos);
    case Token::CONSERVE:
        return Parser::make_CONSERVE(token, pos);
    case Token::CONSTANT:
        return Parser::make_CONSTANT(token, pos);
    case Token::CONSTRUCTOR:
        return Parser::make_CONSTRUCTOR(token, pos);
    case Token::DEFINE1:
        return Parser::make_DEFINE1(token, pos);
    case Token::DEPEND:
        return Parser::make_DEPEND(token, pos);
    case Token::ASSIGNED:
        return Parser::make_ASSIGNED(token, pos);
    case Token::DERIVATIVE:
        return Parser::make_DERIVATIVE(token, pos);
    case Token::DESTRUCTOR:
        return Parser::make_DESTRUCTOR(token, pos);
    case Token::DISCRETE:
        return Parser::make_DISCRETE(token, pos);
    case Token::ELECTRODE_CURRENT:
        return Parser::make_ELECTRODE_CURRENT(token, pos);
    case Token::ELSE:
        return Parser::make_ELSE(token, pos);
    case Token::EQUATION:
        return Parser::make_EQUATION(token, pos);
    case Token::EXTERNAL:
        return Parser::make_EXTERNAL(token, pos);
    case Token::FIRST:
        return Parser::make_FIRST(token, pos);
    case Token::FOR_NETCONS:
        return Parser::make_FOR_NETCONS(token, pos);
    case Token::FORALL1:
        return Parser::make_FORALL1(token, pos);
    case Token::FROM:
        return Parser::make_FROM(token, pos);
    case Token::FUNCTION1:
        return Parser::make_FUNCTION1(token, pos);
    case Token::FUNCTION_TABLE:
        return Parser::make_FUNCTION_TABLE(token, pos);
    case Token::GLOBAL:
        return Parser::make_GLOBAL(token, pos);
    case Token::IF:
        return Parser::make_IF(token, pos);
    case Token::IFERROR:
        return Parser::make_IFERROR(token, pos);
    case Token::INCLUDE1:
        return Parser::make_INCLUDE1(token, pos);
    case Token::INDEPENDENT:
        return Parser::make_INDEPENDENT(token, pos);
    case Token::INITIAL1:
        return Parser::make_INITIAL1(token, pos);
    case Token::KINETIC:
        return Parser::make_KINETIC(token, pos);
    case Token::LAG:
        return Parser::make_LAG(token, pos);
    case Token::LAST:
        return Parser::make_LAST(token, pos);
    case Token::LINEAR:
        return Parser::make_LINEAR(token, pos);
    case Token::LOCAL:
        return Parser::make_LOCAL(token, pos);
    case Token::LONGDIFUS:
        return Parser::make_LONGDIFUS(token, pos);
    case Token::MATCH:
        return Parser::make_MATCH(token, pos);
    case Token::MODEL:
        return Parser::make_MODEL(token, pos);
    case Token::MODEL_LEVEL:
        return Parser::make_MODEL_LEVEL(token, pos);
    case Token::NETRECEIVE:
        return Parser::make_NETRECEIVE(token, pos);
    case Token::NEURON:
        return Parser::make_NEURON(token, pos);
    case Token::NONLINEAR:
        return Parser::make_NONLINEAR(token, pos);
    case Token::NONSPECIFIC:
        return Parser::make_NONSPECIFIC(token, pos);
    case Token::NRNMUTEXLOCK:
        return Parser::make_NRNMUTEXLOCK(token, pos);
    case Token::NRNMUTEXUNLOCK:
        return Parser::make_NRNMUTEXUNLOCK(token, pos);
    case Token::PARAMETER:
        return Parser::make_PARAMETER(token, pos);
    case Token::PARTIAL:
        return Parser::make_PARTIAL(token, pos);
    case Token::PLOT:
        return Parser::make_PLOT(token, pos);
    case Token::POINTER:
        return Parser::make_POINTER(token, pos);
    case Token::PROCEDURE:
        return Parser::make_PROCEDURE(token, pos);
    case Token::PROTECT:
        return Parser::make_PROTECT(token, pos);
    case Token::RANGE:
        return Parser::make_RANGE(token, pos);
    case Token::READ:
        return Parser::make_READ(token, pos);
    case Token::RESET:
        return Parser::make_RESET(token, pos);
    case Token::SECTION:
        return Parser::make_SECTION(token, pos);
    case Token::SENS:
        return Parser::make_SENS(token, pos);
    case Token::SOLVE:
        return Parser::make_SOLVE(token, pos);
    case Token::SOLVEFOR:
        return Parser::make_SOLVEFOR(token, pos);
    case Token::START1:
        return Parser::make_START1(token, pos);
    case Token::STATE:
        return Parser::make_STATE(token, pos);
    case Token::STEADYSTATE:
        return Parser::make_STEADYSTATE(token, pos);
    case Token::STEP:
        return Parser::make_STEP(token, pos);
    case Token::STEPPED:
        return Parser::make_STEPPED(token, pos);
    case Token::SWEEP:
        return Parser::make_SWEEP(token, pos);
    case Token::TABLE:
        return Parser::make_TABLE(token, pos);
    case Token::TERMINAL:
        return Parser::make_TERMINAL(token, pos);
    case Token::THREADSAFE:
        return Parser::make_THREADSAFE(token, pos);
    case Token::TO:
        return Parser::make_TO(token, pos);
    case Token::UNITS:
        return Parser::make_UNITS(token, pos);
    case Token::UNITSOFF:
        return Parser::make_UNITSOFF(token, pos);
    case Token::UNITSON:
        return Parser::make_UNITSON(token, pos);
    case Token::USEION:
        return Parser::make_USEION(token, pos);
    case Token::USING:
        return Parser::make_USING(token, pos);
    case Token::VS:
        return Parser::make_VS(token, pos);
    case Token::WATCH:
        return Parser::make_WATCH(token, pos);
    case Token::WHILE:
        return Parser::make_WHILE(token, pos);
    case Token::WITH:
        return Parser::make_WITH(token, pos);
    case Token::WRITE:
        return Parser::make_WRITE(token, pos);
    case Token::REACT1:
        return Parser::make_REACT1(token, pos);
    case Token::NONLIN1:
        return Parser::make_NONLIN1(token, pos);
    case Token::LIN1:
        return Parser::make_LIN1(token, pos);
    case Token::TILDE:
        return Parser::make_TILDE(token, pos);
    case Token::REACTION:
        return Parser::make_REACTION(token, pos);
    case Token::GT:
        return Parser::make_GT(token, pos);
    case Token::GE:
        return Parser::make_GE(token, pos);
    case Token::LT:
        return Parser::make_LT(token, pos);
    case Token::LE:
        return Parser::make_LE(token, pos);
    case Token::EQ:
        return Parser::make_EQ(token, pos);
    case Token::NE:
        return Parser::make_NE(token, pos);
    case Token::NOT:
        return Parser::make_NOT(token, pos);
    case Token::AND:
        return Parser::make_AND(token, pos);
    case Token::OR:
        return Parser::make_OR(token, pos);
    case Token::OPEN_BRACE:
        return Parser::make_OPEN_BRACE(token, pos);
    case Token::CLOSE_BRACE:
        return Parser::make_CLOSE_BRACE(token, pos);
    case Token::OPEN_PARENTHESIS:
        return Parser::make_OPEN_PARENTHESIS(token, pos);
    case Token::CLOSE_PARENTHESIS:
        return Parser::make_CLOSE_PARENTHESIS(token, pos);
    case Token::OPEN_BRACKET:
        return Parser::make_OPEN_BRACKET(token, pos);
    case Token::CLOSE_BRACKET:
        return Parser::make_CLOSE_BRACKET(token, pos);
    case Token::AT:
        return Parser::make_AT(token, pos);
    case Token::ADD:
        return Parser::make_ADD(token, pos);
    case Token::MINUS:
        return Parser::make_MINUS(token, pos);
    case Token::MULTIPLY:
        return Parser::make_MULTIPLY(token, pos);
    case Token::DIVIDE:
        return Parser::make_DIVIDE(token, pos);
    case Token::EQUAL:
        return Parser::make_EQUAL(token, pos);
    case Token::CARET:
        return Parser::make_CARET(token, pos);
    case Token::COLON:
        return Parser::make_COLON(token, pos);
    case Token::COMMA:
        return Parser::make_COMMA(token, pos);
    case Token::PERIOD:
        return Parser::make_PERIOD(token, pos);
    case Token::REPRESENTS:
        return Parser::make_REPRESENTS(token, pos);
    /**
     * we hit default case for missing token type. This is more likely
     * a bug in the implementation where we forgot to handle token type.
     */
    default:
        auto msg = "Token type not found while creating a symbol!";
        throw std::runtime_error(msg);
    }
}

}  // namespace nmodl
