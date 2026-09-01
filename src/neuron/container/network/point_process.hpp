#pragma once
/**
 * @file network/point_process.hpp
 * @brief SoA storage for integration-hot Point_process fields.
 *
 * CoreNEURON reference: coreneuron::Point_process {_i_instance, _type, _tid}.
 * Design: doc/network-soa-phase0.md §5.2.
 *
 * HOC location / Object* / Prop* remain on the legacy Point_process shell or
 * sidecars during dual-write (Phase 1 does not replace interpreter objects).
 */
#include "neuron/container/data_handle.hpp"
#include "neuron/container/soa_container.hpp"
#include "neuron/container/view_utils.hpp"

#include <ostream>
#include <string_view>

namespace neuron::container::network::PointProcess {
namespace field {

/** @brief Mechanism SoA instance row (_i_instance). */
struct Instance {
    using type = int;
    constexpr type default_value() const {
        return -1;
    }
};

/** @brief Mechanism type (_type). */
struct MechType {
    using type = int;
    constexpr type default_value() const {
        return -1;
    }
};

/** @brief Owning NrnThread id (_tid). */
struct ThreadId {
    using type = int;
    constexpr type default_value() const {
        return -1;
    }
};

}  // namespace field

/**
 * @brief Public API for PointProcess handles (owning and non-owning).
 * @tparam Identifier owning_identifier or non_owning_identifier for storage.
 */
template <typename Identifier>
struct handle_interface: handle_base<Identifier> {
    using base_type = handle_base<Identifier>;
    using base_type::base_type;

    [[nodiscard]] field::Instance::type& instance() {
        return this->template get<field::Instance>();
    }
    [[nodiscard]] field::Instance::type const& instance() const {
        return this->template get<field::Instance>();
    }
    [[nodiscard]] data_handle<field::Instance::type> instance_handle() {
        return this->template get_handle<field::Instance>();
    }

    [[nodiscard]] field::MechType::type& mech_type() {
        return this->template get<field::MechType>();
    }
    [[nodiscard]] field::MechType::type const& mech_type() const {
        return this->template get<field::MechType>();
    }
    [[nodiscard]] data_handle<field::MechType::type> mech_type_handle() {
        return this->template get_handle<field::MechType>();
    }

    [[nodiscard]] field::ThreadId::type& thread_id() {
        return this->template get<field::ThreadId>();
    }
    [[nodiscard]] field::ThreadId::type const& thread_id() const {
        return this->template get<field::ThreadId>();
    }
    [[nodiscard]] data_handle<field::ThreadId::type> thread_id_handle() {
        return this->template get_handle<field::ThreadId>();
    }

    friend std::ostream& operator<<(std::ostream& os, handle_interface const& handle) {
        if (handle.id()) {
            return os << "PointProcess{" << handle.id() << '/' << handle.underlying_storage().size()
                      << " instance=" << handle.instance() << " mech_type=" << handle.mech_type()
                      << " thread_id=" << handle.thread_id() << '}';
        }
        return os << "PointProcess{null}";
    }
};

/** @brief Underlying storage for all PointProcess integration rows. */
struct storage: soa<storage, field::Instance, field::MechType, field::ThreadId> {
    [[nodiscard]] std::string_view name() const {
        return "network::PointProcess";
    }
};

/** @brief Non-owning handle; stable across permute; invalid after owner dies. */
using handle = handle_interface<non_owning_identifier<storage>>;

/** @brief Owning handle; destroys the storage row on destruction. */
struct owning_handle: handle_interface<owning_identifier<storage>> {
    using base_type = handle_interface<owning_identifier<storage>>;
    using base_type::base_type;

    [[nodiscard]] handle non_owning_handle() {
        return non_owning_identifier<storage>{&underlying_storage(), id()};
    }
};
}  // namespace neuron::container::network::PointProcess
