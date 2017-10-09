#include <string.h>
#include <map>
#include <vector>
#include <iostream>
#include <iterator>
#include "lexer/modtoken.hpp"
#include "ast/ast.hpp"
#include "parser/nmodl_context.hpp"
#include "parser/nmodl_parser.hpp"
#include "lexer/modl.h"
#include "lexer/init.hpp"

/* Lookup the token from lexer and check if it is keyword, method or external
 * function definition. Currently just return type, no symbol creation
 */
ModToken* putintoken(std::string key, int type, Position pos) {
    int toktype;

    switch (type) {
        case STRING:
        case REAL:
        case INTEGER:
            toktype = type;
            break;

        default:

            /* check if name is KEYWORD in NMODL */
            if (Keywords::exists(key)) {
                toktype = Keywords::type(key);
                break;
            }

            /* check if name is METHOD name in NMODL */
            if (Methods::exists(key)) {
                toktype = Methods::type(key);
                break;
            }

            /* current token is neither keyword nor method name, treating as NAME */
            toktype = NAME;
    }

    ModToken* tok = new ModToken(key, toktype, pos);

    // std::cout << std::endl << "\t" <<  tok;

    return tok;
}
