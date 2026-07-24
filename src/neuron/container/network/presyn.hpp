#pragma once
/**
 * @file network/presyn.hpp
 * @brief SoA storage for integration-hot PreSyn fields.
 *
 * CoreNEURON reference: nc_index_, nc_cnt_, thvar_index_, threshold_, gid_.
 * Design: doc/network-soa-phase0.md §5.5.
 *
 * Phase 3 dual-write / heap-free step 3: legacy PreSyn keeps dil_ (rebuild
 * source) and thvar_; this SoA holds CoreNEURON-shaped columns. Fanout order is
 * a global table of NetCon SoA row indices (see netcvode.cpp); NcIndex/NcCount
 * describe ranges in that table.
 */
#include "neuron/container/data_handle.hpp"
#include "neuron/container/network/indices.hpp"
#include "neuron/container/soa_container.hpp"
#include "neuron/container/view_utils.hpp"

#include <ostream>
#include <string_view>

namespace neuron::container::network::PreSyn {
namespace field {

/** @brief Spike threshold. */
struct Threshold {
    using type = double;
    constexpr type default_value() const {
        return 10.;
    }
};

/** @brief Output gid, or -1. */
struct Gid {
    using type = int;
    constexpr type default_value() const {
        return -1;
    }
};

/** @brief Start index into global fanout order (NetCon SoA rows). */
struct NcIndex {
    using type = netcon_index_t;
    constexpr type default_value() const {
        return invalid_netcon_index;
    }
};

/** @brief Fanout count (replaces dil_.size() on hot path when sorted). */
struct NcCount {
    using type = netcon_count_t;
    constexpr type default_value() const {
        return 0;
    }
};

/** @brief CoreNEURON output_index_ (MPI / spike compression). */
struct OutputIndex {
    using type = int;
    constexpr type default_value() const {
        return -1;
    }
};

/**
 * @brief Optional denormalized node-voltage row for threshold scans.
 *
 * Canonical threshold source remains PreSyn::thvar_ (data_handle). -1 if unknown.
 */
struct ThVarRow {
    using type = int;
    constexpr type default_value() const {
        return -1;
    }
};

/** @brief Owning NrnThread id. */
struct ThreadId {
    using type = int;
    constexpr type default_value() const {
        return -1;
    }
};

}  // namespace field

template <typename Identifier>
struct handle_interface: handle_base<Identifier> {
    using base_type = handle_base<Identifier>;
    using base_type::base_type;

    [[nodiscard]] field::Threshold::type& threshold() {
        return this->template get<field::Threshold>();
    }
    [[nodiscard]] field::Threshold::type const& threshold() const {
        return this->template get<field::Threshold>();
    }

    [[nodiscard]] field::Gid::type& gid() {
        return this->template get<field::Gid>();
    }
    [[nodiscard]] field::Gid::type const& gid() const {
        return this->template get<field::Gid>();
    }

    [[nodiscard]] field::NcIndex::type& nc_index() {
        return this->template get<field::NcIndex>();
    }
    [[nodiscard]] field::NcIndex::type const& nc_index() const {
        return this->template get<field::NcIndex>();
    }

    [[nodiscard]] field::NcCount::type& nc_count() {
        return this->template get<field::NcCount>();
    }
    [[nodiscard]] field::NcCount::type const& nc_count() const {
        return this->template get<field::NcCount>();
    }

    [[nodiscard]] field::OutputIndex::type& output_index() {
        return this->template get<field::OutputIndex>();
    }
    [[nodiscard]] field::OutputIndex::type const& output_index() const {
        return this->template get<field::OutputIndex>();
    }

    [[nodiscard]] field::ThVarRow::type& thvar_row() {
        return this->template get<field::ThVarRow>();
    }
    [[nodiscard]] field::ThVarRow::type const& thvar_row() const {
        return this->template get<field::ThVarRow>();
    }

    [[nodiscard]] field::ThreadId::type& thread_id() {
        return this->template get<field::ThreadId>();
    }
    [[nodiscard]] field::ThreadId::type const& thread_id() const {
        return this->template get<field::ThreadId>();
    }

    friend std::ostream& operator<<(std::ostream& os, handle_interface const& handle) {
        if (handle.id()) {
            return os << "PreSyn{" << handle.id() << '/' << handle.underlying_storage().size()
                      << " thr=" << handle.threshold() << " gid=" << handle.gid() << " nc=["
                      << handle.nc_index() << "," << handle.nc_count() << ")"
                      << " out=" << handle.output_index() << " tid=" << handle.thread_id() << '}';
        }
        return os << "PreSyn{null}";
    }
};

struct storage: soa<storage,
                    field::Threshold,
                    field::Gid,
                    field::NcIndex,
                    field::NcCount,
                    field::OutputIndex,
                    field::ThVarRow,
                    field::ThreadId> {
    [[nodiscard]] std::string_view name() const {
        return "network::PreSyn";
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
}  // namespace neuron::container::network::PreSyn
