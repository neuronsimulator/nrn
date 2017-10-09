#include <string.h>
#include <map>
#include <vector>
#include <set>
#include <iostream>
#include <iterator>
#include "lexer/modtoken.hpp"
#include "ast/ast.hpp"
#include "parser/nmodl_context.hpp"
#include "parser/nmodl_parser.hpp"
#include "lexer/modl.h"
#include "lexer/init.hpp"

/* Keywords from NMODL reprsent name and kval pair
 * in the original struct [source init.c]
 */
namespace Keywords {

    static std::map<std::string, int> create_keywords_map() {
        /* @todo: map for pretty-printing keyrowrds in sorted order? */
        std::map<std::string, int> km;

        km["VERBATIM"] = VERBATIM;
        km["COMMENT"] = COMMENT;
        km["TITLE"] = MODEL;
        km["CONSTANT"] = CONSTANT;
        km["PARAMETER"] = PARAMETER;
        km["INDEPENDENT"] = INDEPENDENT;
        km["ASSIGNED"] = DEPENDENT;
        km["INITIAL"] = INITIAL1;
        km["TERMINAL"] = TERMINAL;
        km["DERIVATIVE"] = DERIVATIVE;
        km["EQUATION"] = EQUATION;
        km["BREAKPOINT"] = BREAKPOINT;
        km["CONDUCTANCE"] = CONDUCTANCE;
        km["SOLVE"] = SOLVE;
        km["STATE"] = STATE;
        km["STEPPED"] = STEPPED;
        km["LINEAR"] = LINEAR;
        km["NONLINEAR"] = NONLINEAR;
        km["DISCRETE"] = DISCRETE;
        km["FUNCTION"] = FUNCTION1;
        km["FUNCTION_TABLE"] = FUNCTION_TABLE;
        km["PROCEDURE"] = PROCEDURE;
        km["PARTIAL"] = PARTIAL;
        km["DEL2"] = DEL2;
        km["DEL"] = DEL;
        km["LOCAL"] = LOCAL;
        km["METHOD"] = USING;
        km["STEADYSTATE"] = USING;
        km["SENS"] = SENS;
        km["STEP"] = STEP;
        km["WITH"] = WITH;
        km["FROM"] = FROM;
        km["FORALL"] = FORALL1;
        km["TO"] = TO;
        km["BY"] = BY;
        km["if"] = IF;
        km["else"] = ELSE;
        km["while"] = WHILE;
        km["START"] = START1;
        km["DEFINE"] = DEFINE1;
        km["KINETIC"] = KINETIC;
        km["CONSERVE"] = CONSERVE;
        km["PLOT"] = PLOT;
        km["VS"] = VS;
        km["LAG"] = LAG;
        km["RESET"] = RESET;
        km["MATCH"] = MATCH;
        km["MODEL_LEVEL"] = MODEL_LEVEL; /* inserted by merge */
        km["SWEEP"] = SWEEP;
        km["FIRST"] = FIRST;
        km["LAST"] = LAST;
        km["COMPARTMENT"] = COMPARTMENT;
        km["LONGITUDINAL_DIFFUSION"] = LONGDIFUS;
        km["PUTQ"] = PUTQ;
        km["GETQ"] = GETQ;
        km["IFERROR"] = IFERROR;
        km["SOLVEFOR"] = SOLVEFOR;
        km["UNITS"] = UNITS;
        km["UNITSON"] = UNITSON;
        km["UNITSOFF"] = UNITSOFF;
        km["TABLE"] = TABLE;
        km["DEPEND"] = DEPEND;
        km["NEURON"] = NEURON;
        km["SUFFIX"] = SUFFIX;
        km["POINT_PROCESS"] = SUFFIX;
        km["ARTIFICIAL_CELL"] = SUFFIX;
        km["NONSPECIFIC_CURRENT"] = NONSPECIFIC;
        km["ELECTRODE_CURRENT"] = ELECTRODE_CURRENT;
        km["SECTION"] = SECTION;
        km["RANGE"] = RANGE;
        km["USEION"] = USEION;
        km["READ"] = READ;
        km["WRITE"] = WRITE;
        km["VALENCE"] = VALENCE;
        km["CHARGE"] = VALENCE;
        km["GLOBAL"] = GLOBAL;
        km["POINTER"] = POINTER;
        km["BBCOREPOINTER"] = BBCOREPOINTER;
        km["EXTERNAL"] = EXTERNAL;
        km["INCLUDE"] = INCLUDE1;
        km["CONSTRUCTOR"] = CONSTRUCTOR;
        km["DESTRUCTOR"] = DESTRUCTOR;
        km["NET_RECEIVE"] = NETRECEIVE;
        km["BEFORE"] = BEFORE; /* before NEURON sets up cy' = f(y;t) */
        km["AFTER"] = AFTER;   /* after NEURON solves cy' = f(y; t) */
        km["WATCH"] = WATCH;
        km["FOR_NETCONS"] = FOR_NETCONS;
        km["THREADSAFE"] = THREADSAFE;
        km["PROTECT"] = PROTECT;
        km["MUTEXLOCK"] = NRNMUTEXLOCK;
        km["MUTEXUNLOCK"] = NRNMUTEXUNLOCK;

        return km;
    }

