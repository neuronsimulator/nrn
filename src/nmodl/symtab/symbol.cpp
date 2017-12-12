#include "symtab/symbol.hpp"

namespace symtab {

    /// if symbol has only extern_token property : this is true
    /// for symbols which are external variables from neuron
    /// like v, t, dt etc.
    bool Symbol::is_extern_token_only() {
        return (properties == details::NmodlInfo::extern_token);
    }

    /// check if symbol has any of the common properties
    bool Symbol::has_properties(SymbolInfo& new_properties) {
        return static_cast<bool>(properties & new_properties);
    }

    /// add new properties to symbol with bitwise or
    void Symbol::combine_properties(SymbolInfo& new_properties) {
        properties |= new_properties;
    }

    void Symbol::add_property(NmodlInfo property) {
        properties |= property;
    }

    void Symbol::add_property(SymbolInfo& property) {
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

}  // namespace symtab