/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <cstring>
#include <map>
#include <vector>

#include "ast/ast.hpp"
#include "lexer/modl.h"
#include "lexer/token_mapping.hpp"
#include "parser/nmodl/nmodl_parser.hpp"

namespace nmodl {

using Token = parser::NmodlParser::token;
using TokenType = parser::NmodlParser::token_type;
using Parser = parser::NmodlParser;

/// details of lexer tokens
namespace details {

/**
 * \brief Keywords from NMODL language
 *
 * Keywords are defined with key-value pair where key is name
 * from scanner and value is token type used in parser.
 *
 * \todo Some keywords have different token names, e.g. TITLE
 * keyword has MODEL as a keyword. These token names are used
 * in multiple context and hence we are keeping original names.
 * Once we finish code generation part then we change this.
 */
const static std::map<std::string, TokenType> keywords = {
    {"VERBATIM", Token::VERBATIM},
    {"COMMENT", Token::BLOCK_COMMENT},
    {"TITLE", Token::MODEL},
    {"CONSTANT", Token::CONSTANT},
    {"PARAMETER", Token::PARAMETER},
    {"INDEPENDENT", Token::INDEPENDENT},
    {"ASSIGNED", Token::ASSIGNED},
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


/**
 * \class MethodInfo
 * \brief Information about integration method
 */
struct MethodInfo {
    /// block types where this method will work with
    int64_t subtype = 0;

    /// true if it is a variable timestep method
    int variable_timestep = 0;

    MethodInfo() = default;

    MethodInfo(int64_t s, int v)
        : subtype(s)
        , variable_timestep(v) {}
};


/**
 * Integration methods available in the NMODL
 *
 * Different integration methods are available in NMODL and they are used with
 * different block types in NMODL. This variable provide list of method names,
 * which blocks they can be used with and whether it is usable with variable
 * timestep.
 *
 * \todo MethodInfo::subtype should be changed from integer flag to proper type
 */
const static std::map<std::string, MethodInfo> methods = {{"adams", MethodInfo(DERF | KINF, 0)},
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


/**
 * Definition type similar to old implementation
 *
 * In the original implementation of NMODL (mod2c, nocmodl) different vectors were
 * created for \c extdef, \c extdef2, \c extdef3, \c extdef4 etc. We are changing
 * those vectors with <c><name, type></c> map. This will help us to search
 * in single map and find it's type. The types are defined as follows:
 *
 * - DefinitionType::EXT_DOUBLE : external names that can be used as doubles
 *                                without giving an error message
 * - DefinitionType::EXT_2      : external function names that can be used with
 *                                array and function name arguments
 * - DefinitionType::EXT_3      : function names that get two reset arguments
 * - DefinitionType::EXT_4      : functions that need a first arg of \c NrnThread*
 * - DefinitionType::EXT_5      : external definition names that are not \c threadsafe
 *
 */
enum class DefinitionType { EXT_DOUBLE, EXT_2, EXT_3, EXT_4, EXT_5 };


const static std::map<std::string, DefinitionType> extern_definitions = {
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


/**
 * Checks if \c token is one of the functions coming from NEURON/CoreNEURON and needs
 * passing NrnThread* as first argument (typical name of variable \c nt)
 *
 * @param token Name of function
 * @return True or false depending if the function needs NrnThread* argument
 */
bool needs_neuron_thread_first_arg(const std::string& token) {
    auto extern_def = extern_definitions.find(token);
    return extern_def != extern_definitions.end() && extern_def->second == DefinitionType::EXT_4;
}


/**
 * Variables from NEURON that are directly used in NMODL
 *
 * NEURON exposes certain variable that can be directly used in NMODLvar.
 * The passes like scope checker needs to know if certain variable is
 * undefined and hence these needs to be inserted into symbol table
 */
static std::vector<std::string> NEURON_VARIABLES = {"t", "dt", "celsius", "v", "diam", "area"};


/// Return token type for the keyword
TokenType keyword_type(const std::string& name) {
    return keywords.at(name);
}

}  // namespace details


/**
 * Check if given name is a keyword in NMODL
 * @param name token name
 * @return true if name is a keyword
 */
bool is_keyword(const std::string& name) {
    return (details::keywords.find(name) != details::keywords.end());
}


/**
 * Check if given name is an integration method in NMODL
 * @param name Name of the integration method
 * @return true if name is an integration method in NMODL
 */
bool is_method(const std::string& name) {
    return (details::methods.find(name) != details::methods.end());
}


/**
 * Return token type for given token name
 * @param name Token name from lexer
 * @return type of NMODL token
 */
TokenType token_type(const std::string& name) {
    if (is_keyword(name)) {
        return details::keyword_type(name);
    }
    if (is_method(name)) {
        return Token::METHOD;
    }
    throw std::runtime_error("token_type called for non-existent token " + name);
}


/**
 * Return variables declared in NEURON that are available to NMODL
 * @return vector of NEURON variables
 */
std::vector<std::string> get_external_variables() {
    std::vector<std::string> result;
    result.insert(result.end(), details::NEURON_VARIABLES.begin(), details::NEURON_VARIABLES.end());
    return result;
}


/**
 * Return functions that can be used in the NMODL
 * @return vector of function names used in NMODL
 */
std::vector<std::string> get_external_functions() {
    std::vector<std::string> result;
    result.reserve(details::methods.size());
    for (auto& method: details::methods) {
        result.push_back(method.first);
    }
    for (auto& definition: details::extern_definitions) {
        result.push_back(definition.first);
    }
    return result;
}

}  // namespace nmodl