    /* Keywords: name and kval pair */
    static std::map<std::string, int> keywords = create_keywords_map();

    /* keyword exists ? */
    bool exists(std::string key) {
        return (keywords.find(key) != keywords.end());
    }

    /* return type of the keyword */
    int type(std::string key) {
        if (!exists(key)) {
            /* throw exception or abort here */
            std::cout << "Error : " << key << " doesn't exist! " << std::endl;
            return -1;
        }

        return keywords[key];
    }
}  // namespace Keywords

namespace Methods {

    /* Numerical methods from NMODL reprsent name, all the types
     * that will work with this pair and whether it's a variable
     * time step method. [source init.c]
     */

    static std::map<std::string, MethodInfo> create_methods_map() {
        std::map<std::string, MethodInfo> mm;

        mm["adams"] = MethodInfo(DERF | KINF, 0);
        mm["runge"] = MethodInfo(DERF | KINF, 0);
        mm["euler"] = MethodInfo(DERF | KINF, 0);
        mm["adeuler"] = MethodInfo(DERF | KINF, 1);
        mm["heun"] = MethodInfo(DERF | KINF, 0);
        mm["adrunge"] = MethodInfo(DERF | KINF, 1);
        mm["gear"] = MethodInfo(DERF | KINF, 1);
        mm["newton"] = MethodInfo(NLINF, 0);
        mm["simplex"] = MethodInfo(NLINF, 0);
        mm["simeq"] = MethodInfo(LINF, 0);
        mm["seidel"] = MethodInfo(LINF, 0);
        mm["_advance"] = MethodInfo(KINF, 0);
        mm["sparse"] = MethodInfo(KINF, 0);
        mm["derivimplicit"] = MethodInfo(DERF, 0); /* name hard wired in deriv.c */
        mm["cnexp"] = MethodInfo(DERF, 0);         /* see solve.c */
        mm["clsoda"] = MethodInfo(DERF | KINF, 1); /* Tolerance built in to scopgear.c */
        mm["after_cvode"] = MethodInfo(0, 0);
        mm["cvode_t"] = MethodInfo(0, 0);
        mm["cvode_t_v"] = MethodInfo(0, 0);

        return mm;
    }

    /* Methods: name, subtype and varstep touple */
    static std::map<std::string, MethodInfo> methods = create_methods_map();

    /* Method exists ? */
    bool exists(std::string key) {
        return (methods.find(key) != methods.end());
    }

