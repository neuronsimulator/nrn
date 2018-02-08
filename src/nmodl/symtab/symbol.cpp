#include "symtab/symbol.hpp"

namespace symtab {

    /** if symbol has only extern_token property : this is true
     *  for symbols which are external variables from neuron
     *  like v, t, dt etc.
     * \todo: check if we should check two properties using has_property
     *        instead of exact comparisons
     */
    bool Symbol::is_external_symbol_only() {
        return (properties == details::NmodlInfo::extern_neuron_variable ||
                properties == details::NmodlInfo::extern_method);
    }

    /// check if symbol has any of the common properties
    /// \todo : rename to has_any_property
    bool Symbol::has_properties(SymbolInfo new_properties) {
        return static_cast<bool>(properties & new_properties);
    }

    /// check if symbol has any of the status
    bool Symbol::has_any_status(SymbolStatus new_status) {
        return static_cast<bool>(status & new_status);
    }

    /// add new properties to symbol with bitwise or
    void Symbol::combine_properties(SymbolInfo new_properties) {
        properties |= new_properties;
    }

    void Symbol::add_property(NmodlInfo property) {
        properties |= property;
    }

    void Symbol::add_property(SymbolInfo property) {
        properties |= property;
    }

    /** Prime variable will appear in different block and could have
     *  multiple derivative orders. We have to store highest order.
     *
     * \todo Check if analysis passes need more information.
     */
    void Symbol::set_order(int new_order) {
        if (new_order > order) {
            order = new_order;
        }
    }

    /// check if symbol is of variable type in nmodl
    bool Symbol::is_variable() {
        // clang-format off
        SymbolInfo var_properties = NmodlInfo::local_var
                                    | NmodlInfo::global_var
                                    | NmodlInfo::range_var
                                    | NmodlInfo::param_assign
                                    | NmodlInfo::pointer_var
                                    | NmodlInfo::bbcore_pointer_var
                                    | NmodlInfo::extern_var
                                    | NmodlInfo::dependent_def
                                    | NmodlInfo::read_ion_var
                                    | NmodlInfo::write_ion_var
                                    | NmodlInfo::nonspe_cur_var
                                    | NmodlInfo::electrode_cur_var
                                    | NmodlInfo::section_var
                                    | NmodlInfo::argument
                                    | NmodlInfo::extern_neuron_variable;
        // clang-format on
        return has_properties(var_properties);
    }

}  // namespace symtab
