#ifndef SYMBOL_PROPERTIES_HPP
#define SYMBOL_PROPERTIES_HPP

#include <sstream>

#include "flags/flags.hpp"
#include "utils/string_utils.hpp"

namespace symtab {

    namespace details {

        /** kind of symbol */
        enum class DeclarationType : long long {
            /** variable */
            variable,

            /** function */
            function
        };

        /** scope within a mod file */
        enum class Scope : long long {
            /** local variable */
            local,

            /** global variable */
            global,

            /** neuron variable */
            neuron,

            /** extern variable */
            external
        };

        /** state during various compiler passes */
        enum class Status : long long {
            /** converted to local */
            localized = 1L << 0,

            /** converted to global */
            globalized = 1L << 1,

            /** inlined */
            inlined = 1L << 2,

            /** renamed */
            renamed = 1L << 3,

            /** created */
            created = 1L << 4,

            /** derived from state */
            from_state = 1L << 5,

            /** variable marked as thread safe */
            thread_safe = 1L << 6
        };

        /** usage of mod file as array or scalar */
        enum class VariableType : long long {
            /** scalar / single value */
            scalar,

            /** vector type */
            array
        };

        /** variable usage within a mod file */
        enum class Access : long long {
            /** variable is ready only */
            read = 1L << 0,

            /** variable is written only */
            write = 1L << 1
        };


        /** @brief nmodl variable properties
         *
         * Certain variables/symbols specified in various places in the
         * same mod file. For example, RANGE variable could be in PARAMETER
         * bloc, ASSIGNED block etc. In this case, the symbol in global
         * scope need to have multiple properties. This is also important
         * while code generation. Hence, in addition to AST node pointer,
         * we define NmodlInfo so that we can set multiple properties.
         * Note that AST pointer is no longer pointing to all pointers.
         * Same applies for ModToken.
         *
         * These is some redundancy between NmodlInfo and other enums like
         * DeclarationType and Scope. But as AST will become more abstract
         * from NMODL (towards C/C++) then other types will be useful.
         *
         * \todo Rename param_assign to parameter_var
         * \todo Reaching max limit (31), need to refactor all block types
         *       into separate type
         */
        enum class NmodlInfo : long long {
            /** Local Variable */
            local_var = 1L << 0,

            /** Global Variable */
            global_var = 1L << 1,

            /** Range Variable */
            range_var = 1L << 2,

            /** Parameter Variable */
            param_assign = 1L << 3,

            /** Pointer Type */
            pointer_var = 1L << 4,

            /** Bbcorepointer Type */
            bbcore_pointer_var = 1L << 5,

            /** Extern Type */
            extern_var = 1L << 6,

            /** Prime Type */
            prime_name = 1L << 7,

            /** Dependent Def */
            dependent_def = 1L << 8,

            /** Unit Def */
            unit_def = 1L << 9,

            /** Read Ion */
            read_ion_var = 1L << 10,

            /** Write Ion */
            write_ion_var = 1L << 11,

            /** Non Specific Current */
            nonspe_cur_var = 1L << 12,

            /** Electrode Current */
            electrode_cur_var = 1L << 13,

            /** Section Type */
            section_var = 1L << 14,

            /** Argument Type */
            argument = 1L << 15,

            /** Function Type */
            function_block = 1L << 16,

            /** Procedure Type */
            procedure_block = 1L << 17,

            /** Derivative Block */
            derivative_block = 1L << 18,

            /** Linear Block */
            linear_block = 1L << 19,

            /** NonLinear Block */
            non_linear_block = 1L << 20,

            /** constant variable */
            constant_var = 1L << 21,

            /** Partial Block */
            partial_block = 1L << 22,

            /** Kinetic Block */
            kinetic_block = 1L << 23,

            /** FunctionTable Block */
            function_table_block = 1L << 24,

            /** factor in unit block */
            factor_def = 1L << 25,

            /** neuron variable accessible in mod file */
            extern_neuron_variable = 1L << 26,

            /** neuron solver methods and math functions */
            extern_method = 1L << 27,

            /** state variable */
            state_var = 1L << 28,

            /** need to solve : used in solve statement */
            to_solve = 1L << 29,

            /** ion type */
            useion = 1L << 30,

            /** variable is used in table statement */
            table_statement_var = 1L << 31,

            /** variable is used in table as dependent */
            table_dependent_var = 1L << 32,

            /** Discrete Block */
            discrete_block = 1L << 33
        };

    }  // namespace details

}  // namespace symtab

ALLOW_FLAGS_FOR_ENUM(symtab::details::NmodlInfo)
ALLOW_FLAGS_FOR_ENUM(symtab::details::Access)
ALLOW_FLAGS_FOR_ENUM(symtab::details::Status)

using SymbolInfo = flags::flags<symtab::details::NmodlInfo>;
using SymbolStatus = flags::flags<symtab::details::Status>;

std::ostream& operator<<(std::ostream& os, const SymbolInfo& obj);
std::ostream& operator<<(std::ostream& os, const SymbolStatus& obj);

std::vector<std::string> to_string_vector(const SymbolInfo& obj);
std::vector<std::string> to_string_vector(const SymbolStatus& obj);

template <typename T>
std::string to_string(const T& obj) {
    auto elements = to_string_vector(obj);
    std::string text;
    for (const auto& element : elements) {
        text += element + " ";
    }
    // remove extra whitespace at the end
    stringutils::trim(text);
    return text;
}

#endif