    /* return type of the method */
    int type(std::string key) {
        if (!exists(key)) {
            /* throw exception or abort here */
            std::cout << "Error : " << key << " doesn't exist! " << std::endl;
            return -1;
        }

        return METHOD;
    }

    std::vector<std::string> get_methods() {
        std::vector<std::string> v;

        for (std::map<std::string, MethodInfo>::iterator it = methods.begin(); it != methods.end(); ++it) {
            v.push_back(it->first);
        }

        return v;
    }
}  // namespace Methods

namespace ExternalDefinitions {

    /* In the original implementation, different vectors were created
     * for extdef, extdef2, extdef3, extdef4 etc. Instead of that we
     * are changing those vectors with <name, type> map. This will
     * help us to search in single map and find it's type. The types
     * are defined as follows:
     *
     * TYPE_EXTDEF_DOUBLE: external names that can be used as doubles
     *                      without giving an error message
     *
     * TYPE_EXTDEF_2     : external function names that can be used with
     *                      array and function name arguments
     *
     * TYPE_EXTDEF_3     : function names that get two reset arguments added
     *
     * TYPE_EXTDEF_4     : functions that need a first arg of NrnThread*
     *
     * TYPE_EXTDEF_5     : the extdef names that are not threadsafe
     *
     * These name types were used so that it's easy to it to old implementation.
     *
     */

