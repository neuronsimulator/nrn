#pragma once
namespace neuron::container {
/**
 * @brief Base class for neuron::container::soa<...> handles.
 *
 * This provides some common methods that are neither specific to a particular data
 * structure (Node, Mechanism, ...) nor specific to whether or not the handle
 * has owning semantics or not. The typical hierarchy would be:
 *
 * Node::identifier = non_owning_identifier<Node::storage>
 * Node::owning_identifier = owning_identifier<Node::storage>
 * Node::handle = Node::interface<Node::identifier>
 *                inherits from: handle_base<Node::identifier>
 * Node::owning_handle = Node::interface<Node::owning_identifier>
 *                       inherits from handle_base<Node::owning_identifier>
 */
template <typename Identifier>
struct handle_base {
    handle_base(Identifier identifier)
        : m_identifier{std::move(identifier)} {}

    /**
     * @brief Return current offset in the underlying storage where this object lives.
     */
    [[nodiscard]] std::size_t current_row() const {
        return m_identifier.current_row();
    }

    [[nodiscard]] non_owning_identifier_without_container id() const {
        return m_identifier;
    }

    /**
     * @brief This is a workaround for id sometimes being a macro.
     * @todo Remove those macros once and for all.
     */
    [[nodiscard]] auto id_hack() const {
        return id();
    }

    /** @brief Return the (identifier_base-derived) identifier of the pointed-to
     *  object.
     *
     *  @todo In some cases (handle, owning_handle) we already know the
     *  std::size_t* value that is needed, so we should be able to get this more
     *  directly (or add an extra assertion).
     */
    // auto id() const {
    //     auto const tmp = underlying_storage().identifier(derived().offset());
    //     //static_assert(std::is_base_of_v<identifier_base, decltype(tmp)>);
    //     return tmp;
    // }

    auto& underlying_storage() {
        return m_identifier.underlying_storage();
    }
    auto const& underlying_storage() const {
        return m_identifier.underlying_storage();
    }

  protected:
    // [[nodiscard]] View& derived() {
    //     return static_cast<View&>(*this);
    // }
    // [[nodiscard]] View const& derived() const {
    //     return static_cast<View const&>(*this);
    // }
    template <typename Tag>
    [[nodiscard]] auto& get_container() {
        return underlying_storage().template get<Tag>();
    }
    template <typename Tag>
    [[nodiscard]] auto const& get_container() const {
        return underlying_storage().template get<Tag>();
    }
    // TODO const-ness -- should a const view yield data_handle<T const>?
    template <typename Tag>
    [[nodiscard]] auto get_handle() {
        auto const* const_this = this;
        auto const& container = const_this->template get_container<Tag>();
        data_handle<typename Tag::type> const rval{this->id(), container};
        assert(bool{rval});
        assert(rval.refers_to_a_modern_data_structure());
        assert(rval.template refers_to<Tag>(underlying_storage()));
        return rval;
    }
    template <typename Tag>
    [[nodiscard]] auto get_handle(std::size_t field_index) {
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
    [[nodiscard]] auto& get() {
        return underlying_storage().template get<Tag>(current_row());
    }
    template <typename Tag>
    [[nodiscard]] auto const& get() const {
        return underlying_storage().template get<Tag>(current_row());
    }
    template <typename Tag>
    [[nodiscard]] constexpr Tag const& get_tag() const {
        return underlying_storage().template get_tag<Tag>();
    }
    template <typename Tag>
    [[nodiscard]] auto& get(std::size_t field_index) {
        return underlying_storage().template get_field_instance<Tag>(field_index, current_row());
    }
    template <typename Tag>
    [[nodiscard]] auto const& get(std::size_t field_index) const {
        return underlying_storage().template get_field_instance<Tag>(field_index, current_row());
    }

  private:
    Identifier m_identifier;
};

}  // namespace neuron::container
