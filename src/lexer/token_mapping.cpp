/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <cstring>
#include <map>
#include <vector>

#include "ast/ast.hpp"
#include "lexer/modl.h"
#include "parser/nmodl/nmodl_parser.hpp"

namespace nmodl {

using Token = parser::NmodlParser::token;
using TokenType = parser::NmodlParser::token_type;
using Parser = parser::NmodlParser;

namespace internal {
// clang-format off
        /** Keywords from NMODL language : name and token pair
         *
         * \todo Some keywords have different token names, e.g. TITLE
         * keyword has MODEL as a keyword. These token names are used
         * in multiple context and hence we are keeping original names.
         * Once we finish code generation part then we change this. */
        static std::map<std::string, TokenType> keywords = {
            {"VERBATIM", Token::VERBATIM},
            {"COMMENT", Token::BLOCK_COMMENT},
            {"TITLE", Token::MODEL},
            {"CONSTANT", Token::CONSTANT},
            {"PARAMETER", Token::PARAMETER},
            {"INDEPENDENT", Token::INDEPENDENT},
            {"ASSIGNED", Token::DEPENDENT},
            {"INITIAL", Token::INITIAL1},
            {"TERMINAL", Token::TERMINAL},
            {"DERIVATIVE", Token::DERIVATIVE},
            {"EQUATION", Token::BREAKPOINT},
            {"BREAKPOINT", Token::BREAKPOINT},
            {"CONDUCTANCE", Token::CONDUCTANCE},
            {"SOLVE", Token::SOLVE},
            {"STATE", Token::STATE},
            {"STEPPED", Token::STEPPED},
            {"LINEAR", Token::LINEAR},
            {"NONLINEAR", Token::NONLINEAR},
            {"DISCRETE", Token::DISCRETE},
            {"FUNCTION", Token::FUNCTION1},
            {"FUNCTION_TABLE", Token::FUNCTION_TABLE},
            {"PROCEDURE", Token::PROCEDURE},
            {"PARTIAL", Token::PARTIAL},
            {"DEL2", Token::DEL2},
            {"DEL", Token::DEL},
            {"LOCAL", Token::LOCAL},
            {"METHOD", Token::USING},
            {"STEADYSTATE", Token::STEADYSTATE},
            {"SENS", Token::SENS},
            {"STEP", Token::STEP},
            {"WITH", Token::WITH},
            {"FROM", Token::FROM},
            {"FORALL", Token::FORALL1},
            {"TO", Token::TO},
            {"BY", Token::BY},
            {"if", Token::IF},
            {"else", Token::ELSE},
            {"while", Token::WHILE},
            {"START", Token::START1},
            {"DEFINE", Token::DEFINE1},
            {"KINETIC", Token::KINETIC},
            {"CONSERVE", Token::CONSERVE},
            {"PLOT", Token::PLOT},
            {"VS", Token::VS},
            {"LAG", Token::LAG},
            {"RESET", Token::RESET},
            {"MATCH", Token::MATCH},
            {"MODEL_LEVEL", Token::MODEL_LEVEL},
            {"SWEEP", Token::SWEEP},
            {"FIRST", Token::FIRST},
            {"LAST", Token::LAST},
            {"COMPARTMENT", Token::COMPARTMENT},
            {"LONGITUDINAL_DIFFUSION", Token::LONGDIFUS},
            {"PUTQ", Token::PUTQ},
            {"GETQ", Token::GETQ},
            {"IFERROR", Token::IFERROR},
            {"SOLVEFOR", Token::SOLVEFOR},
            {"UNITS", Token::UNITS},
            {"UNITSON", Token::UNITSON},
            {"UNITSOFF", Token::UNITSOFF},
            {"TABLE", Token::TABLE},
            {"DEPEND", Token::DEPEND},
            {"NEURON", Token::NEURON},
            {"SUFFIX", Token::SUFFIX},
            {"POINT_PROCESS", Token::SUFFIX},
            {"ARTIFICIAL_CELL", Token::SUFFIX},
            {"NONSPECIFIC_CURRENT", Token::NONSPECIFIC},
            {"ELECTRODE_CURRENT", Token::ELECTRODE_CURRENT},
            {"SECTION", Token::SECTION},
            {"RANGE", Token::RANGE},
            {"USEION", Token::USEION},
            {"READ", Token::READ},
            {"WRITE", Token::WRITE},
            {"VALENCE", Token::VALENCE},
            {"CHARGE", Token::VALENCE},
            {"GLOBAL", Token::GLOBAL},
            {"POINTER", Token::POINTER},
            {"BBCOREPOINTER", Token::BBCOREPOINTER},
            {"EXTERNAL", Token::EXTERNAL},
            {"INCLUDE", Token::INCLUDE1},
            {"CONSTRUCTOR", Token::CONSTRUCTOR},
            {"DESTRUCTOR", Token::DESTRUCTOR},
            {"NET_RECEIVE", Token::NETRECEIVE},
            {"BEFORE", Token::BEFORE},
            {"AFTER", Token::AFTER},
            {"WATCH", Token::WATCH},
            {"FOR_NETCONS", Token::FOR_NETCONS},
            {"THREADSAFE", Token::THREADSAFE},
            {"PROTECT", Token::PROTECT},
            {"MUTEXLOCK", Token::NRNMUTEXLOCK},
            {"MUTEXUNLOCK", Token::NRNMUTEXUNLOCK}};
// clang-format on

/// numerical methods supported in nmodl
struct MethodInfo {
    /// method types that will work with this method
    int64_t subtype = 0;