    /* external names that can be used as doubles without giving an error message */
    static std::map<std::string, int> create_extdef_map() {
        std::map<std::string, int> extdef;

        extdef["first_time"] = TYPE_EXTDEF_DOUBLE;
        extdef["error"] = TYPE_EXTDEF_DOUBLE;
        extdef["f_flux"] = TYPE_EXTDEF_DOUBLE;
        extdef["b_flux"] = TYPE_EXTDEF_DOUBLE;
        extdef["fabs"] = TYPE_EXTDEF_DOUBLE;
        extdef["sqrt"] = TYPE_EXTDEF_DOUBLE;
        extdef["sin"] = TYPE_EXTDEF_DOUBLE;
        extdef["cos"] = TYPE_EXTDEF_DOUBLE;
        extdef["tan"] = TYPE_EXTDEF_DOUBLE;
        extdef["acos"] = TYPE_EXTDEF_DOUBLE;
        extdef["asin"] = TYPE_EXTDEF_DOUBLE;
        extdef["atan"] = TYPE_EXTDEF_DOUBLE;
        extdef["atan2"] = TYPE_EXTDEF_DOUBLE;
        extdef["sinh"] = TYPE_EXTDEF_DOUBLE;
        extdef["cosh"] = TYPE_EXTDEF_DOUBLE;
        extdef["tanh"] = TYPE_EXTDEF_DOUBLE;
        extdef["floor"] = TYPE_EXTDEF_DOUBLE;
        extdef["ceil"] = TYPE_EXTDEF_DOUBLE;
        extdef["fmod"] = TYPE_EXTDEF_DOUBLE;
        extdef["log10"] = TYPE_EXTDEF_DOUBLE;
        extdef["log"] = TYPE_EXTDEF_DOUBLE;
        extdef["pow"] = TYPE_EXTDEF_DOUBLE;
        extdef["printf"] = TYPE_EXTDEF_DOUBLE;
        extdef["prterr"] = TYPE_EXTDEF_DOUBLE;
        extdef["exp"] = TYPE_EXTDEF_DOUBLE;
        extdef["threshold"] = TYPE_EXTDEF_DOUBLE;
        extdef["force"] = TYPE_EXTDEF_DOUBLE;
        extdef["deflate"] = TYPE_EXTDEF_DOUBLE;
        extdef["expfit"] = TYPE_EXTDEF_DOUBLE;
        extdef["derivs"] = TYPE_EXTDEF_DOUBLE;
        extdef["spline"] = TYPE_EXTDEF_DOUBLE;
        extdef["hyperbol"] = TYPE_EXTDEF_DOUBLE;
        extdef["revhyperbol"] = TYPE_EXTDEF_DOUBLE;
        extdef["sigmoid"] = TYPE_EXTDEF_DOUBLE;
        extdef["revsigmoid"] = TYPE_EXTDEF_DOUBLE;
        extdef["harmonic"] = TYPE_EXTDEF_DOUBLE;
        extdef["squarewave"] = TYPE_EXTDEF_DOUBLE;
        extdef["sawtooth"] = TYPE_EXTDEF_DOUBLE;
        extdef["revsawtooth"] = TYPE_EXTDEF_DOUBLE;
        extdef["ramp"] = TYPE_EXTDEF_DOUBLE;
        extdef["pulse"] = TYPE_EXTDEF_DOUBLE;
        extdef["perpulse"] = TYPE_EXTDEF_DOUBLE;
        extdef["step"] = TYPE_EXTDEF_DOUBLE;
        extdef["perstep"] = TYPE_EXTDEF_DOUBLE;
        extdef["erf"] = TYPE_EXTDEF_DOUBLE;
        extdef["exprand"] = TYPE_EXTDEF_DOUBLE;
        extdef["factorial"] = TYPE_EXTDEF_DOUBLE;
        extdef["gauss"] = TYPE_EXTDEF_DOUBLE;
        extdef["normrand"] = TYPE_EXTDEF_DOUBLE;
        extdef["poisrand"] = TYPE_EXTDEF_DOUBLE;
        extdef["poisson"] = TYPE_EXTDEF_DOUBLE;
        extdef["setseed"] = TYPE_EXTDEF_DOUBLE;
        extdef["scop_random"] = TYPE_EXTDEF_DOUBLE;
        extdef["boundary"] = TYPE_EXTDEF_DOUBLE;
        extdef["romberg"] = TYPE_EXTDEF_DOUBLE;
        extdef["legendre"] = TYPE_EXTDEF_DOUBLE;
        extdef["invert"] = TYPE_EXTDEF_DOUBLE;
        extdef["stepforce"] = TYPE_EXTDEF_DOUBLE;
        extdef["schedule"] = TYPE_EXTDEF_DOUBLE;
        extdef["set_seed"] = TYPE_EXTDEF_DOUBLE;
        extdef["nrn_pointing"] = TYPE_EXTDEF_DOUBLE;
        extdef["state_discontinuity"] = TYPE_EXTDEF_DOUBLE;
        extdef["net_send"] = TYPE_EXTDEF_DOUBLE;
        extdef["net_move"] = TYPE_EXTDEF_DOUBLE;
        extdef["net_event"] = TYPE_EXTDEF_DOUBLE;
        extdef["nrn_random_play"] = TYPE_EXTDEF_DOUBLE;
        extdef["at_time"] = TYPE_EXTDEF_DOUBLE;
        extdef["nrn_ghk"] = TYPE_EXTDEF_DOUBLE;

        /* external function names that can be used with array and
         * function name arguments
         */
        extdef["romberg"] = TYPE_EXTDEF_2;
        extdef["legendre"] = TYPE_EXTDEF_2;
        extdef["deflate"] = TYPE_EXTDEF_2;

        /* function names that get two reset arguments added */
        extdef["threshold"] = TYPE_EXTDEF_3;
        extdef["squarewave"] = TYPE_EXTDEF_3;
        extdef["sawtooth"] = TYPE_EXTDEF_3;
        extdef["revsawtooth"] = TYPE_EXTDEF_3;
        extdef["ramp"] = TYPE_EXTDEF_3;
        extdef["pulse"] = TYPE_EXTDEF_3;
        extdef["perpulse"] = TYPE_EXTDEF_3;
        extdef["step"] = TYPE_EXTDEF_3;
        extdef["perstep"] = TYPE_EXTDEF_3;
        extdef["stepforce"] = TYPE_EXTDEF_3;
        extdef["schedule"] = TYPE_EXTDEF_3;

        /* functions that need a first arg of NrnThread* */
        extdef["at_time"] = TYPE_EXTDEF_4;

        /* the extdef names that are not threadsafe */
        extdef["force"] = TYPE_EXTDEF_5;
        extdef["deflate"] = TYPE_EXTDEF_5;
        extdef["expfit"] = TYPE_EXTDEF_5;
        extdef["derivs"] = TYPE_EXTDEF_5;
        extdef["spline"] = TYPE_EXTDEF_5;
        extdef["exprand"] = TYPE_EXTDEF_5;
        extdef["gauss"] = TYPE_EXTDEF_5;
        extdef["normrand"] = TYPE_EXTDEF_5;
        extdef["poisrand"] = TYPE_EXTDEF_5;
        extdef["poisson"] = TYPE_EXTDEF_5;
        extdef["setseed"] = TYPE_EXTDEF_5;
        extdef["scop_random"] = TYPE_EXTDEF_5;
        extdef["boundary"] = TYPE_EXTDEF_5;
        extdef["romberg"] = TYPE_EXTDEF_5;
        extdef["invert"] = TYPE_EXTDEF_5;
        extdef["stepforce"] = TYPE_EXTDEF_5;
        extdef["schedule"] = TYPE_EXTDEF_5;
        extdef["set_seed"] = TYPE_EXTDEF_5;
        extdef["nrn_random_play"] = TYPE_EXTDEF_5;

        return extdef;
    }

