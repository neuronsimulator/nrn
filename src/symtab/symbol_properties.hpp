#ifndef _SYMBOL_PROPERTIES_HPP_
#define _SYMBOL_PROPERTIES_HPP_

#include <sstream>

#include "flags/flags.hpp"

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
            /** unmodified */
            unmodified,

            /** converted to local */
            to_local,

            /** converted to global */
            to_global,

            /** inlined */
            inlined,

            /** renamed */
            renamed
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
         */
        enum class NmodlInfo {
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

            /** Discrete Block */
            discrete_block = 1 << 21,

            /** Partial Block */
            partial_block = 1 << 22,

            /** Kinetic Block */
            kinetic_block = 1 << 23,

            /** FunctionTable Block */
            function_table_block = 1 << 24,

            /** factor in unit block */
            factor_def = 1 << 25,

            /** extern token */
            extern_token = 1 << 26
        };

    }  // namespace details

}  // namespace symtab

ALLOW_FLAGS_FOR_ENUM(symtab::details::NmodlInfo)
ALLOW_FLAGS_FOR_ENUM(symtab::details::Access)

using SymbolInfo = flags::flags<symtab::details::NmodlInfo>;

std::ostream& operator<<(std::ostream& os, const SymbolInfo& obj);

std::string to_string(const SymbolInfo& obj);

#endif
