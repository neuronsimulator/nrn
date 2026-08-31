#pragma once
/**
 * @file network/weights.hpp
 * @brief SoA storage for the flat NetCon weight pool.
 *
 * CoreNEURON reference: NrnThread::weights[n_weight]; NetCon.u.weight_index_ base.
 * Design: doc/network-soa-phase0.md §5.3.
 *
 * Contiguity: a NetCon with weight_count = K owns K consecutive rows.
 * Pack/repack at sort time; WeightIndex on NetCon is rewritten after permute.
 */
#include "neuron/container/data_handle.hpp"
#include "neuron/container/soa_container.hpp"
#include "neuron/container/view_utils.hpp"

#include <ostream>
#include <string_view>

namespace neuron::container::network::Weight {
namespace field {

/** @brief One scalar weight entry in the flat pool. */
struct Value {
    using type = double;
    constexpr type default_value() const {
        return 0.;
    }
};

}  // namespace field

/**
 * @brief Public API for Weight handles (owning and non-owning).
 * @tparam Identifier owning_identifier or non_owning_identifier for storage.
 */
template <typename Identifier>
struct handle_interface: handle_base<Identifier> {
    using base_type = handle_base<Identifier>;
    using base_type::base_type;

    [[nodiscard]] field::Value::type& value() {
        return this->template get<field::Value>();
    }
    [[nodiscard]] field::Value::type const& value() const {
        return this->template get<field::Value>();
    }
    [[nodiscard]] data_handle<field::Value::type> value_handle() {
        return this->template get_handle<field::Value>();
    }

    friend std::ostream& operator<<(std::ostream& os, handle_interface const& handle) {
        if (handle.id()) {
            return os << "Weight{" << handle.id() << '/' << handle.underlying_storage().size()
                      << " value=" << handle.value() << '}';
        }
        return os << "Weight{null}";
    }
};

/** @brief Underlying storage for all weight scalars. */
struct storage: soa<storage, field::Value> {
    [[nodiscard]] std::string_view name() const {
        return "network::Weight";
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
}  // namespace neuron::container::network::Weight