    /* external names, function names with various attributes */
    static std::map<std::string, int> extdef = create_extdef_map();

    /* External definition exists ? */
    bool exists(std::string key) {
        return (extdef.find(key) != extdef.end());
    }

    /* return type of the external definition */
    int type(std::string key) {
        if (!exists(key)) {
            /* throw exception or abort here */
            std::cout << "Error : " << key << " doesn't exist! " << std::endl;
            return -1;
        }

        return extdef[key];
    }

    std::vector<std::string> get_extern_definitions() {
        std::vector<std::string> v;

        for (std::map<std::string, int>::iterator it = extdef.begin(); it != extdef.end(); ++it) {
            v.push_back(it->first);
        }

        return v;
    }
}  // namespace ExternalDefinitions

/* after constructing symbol table noticed that the variables
 * from neuron are not yet listed so that error checker passes
 * could use those. Note that these are not used by lexer at the
 * moment and hence adding as vector of strings.
 */
namespace NeuronVariables {

    /* neuron variables used in neuron */
    static std::set<std::string> create_neuron_var_vec() {
        std::set<std::string> var;
        var.insert("t");
        var.insert("dt");
        var.insert("celsius");
        var.insert("v");
        var.insert("diam");
        var.insert("area");
        return var;
    }

    /* neuron variables */
    static std::set<std::string> variables = create_neuron_var_vec();

    /* check if neuron variable */
    bool exists(std::string name) {
        return (variables.find(name) != variables.end());
    }

    std::vector<std::string> get_neuron_vars() {
        std::vector<std::string> v(variables.begin(), variables.end());
        return v;
    }
}  // namespace NeuronVariables

namespace Lexer {
    std::vector<std::string> get_external_symbols() {
        std::vector<std::string> v, tmp;

        tmp = NeuronVariables::get_neuron_vars();
        v.insert(v.end(), tmp.begin(), tmp.end());

        tmp = Methods::get_methods();
        v.insert(v.end(), tmp.begin(), tmp.end());

        tmp = ExternalDefinitions::get_extern_definitions();
        v.insert(v.end(), tmp.begin(), tmp.end());

        return v;
    }

    static std::vector<std::string> extern_symbols = get_external_symbols();

    /* check if external symbol */
    bool is_extern_variable(std::string name) {
        bool exist = false;

        if (std::find(extern_symbols.begin(), extern_symbols.end(), name) != extern_symbols.end()) {
            exist = true;
        }

        return exist;
    }
}  // namespace Lexer
