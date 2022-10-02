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
    {"DERIVATIVE", Token::DERIVATIVE},
    {"EQUATION", Token::BREAKPOINT},
    {"BREAKPOINT", Token::BREAKPOINT},
    {"CONDUCTANCE", Token::CONDUCTANCE},
    {"SOLVE", Token::SOLVE},
    {"STATE", Token::STATE},
    {"LINEAR", Token::LINEAR},
    {"NONLINEAR", Token::NONLINEAR},
    {"DISCRETE", Token::DISCRETE},
    {"FUNCTION", Token::FUNCTION1},
    {"FUNCTION_TABLE", Token::FUNCTION_TABLE},
    {"PROCEDURE", Token::PROCEDURE},
    {"DEL2", Token::DEL2},
    {"DEL", Token::DEL},
    {"LOCAL", Token::LOCAL},
    {"METHOD", Token::USING},
    {"STEADYSTATE", Token::STEADYSTATE},
    {"STEP", Token::STEP},
    {"WITH", Token::WITH},
    {"FROM", Token::FROM},
    {"TO", Token::TO},
    {"BY", Token::BY},
    {"if", Token::IF},
    {"else", Token::ELSE},
    {"while", Token::WHILE},
    {"START", Token::START1},
    {"DEFINE", Token::DEFINE1},
    {"KINETIC", Token::KINETIC},
    {"CONSERVE", Token::CONSERVE},
    {"VS", Token::VS},
    {"LAG", Token::LAG},
    {"SWEEP", Token::SWEEP},
    {"COMPARTMENT", Token::COMPARTMENT},
    {"LONGITUDINAL_DIFFUSION", Token::LONGDIFUS},
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
    {"RANGE", Token::RANGE},
    {"USEION", Token::USEION},
    {"READ", Token::READ},
    {"REPRESENTS", Token::REPRESENTS},
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

const static std::vector<std::string> extern_definitions = {"acos",
                                                            "asin",
                                                            "at_time",
                                                            "atan",
                                                            "atan2",
                                                            "b_flux",
                                                            "boundary",
                                                            "ceil",
                                                            "cos",
                                                            "cosh",
                                                            "deflate",
                                                            "derivs",
                                                            "erf",
                                                            "error",
                                                            "exp",
                                                            "expfit",
                                                            "exprand",
                                                            "f_flux",
                                                            "fabs",
                                                            "factorial",
                                                            "first_time",
                                                            "floor",
                                                            "fmod",
                                                            "force",
                                                            "gauss",
                                                            "harmonic",
                                                            "hyperbol",
                                                            "invert",
                                                            "legendre",
                                                            "log",
                                                            "log10",
                                                            "net_event",
                                                            "net_move",
                                                            "net_send",
                                                            "normrand",
                                                            "nrn_ghk",
                                                            "nrn_pointing",
                                                            "nrn_random_play",
                                                            "perpulse",
                                                            "perstep",
                                                            "poisrand",
                                                            "poisson",
                                                            "pow",
                                                            "printf",
                                                            "prterr",
                                                            "pulse",
                                                            "ramp",
                                                            "revhyperbol",
                                                            "revsawtooth",
                                                            "revsigmoid",
                                                            "romberg",
                                                            "sawtooth",
                                                            "schedule",
                                                            "scop_random",
                                                            "set_seed",
                                                            "setseed",
                                                            "sigmoid",
                                                            "sin",
                                                            "sinh",
                                                            "spline",
                                                            "sqrt",
                                                            "squarewave",
                                                            "state_discontinuity",
                                                            "step",
                                                            "stepforce",
                                                            "tan",
                                                            "tanh",
                                                            "threshold"};
const static std::vector<std::string> need_nt = {"at_time"};

/**
 * Checks if \c token is one of the functions coming from NEURON/CoreNEURON and needs
 * passing NrnThread* as first argument (typical name of variable \c nt)
 *
 * @param token Name of function
 * @return True or false depending if the function needs NrnThread* argument
 */
bool needs_neuron_thread_first_arg(const std::string& token) {
    return std::find(need_nt.cbegin(), need_nt.cend(), token) != need_nt.cend();
}


/**
 * Variables from NEURON that are directly used in NMODL
 *
 * NEURON exposes certain variable that can be directly used in NMODLvar.
 * The passes like scope checker needs to know if certain variable is
 * undefined and hence these needs to be inserted into symbol table
 */
static std::vector<std::string> const NEURON_VARIABLES =
    {"t", "dt", "celsius", "v", "diam", "area", "pi", "secondorder"};


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
    return details::keywords.find(name) != details::keywords.end();
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
    return details::NEURON_VARIABLES;
}


/**
 * Return functions that can be used in the NMODL
 * @return vector of function names used in NMODL
 */
std::vector<std::string> get_external_functions() {
    std::vector<std::string> result;
    result.reserve(details::methods.size() + details::extern_definitions.size());
    for (auto& method: details::methods) {
        result.push_back(method.first);
    }
    result.insert(result.cend(),
                  details::extern_definitions.begin(),
                  details::extern_definitions.end());
    return result;
}

}  // namespace nmodl
