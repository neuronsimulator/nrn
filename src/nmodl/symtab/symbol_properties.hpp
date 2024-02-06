/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * \file
 * \brief Implement various classes to represent various Symbol properties
 */

#include <sstream>

#include "utils/string_utils.hpp"


namespace nmodl {
namespace symtab {

/// %Symbol information
namespace syminfo {

/// \todo Error with pybind if std::underlying_typ is used
using enum_type = long long;

/// kind of symbol
enum class DeclarationType : enum_type {
    /// variable
    variable,

    /// function
    function
};

/// scope within a mod file
enum class Scope : enum_type {
    /// local variable
    local,

    /// global variable
    global,

    /// neuron variable
    neuron,

    /// extern variable
    external
};

/// state during various compiler passes
enum class Status : enum_type {
    empty = 0,

    /// converted to local
    localized = 1LL << 0,

    /// converted to global
    globalized = 1LL << 1,

    /// inlined
    inlined = 1LL << 2,

    /// renamed
    renamed = 1LL << 3,

    /// created
    created = 1LL << 4,

    /// derived from state
    from_state = 1LL << 5,

    /// variable marked as thread safe
    thread_safe = 1LL << 6
};

/// usage of mod file as array or scalar
enum class VariableType : enum_type {
    /// scalar / single value
    scalar,

    /// vector type
    array
};

/// variable usage within a mod file
enum class Access : enum_type {
    /// variable is ready only
    read = 1LL << 0,

    /// variable is written only
    write = 1LL << 1
};


/**
 * \brief NMODL variable properties
 *
 * Certain variables/symbols specified in various places in the same mod file.
 * For example, RANGE variable could be in PARAMETER bloc, ASSIGNED block etc.
 * In this case, the symbol in global scope need to have multiple properties.
 * This is also important while code generation. Hence, in addition to AST node
 * pointer, we define NmodlType so that we can set multiple properties.  Note
 * that AST pointer is no longer pointing to all pointers.  Same applies for
 * ModToken.
 *
 * These is some redundancy between NmodlType and other enums like
 * syminfo::DeclarationType and syminfo::Scope. But as AST will become more
 * abstract from NMODL (towards C/C++) then other types will be useful.
 *
 * \todo
 *   - Rename param_assign to parameter_var
 */
enum class NmodlType : enum_type {

    empty = 0,

    /// Local Variable
    local_var = 1LL << 0,

    /// Global Variable
    global_var = 1LL << 1,

    /// Range Variable
    range_var = 1LL << 2,

    /// Parameter Variable
    param_assign = 1LL << 3,

    /// Pointer Type
    pointer_var = 1LL << 4,

    /// Bbcorepointer Type
    bbcore_pointer_var = 1LL << 5,

    /// Extern Type
    extern_var = 1LL << 6,

    /// Prime Type
    prime_name = 1LL << 7,

    /// Assigned Definition
    assigned_definition = 1LL << 8,

    /// Unit Def
    unit_def = 1LL << 9,

    /// Read Ion
    read_ion_var = 1LL << 10,

    /// Write Ion
    write_ion_var = 1LL << 11,

    /// Non Specific Current
    nonspecific_cur_var = 1LL << 12,

    /// Electrode Current
    electrode_cur_var = 1LL << 13,

    /// Argument Type
    argument = 1LL << 14,

    /// Function Type
    function_block = 1LL << 15,

    /// Procedure Type
    procedure_block = 1LL << 16,

    /// Derivative Block
    derivative_block = 1LL << 17,

    /// Linear Block
    linear_block = 1LL << 18,

    /// NonLinear Block
    non_linear_block = 1LL << 19,

    /// constant variable
    constant_var = 1LL << 20,

    /// Kinetic Block
    kinetic_block = 1LL << 21,

    /// FunctionTable Block
    function_table_block = 1LL << 22,

    /// factor in unit block
    factor_def = 1LL << 23,

    /// neuron variable accessible in mod file
    extern_neuron_variable = 1LL << 24,

    /// neuron solver methods and math functions
    extern_method = 1LL << 25,

    /// state variable
    state_var = 1LL << 26,

    /// need to solve : used in solve statement
    to_solve = 1LL << 27,

    /// ion type
    useion = 1LL << 28,

    /// variable is used in table statement
    table_statement_var = 1LL << 29,

    /// variable is used in table as assigned
    table_assigned_var = 1LL << 30,

    /// Discrete Block
    discrete_block = 1LL << 31,

    /// Define variable / macro
    define = 1LL << 32,

    /// Codegen specific variable
    codegen_var = 1LL << 33
};

template <typename T>
inline T operator~(T arg) {
    using utype = enum_type;
    return static_cast<T>(~static_cast<utype>(arg));
}

template <typename T>
inline T operator|(T lhs, T rhs) {
    using utype = enum_type;
    return static_cast<T>(static_cast<utype>(lhs) | static_cast<utype>(rhs));
}

template <typename T>
inline T operator&(T lhs, T rhs) {
    using utype = enum_type;
    return static_cast<T>(static_cast<utype>(lhs) & static_cast<utype>(rhs));
}

template <typename T>
inline T& operator|=(T& lhs, T rhs) {
    lhs = lhs | rhs;
    return lhs;
}

template <typename T>
inline T& operator&=(T& lhs, T rhs) {
    lhs = lhs & rhs;
    return lhs;
}


/// check if any property is set
inline bool has_property(const NmodlType& obj, NmodlType property) {
    return static_cast<bool>(obj & property);
}

/// check if any status is set
inline bool has_status(const Status& obj, Status state) {
    return static_cast<bool>(obj & state);
}

std::ostream& operator<<(std::ostream& os, const syminfo::NmodlType& obj);
std::ostream& operator<<(std::ostream& os, const syminfo::Status& obj);

/// helper function to convert nmodl properties to string
std::vector<std::string> to_string_vector(const syminfo::NmodlType& obj);

/// helper function to convert symbol status to string
std::vector<std::string> to_string_vector(const syminfo::Status& obj);

template <typename T>
std::string to_string(const T& obj) {
    std::string text;
    bool is_first{true};
    for (const auto& element: to_string_vector(obj)) {
        if (is_first) {
            text += element;
            is_first = false;
        } else {
            text += " " + element;
        }
    }
    return text;
}

}  // namespace syminfo
}  // namespace symtab
}  // namespace nmodl
