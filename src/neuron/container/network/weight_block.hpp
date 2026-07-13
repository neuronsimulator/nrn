#pragma once
/**
 * @file network/weight_block.hpp
 * @brief Helpers to dual-write NetCon weight blocks into Weight SoA.
 *
 * Kept separate from weights.hpp so model_data.hpp can include Weight storage
 * without a circular include.
 *
 * Phase 1: heap `double* weight_` remains the delivery primary; SoA rows are
 * owned in parallel for layout readiness (Phase 2 WeightIndex).
 */
#include "neuron/container/network/weights.hpp"
#include "neuron/model_data.hpp"

#include <algorithm>
#include <vector>

namespace neuron::container::network::Weight {

/**
 * @brief Allocate @p n weight SoA rows, optionally mirroring heap values.
 */
inline std::vector<owning_handle> allocate_weight_rows(int n, double const* mirror = nullptr) {
    std::vector<owning_handle> rows;
    if (n <= 0) {
        return rows;
    }
    auto& store = neuron::model().weights();
    rows.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        rows.emplace_back(store);
        rows.back().value() = mirror ? mirror[i] : 0.;
    }
    return rows;
}

/** @brief Copy heap weight values into already-allocated SoA rows. */
inline void mirror_weights_to_soa(std::vector<owning_handle>& rows, double const* heap, int n) {
    if (!heap) {
        return;
    }
    auto const m = std::min(static_cast<int>(rows.size()), n);
    for (int i = 0; i < m; ++i) {
        rows[i].value() = heap[i];
    }
}

}  // namespace neuron::container::network::Weight
