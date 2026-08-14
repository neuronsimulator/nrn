#pragma once
/**
 * @file network/self_event.hpp
 * @brief Phase 4 SelfEvent index fields (not a long-lived SoA container).
 *
 * Design: doc/network-soa-phase0.md §5.6, §9.
 *
 * SelfEvents are short-lived queue/pool objects. Integration-relevant payload:
 *   - target PointProcess row (optional dual-write)
 *   - weight_index into Weight SoA (base of the NetCon weight block)
 *   - flag (NET_RECEIVE flag)
 *
 * Generated MOD code still receives double* for Phase 4 (M2 dual-write):
 * delivery materializes SoA → heap around pnt_receive.
 */
#include "neuron/container/network/weights.hpp"
#include "neuron/model_data.hpp"

#include <vector>

namespace neuron::container::network::SelfEventFields {

/** Field tags for documentation / future structural pool (not used as soa<> tags yet). */
namespace field {
struct TargetPnt {
    using type = int;
    static constexpr type default_value() {
        return -1;
    }
};
struct WeightIndex {
    using type = int;
    static constexpr type default_value() {
        return -1;
    }
};
struct Flag {
    using type = double;
    static constexpr type default_value() {
        return 0.;
    }
};
}  // namespace field

/**
 * @brief CoreNEURON-style zero-copy pointer into Weight SoA at base @p weight_index.
 *
 * Under packing A (post-sort / sim freeze), a NetCon block of @p count is
 * base..base+count-1 consecutive rows. No NetCon* reverse lookup: the index is
 * the address (cf. NrnThread::weights + weight_index).
 *
 * @return nullptr if out of range (caller may TLS-materialize).
 */
inline double* weight_soa_ptr(int weight_index, int count) {
    if (weight_index < 0 || count <= 0) {
        return nullptr;
    }
    auto& store = neuron::model().weights();
    auto const n = store.size();
    auto const base = static_cast<std::size_t>(weight_index);
    auto const need = static_cast<std::size_t>(count);
    if (base >= n || base + need > n) {
        return nullptr;
    }
    return &store.get<Weight::field::Value>(base);
}

/**
 * @brief Copy @p count consecutive Weight SoA rows starting at @p weight_index into @p out.
 *
 * Used when zero-copy is unavailable (scattered rows before pack). After weight
 * repack/sort, prefer weight_soa_ptr.
 */
inline void materialize_weight_block(int weight_index, int count, double* out) {
    if (!out || count <= 0 || weight_index < 0) {
        return;
    }
    auto& store = neuron::model().weights();
    auto const n = static_cast<int>(store.size());
    for (int i = 0; i < count; ++i) {
        int const row = weight_index + i;
        out[i] = (row >= 0 && row < n)
                     ? store.get<Weight::field::Value>(static_cast<std::size_t>(row))
                     : 0.;
    }
}

/**
 * @brief Write @p count heap values back into Weight SoA at @p weight_index.
 */
inline void store_weight_block(int weight_index, int count, double const* in) {
    if (!in || count <= 0 || weight_index < 0) {
        return;
    }
    auto& store = neuron::model().weights();
    auto const n = static_cast<int>(store.size());
    for (int i = 0; i < count; ++i) {
        int const row = weight_index + i;
        if (row >= 0 && row < n) {
            store.get<Weight::field::Value>(static_cast<std::size_t>(row)) = in[i];
        }
    }
}

}  // namespace neuron::container::network::SelfEventFields
