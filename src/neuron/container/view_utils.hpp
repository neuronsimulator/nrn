#pragma once
#include <cstddef>  // std::size_t
#include <utility>  // std::as_const, std::move

namespace neuron::container {
/**
 * @brief Base class for neuron::container::soa<...> handles.
 * @tparam Identifier Identifier type used for this handle. This encodes both
 *                    the referred-to type (Node, Mechanism, ...) and the
 *                    ownership semantics (owning, non-owning). The instance of
 *                    this type manages both the pointer-to-row and
 *                    pointer-to-storage members of the handle.
 *
 * This provides some common methods that are neither specific to a particular data
 * structure (Node, Mechanism, ...) nor specific to whether or not the handle
 * has owning semantics or not. Methods that are specific to the data type (e.g.
 * Node) belong in the interface template for that type (e.g. Node::interface).
 * Methods that are specific to the owning/non-owning semantics belong in the
 * generic templates non_owning_identifier<T> and owning_identifier<T>.
 *
 * The typical way these components fit together is:
 *
 * Node::identifier = non_owning_identifier<Node::storage>
 * Node::owning_identifier = owning_identifier<Node::storage>
 * Node::handle = Node::interface<Node::identifier>
 *                inherits from: handle_base<Node::identifier>
 * Node::owning_handle = Node::interface<Node::owning_identifier>
 *                       inherits from handle_base<Node::owning_identifier>
 *
 * Where the "identifier" types should be viewed as an implementation detail and
 * the handle types as user-facing.
 */
template <typename Identifier>
struct handle_base {
    /**
     * @brief Construct a handle from an identifier.
     */
    handle_base(Identifier identifier)
        : m_identifier{std::move(identifier)} {}

    /**
     * @brief Return current offset in the underlying storage where this object lives.
     */
    [[nodiscard]] std::size_t current_row() const {
        return m_identifier.current_row();
    }

    /**
     * @brief Obtain a lightweight identifier of the current entry.
     *
     * The return type is essentially std::size_t* -- it does not contain a
     * pointer/reference to the actual storage container.
     */
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

    /**
     * @brief Obtain a reference to the storage this handle refers to.
     */
    auto& underlying_storage() {
        return m_identifier.underlying_storage();
    }

    /**
     * @brief Obtain a const reference to the storage this handle refers to.
     */
    auto const& underlying_storage() const {
        return m_identifier.underlying_storage();
    }

  protected:
    /**
     * @brief Get a data_handle<T> referring to the given field inside this handle.
     * @tparam Tag Tag type of the field we want a data_handle to.
     *
     * This is used to implement methods like area_handle() and v_handle() in
     * the interface templates.
     *
     * @todo const cleanup -- should there be a const version returning
     *       data_handle<T const>?
     */
    template <typename Tag>
    [[nodiscard]] auto get_handle() {
        return underlying_storage().template get_handle<Tag>(this->id());
    }

    /**
     * @brief Get a data_handle<T> referring to the (runtime) field_index-th
     *        copy of a given (static) field.
     * @tparam Tag Tag type of the set of fields the from which the
     *             field_index-th one is being requested.

     * @todo Const cleanup as above for the zero-argument version.
     */
    template <typename Tag>
    [[nodiscard]] auto get_handle(int field_index, int array_offset = 0) {
        return underlying_storage().template get_field_instance_handle<Tag>(this->id(),
                                                                            field_index,
                                                                            array_offset);
    }
    template <typename Tag>
    [[nodiscard]] auto& get() {
        return underlying_storage().template get<Tag>(current_row());
    }
    template <typename Tag>
    [[nodiscard]] auto const& get() const {
        return underlying_storage().template get<Tag>(current_row());
    }
    /**
     * @brief Get the instance of the given tag type from underlying storage.
     * @tparam Tag The tag type.
     * @return Const reference to the given tag type instance inside the ~global storage struct.
     *
     * This is a thin wrapper that calls @c soa::get_tag<Tag> on the storage container (currently an
     * instance of @c Node::storage or @c Mechanism::storage) that this handle refers to an instance
     * inside.
     */
    template <typename Tag>
    [[nodiscard]] constexpr Tag const& get_tag() const {
        return underlying_storage().template get_tag<Tag>();
    }
    template <typename Tag>
    [[nodiscard]] auto& get(int field_index, int array_offset = 0) {
        return underlying_storage().template get_field_instance<Tag>(current_row(),
                                                                     field_index,
                                                                     array_offset);
    }
    template <typename Tag>
    [[nodiscard]] auto const& get(int field_index, int array_offset = 0) const {
        return underlying_storage().template get_field_instance<Tag>(current_row(),
                                                                     field_index,
                                                                     array_offset);
    }

  private:
    Identifier m_identifier;
};
}  // namespace neuron::container
