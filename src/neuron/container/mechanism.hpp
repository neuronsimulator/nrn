#pragma once
#include "neuron/model_data.hpp"
#include "neuron/container/data_handle.hpp"
#include "neuron/container/mechanism_data.hpp"
#include "neuron/container/view_utils.hpp"

#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>

namespace neuron::container::Mechanism {
/** @brief Base class defining the public API of Mechanism handles/views.
 *  @tparam View The concrete [owning_]handle/view type.
 *
 *  This allows the same struct-like accessors to be used on all of the
 *  different types of objects that represent a single instance of a particular
 *  Mechanism:
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
    /** @brief Return the area.
     */
    [[nodiscard]] field::PerInstanceFloatingPointField::type fpfield(std::size_t field_index) const {
        if (field_index >= this->template get_tag<field::PerInstanceFloatingPointField>().num_instances()) {
            throw std::runtime_error("Mechanism::fpfield(" + std::to_string(field_index) + ") field index out of range");
        }
        return this->template get<field::PerInstanceFloatingPointField>(field_index);
    }

    /** @brief Return a data_handle to the area.
     */
    [[nodiscard]] data_handle<field::PerInstanceFloatingPointField::type> fpfield_handle(std::size_t field_index) {
        if (field_index >= this->template get_tag<field::PerInstanceFloatingPointField>().num_instances()) {
            throw std::runtime_error("Mechanism::fpfield(" + std::to_string(field_index) + ") field index out of range");
        }
        return this->template get_handle<field::PerInstanceFloatingPointField>(field_index);
    }

    /** @brief Set the area.
     */
    void set_fpfield(std::size_t field_index, field::PerInstanceFloatingPointField::type area) {
      if (field_index >= this->template get_tag<field::PerInstanceFloatingPointField>().num_instances()) {
            throw std::runtime_error("Mechanism::fpfield(" + std::to_string(field_index) + ") field index out of range");
        }
        this->template get<field::PerInstanceFloatingPointField>(field_index) = area;
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
    inline view(owning_handle&);
    view(storage& mech_data, identifier const& id)
        : m_row{id.current_row()}
        , m_mech_data{mech_data} {
        assert(m_row != detail::invalid_row);
    }

  private:
    std::size_t m_row;
    std::reference_wrapper<storage> m_mech_data;
    // Interface for neuron::container::view_base
    friend struct view_base<view>;
    std::size_t offset() const {
        return m_row;
    }
    storage& underlying_storage() {
        return m_mech_data;
    }
    storage const& underlying_storage() const {
        return m_mech_data;
    }
};

struct handle: interface<handle> {
    handle(identifier id, storage& storage)
        : m_id{std::move(id)}
        , m_storage{storage} {}

    operator bool() const {
        return bool{m_id};
    }

  private:
    // Interface for neuron::container::view_base
    storage& underlying_storage() {
        return m_storage;
    }
    storage const& underlying_storage() const {
        return m_storage;
    }
    std::size_t offset() const {
        return m_id.current_row();
    }
    friend struct view_base<handle>;
    identifier m_id;
    std::reference_wrapper<storage> m_storage;
};

struct owning_handle: interface<owning_handle> {
    /** @brief Create a new Node at the end of `node_data`.
     *  @todo  For Node, probably just assume that all Nodes belong to the same
     *  neuron::model().node_data() global structure and don't bother holding a
     *  reference to it. This will probably be different for other types of data.
     */
    owning_handle(storage& mech_data) : m_mech_data_offset{mech_data.emplace_back()} {}
    owning_handle(owning_handle&&) = default;
    owning_handle(owning_handle const&) = delete;  // should be done(?)
    owning_handle& operator=(owning_handle&&) = default;
    owning_handle& operator=(owning_handle const&) = delete;  // should be done(?)
    ~owning_handle() = default;
    void swap(owning_handle& other) noexcept {
        m_mech_data_offset.swap(other.m_mech_data_offset);
    }
    operator bool() const {
        return bool{m_mech_data_offset};
    }

  private:
    friend struct view;
    friend struct view_base<owning_handle>;
    owning_identifier m_mech_data_offset;
    // Interface for neuron::container::view_base
    storage& underlying_storage() {
        return m_mech_data_offset.data_container();
    }
    storage const& underlying_storage() const {
        return m_mech_data_offset.data_container();
    }
    std::size_t offset() const {
        return m_mech_data_offset.current_row();
    }
};

inline view::view(owning_handle& mech_instance)
    : m_row{mech_instance.offset()}
    , m_mech_data{mech_instance.underlying_storage()} {}
}  // namespace neuron::container::Mechanism
