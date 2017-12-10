#include "symtab/symbol.hpp"

namespace symtab {

    bool Symbol::is_extern_token_only() {
        return (properties == details::NmodlInfo::extern_token);
    }

    bool Symbol::has_common_properties(SymbolInfo& new_properties) {
        return static_cast<bool>(properties & new_properties);
    }

    void Symbol::combine_properties(SymbolInfo& new_properties) {
        properties = properties | new_properties;
    }

    void Symbol::add_property(NmodlInfo property) {
        properties = properties | property;
    }

    void Symbol::add_property(SymbolInfo& property) {
        properties = properties | property;
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