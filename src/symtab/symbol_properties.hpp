#ifndef SYMBOL_PROPERTIES_HPP
#define SYMBOL_PROPERTIES_HPP

#include <sstream>

#include "flags/flags.hpp"
#include "utils/string_utils.hpp"

namespace symtab {

    namespace details {

        /** kind of symbol */
        enum class DeclarationType {
            /** variable */
            variable,

            /** function */
            function
        };

        /** scope within a mod file */
        enum class Scope {
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
        enum class Status {
            /** converted to local */
            localized = 1 << 0,

            /** converted to global */
            globalized = 1 << 1,

            /** inlined */
            inlined = 1 << 2,

            /** renamed */
            renamed = 1 << 3,

            /** created */
            created = 1 << 4,

            /** derived from state */
            from_state = 1 << 5
        };

        /** usage of mod file as array or scalar */
        enum class VariableType {
            /** scalar / single value */
            scalar,

            /** vector type */
            array
        };

        /** variable usage within a mod file */
        enum class Access {
            /** variable is ready only */
            read = 1 << 0,

            /** variable is written only */
            write = 1 << 1
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
            local_var = 1 << 0,

            /** Global Variable */
            global_var = 1 << 1,

            /** Range Variable */
            range_var = 1 << 2,

            /** Parameter Variable */
            param_assign = 1 << 3,

            /** Pointer Type */
            pointer_var = 1 << 4,

            /** Bbcorepointer Type */
            bbcore_pointer_var = 1 << 5,

            /** Extern Type */
            extern_var = 1 << 6,

            /** Prime Type */
            prime_name = 1 << 7,

            /** Dependent Def */
            dependent_def = 1 << 8,

            /** Unit Def */
            unit_def = 1 << 9,

            /** Read Ion */
            read_ion_var = 1 << 10,

            /** Write Ion */
            write_ion_var = 1 << 11,

            /** Non Specific Current */
            nonspe_cur_var = 1 << 12,

            /** Electrode Current */
            electrode_cur_var = 1 << 13,

            /** Section Type */
            section_var = 1 << 14,

            /** Argument Type */
            argument = 1 << 15,

            /** Function Type */
            function_block = 1 << 16,

            /** Procedure Type */
            procedure_block = 1 << 17,

            /** Derivative Block */
            derivative_block = 1 << 18,

            /** Linear Block */
            linear_block = 1 << 19,

            /** NonLinear Block */
            non_linear_block = 1 << 20,

            /** constant variable */
            constant_var = 1 << 21,

            /** Partial Block */
            partial_block = 1 << 22,

            /** Kinetic Block */
            kinetic_block = 1 << 23,

            /** FunctionTable Block */
            function_table_block = 1 << 24,

            /** factor in unit block */
            factor_def = 1 << 25,

            /** neuron variable accessible in mod file */
            extern_neuron_variable = 1 << 26,

            /** neuron solver methods and math functions */
            extern_method = 1 << 27,

            /** state variable */
            state_var = 1 << 28,

            /** need to solve : used in solve statement */
            to_solve = 1 << 29,

            /** ion type */
            useion = 1 << 30,

            /** variable is converted to thread safe */
            thread_safe = 1 << 31

            /** Discrete Block */
            // discrete_block = 1 << 31,
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
