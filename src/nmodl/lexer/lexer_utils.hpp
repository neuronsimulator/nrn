#ifndef _LEXER_UTILS_HPP_
#define _LEXER_UTILS_HPP_

#include <string>
#include "ast/ast.hpp"

/* lexer provide some utility functions to process input */
namespace LexerUtils {
    ast::StringNode* inputtoparpar(void* scanner);
    std::string inputline(void* scanner);
    std::string input_until_token(std::string, void* scanner);
}

#endif