    /// if it's a variable timestep method
    int varstep = 0;

    MethodInfo() = default;
    MethodInfo(int64_t s, int v)
        : subtype(s)
        , varstep(v) {}
};

// clang-format off
        static std::map<std::string, MethodInfo> methods = {{"adams", MethodInfo(DERF | KINF, 0)},
                                                            {"runge", MethodInfo(DERF | KINF, 0)},
                                                            {"euler", MethodInfo(DERF | KINF, 0)},
                                                            {"adeuler", MethodInfo(DERF | KINF, 1)},
                                                            {"heun", MethodInfo(DERF | KINF, 0)},
                                                            {"adrunge", MethodInfo(DERF | KINF, 1)},
                                                            {"gear", MethodInfo(DERF | KINF, 1)},
                                                            {"newton", MethodInfo(NLINF, 0)},
                                                            {"simplex", MethodInfo(NLINF, 0)},
                                                            {"simeq", MethodInfo(LINF, 0)},
                                                            {"seidel", MethodInfo(LINF, 0)},
                                                            {"_advance", MethodInfo(KINF, 0)},
                                                            {"sparse", MethodInfo(KINF, 0)},
                                                            {"derivimplicit", MethodInfo(DERF, 0)},
                                                            {"cnexp", MethodInfo(DERF, 0)},
                                                            {"clsoda", MethodInfo(DERF | KINF, 1)},
                                                            {"after_cvode", MethodInfo(0, 0)},
                                                            {"cvode_t", MethodInfo(0, 0)},
                                                            {"cvode_t_v", MethodInfo(0, 0)}};
// clang-format on

/** In the original implementation different vectors were created for
 * extdef, extdef2, extdef3, extdef4 etc. Instead of that we are changing
 * those vectors with <name, type> map. This will help us to search
 * in single map and find it's type. The types are defined as follows:
 *
 * DefinitionType::EXT_DOUBLE : external names that can be used as doubles
 *                              without giving an error message
 *
 * DefinitionType::EXT_2      : external function names that can be used with
 *                              array and function name arguments
 *
 * DefinitionType::EXT_3      : function names that get two reset arguments
 *
 * DefinitionType::EXT_4      : functions that need a first arg of NrnThread*
 *
 * DefinitionType::EXT_5      : the extdef names that are not threadsafe
 *
 * These types were used so that it's easy to it to old implementation. */

enum class DefinitionType { EXT_DOUBLE, EXT_2, EXT_3, EXT_4, EXT_5 };

// clang-format off
        static std::map<std::string, DefinitionType> extern_definitions = {
            {"first_time", DefinitionType::EXT_DOUBLE},
            {"error", DefinitionType::EXT_DOUBLE},
            {"f_flux", DefinitionType::EXT_DOUBLE},
            {"b_flux", DefinitionType::EXT_DOUBLE},
            {"fabs", DefinitionType::EXT_DOUBLE},
            {"sqrt", DefinitionType::EXT_DOUBLE},
            {"sin", DefinitionType::EXT_DOUBLE},
            {"cos", DefinitionType::EXT_DOUBLE},
            {"tan", DefinitionType::EXT_DOUBLE},
            {"acos", DefinitionType::EXT_DOUBLE},
            {"asin", DefinitionType::EXT_DOUBLE},
            {"atan", DefinitionType::EXT_DOUBLE},
            {"atan2", DefinitionType::EXT_DOUBLE},
            {"sinh", DefinitionType::EXT_DOUBLE},
            {"cosh", DefinitionType::EXT_DOUBLE},
            {"tanh", DefinitionType::EXT_DOUBLE},
            {"floor", DefinitionType::EXT_DOUBLE},
            {"ceil", DefinitionType::EXT_DOUBLE},
            {"fmod", DefinitionType::EXT_DOUBLE},
            {"log10", DefinitionType::EXT_DOUBLE},
            {"log", DefinitionType::EXT_DOUBLE},
            {"pow", DefinitionType::EXT_DOUBLE},
            {"printf", DefinitionType::EXT_DOUBLE},
            {"prterr", DefinitionType::EXT_DOUBLE},
            {"exp", DefinitionType::EXT_DOUBLE},
            {"threshold", DefinitionType::EXT_DOUBLE},
            {"force", DefinitionType::EXT_DOUBLE},
            {"deflate", DefinitionType::EXT_DOUBLE},
            {"expfit", DefinitionType::EXT_DOUBLE},
            {"derivs", DefinitionType::EXT_DOUBLE},
            {"spline", DefinitionType::EXT_DOUBLE},
            {"hyperbol", DefinitionType::EXT_DOUBLE},
            {"revhyperbol", DefinitionType::EXT_DOUBLE},
            {"sigmoid", DefinitionType::EXT_DOUBLE},
            {"revsigmoid", DefinitionType::EXT_DOUBLE},
            {"harmonic", DefinitionType::EXT_DOUBLE},
            {"squarewave", DefinitionType::EXT_DOUBLE},
            {"sawtooth", DefinitionType::EXT_DOUBLE},
            {"revsawtooth", DefinitionType::EXT_DOUBLE},
            {"ramp", DefinitionType::EXT_DOUBLE},
            {"pulse", DefinitionType::EXT_DOUBLE},
            {"perpulse", DefinitionType::EXT_DOUBLE},
            {"step", DefinitionType::EXT_DOUBLE},
            {"perstep", DefinitionType::EXT_DOUBLE},
            {"erf", DefinitionType::EXT_DOUBLE},
            {"exprand", DefinitionType::EXT_DOUBLE},
            {"factorial", DefinitionType::EXT_DOUBLE},
            {"gauss", DefinitionType::EXT_DOUBLE},
            {"normrand", DefinitionType::EXT_DOUBLE},
            {"poisrand", DefinitionType::EXT_DOUBLE},
            {"poisson", DefinitionType::EXT_DOUBLE},
            {"setseed", DefinitionType::EXT_DOUBLE},
            {"scop_random", DefinitionType::EXT_DOUBLE},
            {"boundary", DefinitionType::EXT_DOUBLE},
            {"romberg", DefinitionType::EXT_DOUBLE},
            {"legendre", DefinitionType::EXT_DOUBLE},
            {"invert", DefinitionType::EXT_DOUBLE},
            {"stepforce", DefinitionType::EXT_DOUBLE},
            {"schedule", DefinitionType::EXT_DOUBLE},
            {"set_seed", DefinitionType::EXT_DOUBLE},
            {"nrn_pointing", DefinitionType::EXT_DOUBLE},
            {"state_discontinuity", DefinitionType::EXT_DOUBLE},
            {"net_send", DefinitionType::EXT_DOUBLE},
            {"net_move", DefinitionType::EXT_DOUBLE},
            {"net_event", DefinitionType::EXT_DOUBLE},
            {"nrn_random_play", DefinitionType::EXT_DOUBLE},
            {"at_time", DefinitionType::EXT_DOUBLE},
            {"nrn_ghk", DefinitionType::EXT_DOUBLE},
            {"romberg", DefinitionType::EXT_2},
            {"legendre", DefinitionType::EXT_2},
            {"deflate", DefinitionType::EXT_2},
            {"threshold", DefinitionType::EXT_3},
            {"squarewave", DefinitionType::EXT_3},
            {"sawtooth", DefinitionType::EXT_3},
            {"revsawtooth", DefinitionType::EXT_3},
            {"ramp", DefinitionType::EXT_3},
            {"pulse", DefinitionType::EXT_3},
            {"perpulse", DefinitionType::EXT_3},
            {"step", DefinitionType::EXT_3},
            {"perstep", DefinitionType::EXT_3},
            {"stepforce", DefinitionType::EXT_3},
            {"schedule", DefinitionType::EXT_3},
            {"at_time", DefinitionType::EXT_4},
            {"force", DefinitionType::EXT_5},
            {"deflate", DefinitionType::EXT_5},
            {"expfit", DefinitionType::EXT_5},
            {"derivs", DefinitionType::EXT_5},
            {"spline", DefinitionType::EXT_5},
            {"exprand", DefinitionType::EXT_5},
            {"gauss", DefinitionType::EXT_5},
            {"normrand", DefinitionType::EXT_5},
            {"poisrand", DefinitionType::EXT_5},
            {"poisson", DefinitionType::EXT_5},
            {"setseed", DefinitionType::EXT_5},
            {"scop_random", DefinitionType::EXT_5},
            {"boundary", DefinitionType::EXT_5},
            {"romberg", DefinitionType::EXT_5},
            {"invert", DefinitionType::EXT_5},
            {"stepforce", DefinitionType::EXT_5},
            {"schedule", DefinitionType::EXT_5},
            {"set_seed", DefinitionType::EXT_5},
            {"nrn_random_play", DefinitionType::EXT_5}};
// clang-format on

/** Internal NEURON variables that can be used in nmod files. The compiler
 * passes like scope checker need to know if certain variable is undefined.
 * Note that these are not used by lexer/parser. */

static std::vector<std::string> neuron_vars = {"t", "dt", "celsius", "v", "diam", "area"};

TokenType keyword_type(const std::string& name) {
    return keywords[name];
}

/** \todo: revisit implementation, this is no longer
 *        necessary as token_type is sufficient
 */
TokenType method_type(const std::string& /*name*/) {
    return Token::METHOD;
}

bool is_externdef(const std::string& name) {
    return (extern_definitions.find(name) != extern_definitions.end());
}

DefinitionType extdef_type(const std::string& name) {
    if (!is_externdef(name)) {
        throw std::runtime_error("Can't find " + name + " in external definitions!");
    }
    return extern_definitions[name];
}

}  // namespace internal

/// methods exposed to lexer, parser and compilers passes

bool is_keyword(const std::string& name) {
    return (internal::keywords.find(name) != internal::keywords.end());
}

bool is_method(const std::string& name) {
    return (internal::methods.find(name) != internal::methods.end());
}

/// return token type for associated name (used by nmodl scanner)
TokenType token_type(const std::string& name) {
    if (is_keyword(name)) {
        return internal::keyword_type(name);
    }
    if (is_method(name)) {
        return internal::method_type(name);
    }
    throw std::runtime_error("get_token_type called for non-existent token " + name);
}

/// return all external variables
std::vector<std::string> get_external_variables() {
    std::vector<std::string> result;
    result.insert(result.end(), internal::neuron_vars.begin(), internal::neuron_vars.end());
    return result;
}

/// return all solver methods as well as commonly used math functions
std::vector<std::string> get_external_functions() {
    std::vector<std::string> result;
    result.reserve(internal::methods.size());
    for (auto& method: internal::methods) {
        result.push_back(method.first);
    }
    for (auto& definition: internal::extern_definitions) {
        result.push_back(definition.first);
    }
    return result;
}

}  // namespace nmodl
