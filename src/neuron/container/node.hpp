#pragma once
#include "neuron/model_data.hpp"
#include "neuron/container/data_handle.hpp"
#include "neuron/container/node_data.hpp"
#include "neuron/container/view_utils.hpp"

#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>

namespace neuron::container::Node {
/** @brief Base class defining the public API of Node handles/views.
 *  @tparam View The concrete [owning_]handle/view type.
 *
 *  This allows the same struct-like accessors (v(), set_v(), ...) to be
 *  used on all of the different types of objects that represent a single Node:
 *  - owning_handle: stable over permutations of underlying data, manages
 *    lifetime of a row in the underlying storage. Can be null.
 *  - handle: stable over permutations of underlying data, produces runtime
 *    error if it is dereferenced after the corresponding owning_handle has gone
 *    out of scope. Can be null.
 *  - view: not stable over permutations of underlying data, must not be
 *    dereferenced after the corresponding owning_handle has gone out of scope.
 *    Cannot be null.
 *
 *  @todo Discuss and improve the three names above.
 */
template <typename View>
struct interface: view_base<View> {
    /** @brief Return the membrane potential.
     *
     *  Note that in translated MOD files this gets included with v redefined to
     *  _v.
     */
    [[nodiscard]] field::Voltage::type v() const {
        return this->template get<field::Voltage>();
    }

    /** Return a generic handle to a value (double in this case) that is stable
     *  over permutations but doesn't know that it is a Node voltage.
     */
    [[nodiscard]] data_handle<field::Voltage::type> v_handle() {
        return {this->id(), this->template get_container<field::Voltage>()};
    }

    /** @brief Set the membrane potentials.
     */
    void set_v(field::Voltage::type v) {
        this->template get<field::Voltage>() = v;
    }
};

// Reconsider the name. At the moment an [owning_]handle is an (owning) thing that can have
// a long lifetime, while a view is a non-owning thing that may only be used
// transiently.
struct owning_handle;

// These must be transient, they are like Node& and can be left dangling if
// the real Node is deleted or a permute operation is performed
// on the underlying data.
struct view: interface<view> {
    inline view(owning_handle const&);
    view(storage& node_data, identifier const& id)
        : m_row{id.current_row()}
        , m_node_data{node_data} {
        assert(m_row != detail::invalid_row);
    }

  private:
    std::size_t m_row;
    std::reference_wrapper<storage> m_node_data;
    // Interface for neuron::container::view_base
    friend struct view_base<view>;
    std::size_t offset() const {
        return m_row;
    }
    storage& underlying_storage() const {
        return m_node_data;
    }
};

struct handle: interface<handle> {
    handle(identifier id = {}, storage& storage = neuron::model().node_data())
        : m_id{std::move(id)}
        , m_node_data{storage} {}

    operator bool() const {
        return bool{m_id};
    }

  private:
    // Interface for neuron::container::view_base
    storage& underlying_storage() const {
        return m_node_data;
    }
    std::size_t offset() const {
        return m_id.current_row();
    }
    friend struct view_base<handle>;
    identifier m_id;
    std::reference_wrapper<storage> m_node_data;
};

// TODO: what are the pitfalls of rebinding a Node::owning_handle to a different Node::storage?
struct owning_handle: interface<owning_handle> {
    /** @brief Create a new Node at the end of `node_data`.
     *  @todo  For Node, probably just assume that all Nodes belong to the same
     *  neuron::model().node_data() global structure and don't bother holding a
     *  reference to it. This will probably be different for other types of data.
     */
    owning_handle(storage& node_data = neuron::model().node_data())
        : m_node_data_offset{node_data} {
        node_data.emplace_back(m_node_data_offset);
    }
    owning_handle(owning_handle&&) = default;
    owning_handle(owning_handle const&) = delete;  // should be done(?)
    owning_handle& operator=(owning_handle&&) = default;
    owning_handle& operator=(owning_handle const&) = delete;  // should be done(?)
    ~owning_handle() = default;
    void swap(owning_handle& other) noexcept {
        m_node_data_offset.swap(other.m_node_data_offset);
    }
    operator bool() const {
        return bool{m_node_data_offset};
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
    friend struct view_base<owning_handle>;
    owning_identifier_base<storage, identifier> m_node_data_offset;
    // Interface for neuron::container::view_base
    storage& underlying_storage() const {
        return m_node_data_offset.data_container();
    }
    std::size_t offset() const {
        return m_node_data_offset.current_row();
    }
};

inline view::view(owning_handle const& node)
    : m_row{node.offset()}
    , m_node_data{node.underlying_storage()} {}
}  // namespace neuron::container::Node
