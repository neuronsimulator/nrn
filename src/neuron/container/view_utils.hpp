#pragma once
namespace neuron::container {
/** @brief Base class for neuron::container::soa<...> views/handles.
 *
 *  This provides some common methods that are not specific to a particular data
 *  structure (Node, ...). The typical hierarchy would be:
 *
 *  view_base <--- Node::interface <--- Node::owning_handle
 *             \                    \-- Node::handle
 *              \                    \- Node::view
 *               \- Foo::interface <--- Foo::owning_handle
 *                                  \-- Foo::handle
 *                                   \- Foo::view
 *
 *  Where the grandchild types (Node::handle, etc.) are expected to implement
 *  the interface:
 *   - underlying_storage(): return a (const) reference to the underlying
 *     storage container (Node::storage, etc.)
 *   - std::size_t offset(): return the current offset into the underlying
 *     storage container
 */
template <typename View>
struct view_base {
    /** @brief Return the (identifier_base-derived) identifier of the pointed-to
     *  object.
     *
     *  @todo In some cases (handle, owning_handle) we already know the
     *  std::size_t* value that is needed, so we should be able to get this more
     *  directly (or add an extra assertion).
     */
    auto id() const {
        auto const tmp = derived().underlying_storage().identifier(derived().offset());
        static_assert(std::is_base_of_v<identifier_base, decltype(tmp)>);
        return tmp;
    }

  protected:
    View& derived() {
        return static_cast<View&>(*this);
    }
    View const& derived() const {
        return static_cast<View const&>(*this);
    }
    auto& underlying_storage() {
        return derived().underlying_storage();
    }
    auto const& underlying_storage() const {
        return derived().underlying_storage();
    }
    template <typename Tag>
    auto& get_container() {
        return underlying_storage().template get<Tag>();
    }
    template <typename Tag>
    auto const& get_container() const {
        return underlying_storage().template get<Tag>();
    }
    // TODO const-ness -- should a const view yield data_handle<T const>?
    template <typename Tag>
    auto get_handle() {
        auto const* const_this = this;
        auto const& container = const_this->template get_container<Tag>();
        data_handle<typename Tag::type> const rval{this->id(), container};
        assert(bool{rval});
        assert(rval.refers_to_a_modern_data_structure());
        assert(rval.template refers_to<Tag>(underlying_storage()));
        return rval;
    }
    template <typename Tag>
    auto get_handle(std::size_t field_index) {
        auto const* const_this = this;
        data_handle<typename Tag::type> const rval{
            this->id(),
            const_this->underlying_storage().template get_field_instance<Tag>(field_index)};
        assert(bool{rval});
        assert(rval.refers_to_a_modern_data_structure());
        // assert(rval.template refers_to<Tag>(derived().underlying_storage()));
        return rval;
    }
    template <typename Tag>
    auto& get() {
        return underlying_storage().template get<Tag>(derived().offset());
    }
    template <typename Tag>
    auto const& get() const {
        return underlying_storage().template get<Tag>(derived().offset());
    }
    template <typename Tag>
    Tag const& get_tag() const {
        return underlying_storage().template get_tag<Tag>();
    }
    template <typename Tag>
    auto& get(std::size_t field_index) {
        return underlying_storage().template get_field_instance<Tag>(field_index,
                                                                     derived().offset());
    }
    template <typename Tag>
    auto const& get(std::size_t field_index) const {
        return underlying_storage().template get_field_instance<Tag>(field_index,
                                                                     derived().offset());
    }
};

}  // namespace neuron::container
