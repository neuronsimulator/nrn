#ifndef _INIT_H_
#define _INIT_H_

/* Numerical methods from NMODL reprsent name, all the types
 * that will work with this pair and whether it's a variable
 * time step method. [source init.c]
 */
struct MethodInfo {
    long subtype;  /* all the types that will work with this */
    short varstep; /* whether it's a variable step method */

    MethodInfo() : subtype(0), varstep(0) {
    }
    MethodInfo(long s, short v) : subtype(s), varstep(v) {
    }
};

namespace Keywords {
    bool exists(std::string key);
    int type(std::string key);
}  // namespace Keywords

namespace Methods {
    bool exists(std::string key);
    int type(std::string key);
}  // namespace Methods

namespace ExternalDefinitions {
    bool exists(std::string key);
    int type(std::string key);
}  // namespace ExternalDefinitions

namespace NeuronVariables {
    bool exists(std::string key);
}

/* naming convention is based on original nocmodl implementation */
enum { TYPE_EXTDEF_DOUBLE, TYPE_EXTDEF_2, TYPE_EXTDEF_3, TYPE_EXTDEF_4, TYPE_EXTDEF_5 };

namespace Lexer {
    bool is_extern_variable(std::string name);
    std::vector<std::string> get_external_symbols();
}  // namespace Lexer

#endif
