#ifndef _NMODL_CONTEXT_
#define _NMODL_CONTEXT_

#include <iostream>
#include <map>
#include "ast/ast.hpp"

class NmodlContext {
  public:
    /* root of the ast */
    ast::ProgramNode* astRoot;

    /* main scanner for NMODL */
    void* scanner;

    /* input stream for parsing */
    std::istream* is;

    /* list of all macro defined variables */
    std::map<std::string, int> defined_var;

    /* constructor */
    NmodlContext(std::istream* is = &std::cin) {
        init_scanner();
        this->is = is;
        this->astRoot = NULL;
    }

    void add_defined_var(std::string var, int value) {
        defined_var[var] = value;
    }

    bool is_defined_var(std::string name) {
        if (defined_var.find(name) == defined_var.end()) {
            return false;
        }

        return true;
    }

    int get_defined_var_value(std::string name) {
        if (is_defined_var(name)) {
            return defined_var[name];
        }

        std::cout << "\n ERROR: Trying to get value of undefined value!";
        abort();
    }

    virtual ~NmodlContext() {
        destroy_scanner();

        if (astRoot) {
            delete astRoot;
        }
    }

  protected:
    /* defined in lexer.l */
    void init_scanner();
    void destroy_scanner();
};

int Nmodl_parse(NmodlContext*);

#endif  // _NMODL_CONTEXT_
