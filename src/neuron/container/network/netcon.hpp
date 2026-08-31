#pragma once
/**
 * @file network/netcon.hpp
 * @brief SoA storage for integration-hot NetCon fields.
 *
 * CoreNEURON reference: target_, u.weight_index_, delay_, active_.
 * Design: doc/network-soa-phase0.md §5.4, §5.4.1.
 *
 * Phase 2 / heap-free step 1: C++ NetCon keeps DiscreteEvent + pointers; this
 * SoA holds CoreNEURON-shaped columns. WeightIndex/WeightCount are authority for
 * the weight block base; HOC weight() steers via the off-shell WeightBlock.
 */
#include "neuron/container/data_handle.hpp"
#include "neuron/container/network/indices.hpp"
#include "neuron/container/soa_container.hpp"
#include "neuron/container/view_utils.hpp"

#include <ostream>
#include <string_view>

namespace neuron::container::network::NetCon {
namespace field {

/** @brief Row in PointProcess storage (CoreNEURON target). */
struct Target {
    using type = int;
    constexpr type default_value() const {
        return -1;
    }
};

/** @brief Base row in Weight storage for this NetCon's weight block. */
struct WeightIndex {
    using type = weight_index_t;
    constexpr type default_value() const {
        return invalid_weight_index;
    }
};

/** @brief Number of weights (pnt_receive_size). */
struct WeightCount {
    using type = netcon_count_t;
    constexpr type default_value() const {
        return 0;
    }
};

/** @brief Delivery delay (ms). */
struct Delay {
    using type = double;
    constexpr type default_value() const {
        return 1.0;
    }
};

/** @brief Active flag as int (GPU-friendly). */
struct Active {
    using type = int;
    constexpr type default_value() const {
        return 1;
    }
};

/**
 * @brief Source PreSyn row, or -1.
 *
 * Phase 2: remains -1 until PreSyn SoA exists (Phase 3). Reverse edge only.
 */
struct SrcPreSyn {
    using type = int;
    constexpr type default_value() const {
        return -1;
    }
};

}  // namespace field

/**
 * @brief Public API for NetCon handles (owning and non-owning).
 * @tparam Identifier owning_identifier or non_owning_identifier for storage.
 */
template <typename Identifier>
struct handle_interface: handle_base<Identifier> {
    using base_type = handle_base<Identifier>;
    using base_type::base_type;

    [[nodiscard]] field::Target::type& target() {
        return this->template get<field::Target>();
    }
    [[nodiscard]] field::Target::type const& target() const {
        return this->template get<field::Target>();
    }

    [[nodiscard]] field::WeightIndex::type& weight_index() {
        return this->template get<field::WeightIndex>();
    }
    [[nodiscard]] field::WeightIndex::type const& weight_index() const {
        return this->template get<field::WeightIndex>();
    }

    [[nodiscard]] field::WeightCount::type& weight_count() {
        return this->template get<field::WeightCount>();
    }
    [[nodiscard]] field::WeightCount::type const& weight_count() const {
        return this->template get<field::WeightCount>();
    }

    [[nodiscard]] field::Delay::type& delay() {
        return this->template get<field::Delay>();
    }
    [[nodiscard]] field::Delay::type const& delay() const {
        return this->template get<field::Delay>();
    }

    [[nodiscard]] field::Active::type& active() {
        return this->template get<field::Active>();
    }
    [[nodiscard]] field::Active::type const& active() const {
        return this->template get<field::Active>();
    }

    [[nodiscard]] field::SrcPreSyn::type& src_presyn() {
        return this->template get<field::SrcPreSyn>();
    }
    [[nodiscard]] field::SrcPreSyn::type const& src_presyn() const {
        return this->template get<field::SrcPreSyn>();
    }

    friend std::ostream& operator<<(std::ostream& os, handle_interface const& handle) {
        if (handle.id()) {
            return os << "NetCon{" << handle.id() << '/' << handle.underlying_storage().size()
                      << " target=" << handle.target() << " widx=" << handle.weight_index()
                      << " wcnt=" << handle.weight_count() << " delay=" << handle.delay()
                      << " active=" << handle.active() << " src=" << handle.src_presyn() << '}';
        }
        return os << "NetCon{null}";
    }
};

/** @brief Underlying storage for all NetCon integration rows. */
struct storage: soa<storage,
                    field::Target,
                    field::WeightIndex,
                    field::WeightCount,
                    field::Delay,
                    field::Active,
                    field::SrcPreSyn> {
    [[nodiscard]] std::string_view name() const {
        return "network::NetCon";
    }
};

using handle = handle_interface<non_owning_identifier<storage>>;

struct owning_handle: handle_interface<owning_identifier<storage>> {
    using base_type = handle_interface<owning_identifier<storage>>;
    using base_type::base_type;

    [[nodiscard]] handle non_owning_handle() {
        return non_owning_identifier<storage>{&underlying_storage(), id()};
    }
};
}  // namespace neuron::container::network::NetCon
