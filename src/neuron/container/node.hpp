#pragma once
#include "neuron/model_data.hpp"
#include "neuron/container/node_data.hpp"

#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>

namespace neuron::container::Node {
/** @brief CRTP base class defining the public API of Node and NodeView.
 *  @tparam Derived   Derived class type, for CRTP.
 *  @tparam NodeView  This is a cheap workaround for parent().
 *
 *  This allows the same struct-like accessors (v(), v_ref(), set_v(), ...) to be
 *  used both on "owning" Node objects and "non-owning" NodeView objects.
 */
template <typename Derived, typename NodeView>
struct interface {
    field::Voltage::type v() const {
        return get<field::Voltage>();
    }
    field::Voltage::type& v_ref() {
        return get<field::Voltage>();
    }
    field::Voltage::type const& v_ref() const {
        return get<field::Voltage>();
    }
    void set_v(field::Voltage::type v) {
        get<field::Voltage>() = v;
    }
    // TODO: could this go in some neuron::container::interface::base class?
    identifier id() const {
        return derived().node_data().identifier(derived().offset());
    }

  private:
    // TODO: could the rest go in some neuron::container::interface::base class?
    Derived const& derived() const {
        return static_cast<Derived const&>(*this);
    }
    template <typename Tag>
    auto& get() {
        return derived().node_data().template get<Tag>(derived().offset());
    }
    template <typename Tag>
    auto const& get() const {
        return derived().node_data().template get<Tag>(derived().offset());
    }
};

// Reconsider the name. At the moment a handle is an owning thing that can have
// a long lifetime, while a view is a non-owning thing that may only be used
// transiently.
struct handle;

// These must be transient, they are like Node& and can be left dangling if
// the real Node is deleted or a permute operation is performed
// on the underlying data.
struct view: interface<view, view> {
    inline view(handle const&);
    view(storage& node_data, identifier const& id)
        : m_row{id.current_row()}
        , m_node_data{node_data} {
        assert(m_row != invalid_row);
    }

  private:
    friend struct interface<view, view>;
    std::size_t m_row;
    std::reference_wrapper<storage> m_node_data;
    // Interface for CRTP base class.
    std::size_t offset() const {
        return m_row;
    }
    storage& node_data() const {
        return m_node_data;
    }
};

// TODO: what are the pitfalls of rebinding a Node to a different NodeData?
struct handle: interface<handle, view> {
    /** @brief Create a new Node at the end of `node_data`.
     */
    handle(storage& node_data = data)
        : m_node_data_offset{node_data} {
        node_data.emplace_back(m_node_data_offset);
    }
    handle(handle&&) = default;
    handle(handle const&) = delete;  // should be done(?)
    handle& operator=(handle&&) = default;
    handle& operator=(handle const&) = delete;  // should be done(?)
    ~handle() = default;
    void swap(handle& other) noexcept {
        m_node_data_offset.swap(other.m_node_data_offset);
    }
    /** Create a new Node that is a clone of this one.
     *  @todo Drop this if we make Node copiable.
     */
    // Node clone() const {
    //   Node new_node{node_data()};
    //   new_node.set_a(this->a());
    //   new_node.set_b(this->b());
    //   new_node.set_parent_id(this->id());
    //   auto const& current_node_mechs = this->mechanisms();
    //   auto& new_node_mechs = new_node.mechanisms();
    //   new_node_mechs.reserve(current_node_mechs.size());
    //   for (auto const& mech : current_node_mechs) {
    //     new_node.insert(mech.type());
    //   }
    //   return new_node;
    // }

  private:
    friend struct view;
    friend struct interface<handle, view>;
    OwningElementHandle<storage, identifier> m_node_data_offset;
    // Interface for CRTP base class.
    storage& node_data() const {
        return m_node_data_offset.data_container();
    }
    std::size_t offset() const {
        return m_node_data_offset.current_row();
    }
};

inline view::view(handle const& node)
    : m_row{node.offset()}
    , m_node_data{node.node_data()} {}
}  // namespace neuron::container::Node
