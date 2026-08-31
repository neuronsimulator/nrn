#pragma once
/**
 * @file network/weight_block.hpp
 * @brief One logical weight block (arity scalars) owned off the NetCon shell.
 *
 * Heap-free step 1: NetCon is O(1); authority for the base is NetCon SoA
 * WeightIndex/WeightCount. This type holds the container owning_identifiers
 * (required by soa<> lifetime/permute) without embedding a vector in NetCon.
 *
 * HOC weight[i] uses value_handle(i) (stable across permute). Contiguous
 * base+i access is valid after sort packs the block.
 *
 * See doc/network-soa/heap-free.md.
 */
#include "neuron/container/network/indices.hpp"
#include "neuron/container/network/weights.hpp"
#include "neuron/model_data.hpp"

#include <algorithm>
#include <cassert>
#include <memory>
#include <vector>

namespace neuron::container::network::Weight {

/**
 * @brief Owning storage for one NetCon's contiguous-logical weight block.
 *
 * Rows remain individually tracked for permute; sort repacks them contiguous.
 */
struct WeightBlock {
    std::vector<owning_handle> rows;

    [[nodiscard]] int size() const {
        return static_cast<int>(rows.size());
    }

    [[nodiscard]] bool empty() const {
        return rows.empty();
    }

    /** @brief Current base row (-1 if empty). Updates after permute/erase. */
    [[nodiscard]] weight_index_t base_row() const {
        if (rows.empty() || !rows.front().id()) {
            return invalid_weight_index;
        }
        return static_cast<weight_index_t>(rows.front().current_row());
    }

    [[nodiscard]] field::Value::type& value(int i) {
        assert(i >= 0 && i < size());
        return rows[static_cast<std::size_t>(i)].value();
    }

    [[nodiscard]] field::Value::type value(int i) const {
        assert(i >= 0 && i < size());
        return rows[static_cast<std::size_t>(i)].value();
    }

    /** @brief Permutation-stable handle for HOC weight[i] / _ref_weight[i]. */
    [[nodiscard]] data_handle<field::Value::type> value_handle(int i) {
        assert(i >= 0 && i < size());
        return rows[static_cast<std::size_t>(i)].value_handle();
    }

    /**
     * @brief Pointer to first value if rows are contiguous in storage (base..base+n-1).
     *
     * True after allocate and after network weight sort; may be false after
     * unrelated erases until the next repack. Used for zero-copy pnt_receive (6b).
     */
    [[nodiscard]] double* data_if_contiguous() {
        if (rows.empty()) {
            return nullptr;
        }
        auto& store = neuron::model().weights();
        auto const r0 = rows.front().current_row();
        auto const n = rows.size();
        if (r0 + n > store.size()) {
            return nullptr;
        }
        for (std::size_t i = 1; i < n; ++i) {
            if (!rows[i].id() || rows[i].current_row() != r0 + i) {
                return nullptr;
            }
        }
        return &store.get<field::Value>(r0);
    }

    [[nodiscard]] double const* data_if_contiguous() const {
        return const_cast<WeightBlock*>(this)->data_if_contiguous();
    }
};

/**
 * @brief Allocate @p n weight SoA rows, optionally mirroring heap values.
 * @return Owning block (nullptr if n <= 0).
 */
inline std::unique_ptr<WeightBlock> allocate_weight_block(int n, double const* mirror = nullptr) {
    if (n <= 0) {
        return nullptr;
    }
    auto block = std::make_unique<WeightBlock>();
    auto& store = neuron::model().weights();
    block->rows.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        block->rows.emplace_back(store);
        block->rows.back().value() = mirror ? mirror[i] : 0.;
    }
    return block;
}

/** @brief Copy heap weight values into an existing block. */
inline void mirror_heap_to_block(WeightBlock& block, double const* heap, int n) {
    if (!heap) {
        return;
    }
    auto const m = std::min(block.size(), n);
    for (int i = 0; i < m; ++i) {
        block.value(i) = heap[i];
    }
}

/** @brief Copy block values into heap. */
inline void mirror_block_to_heap(WeightBlock const& block, double* heap, int n) {
    if (!heap) {
        return;
    }
    auto const m = std::min(block.size(), n);
    for (int i = 0; i < m; ++i) {
        heap[i] = block.value(i);
    }
}

// --- Backward-compatible aliases used by unit tests / gradual migration ---

/** @deprecated Prefer allocate_weight_block. */
inline std::vector<owning_handle> allocate_weight_rows(int n, double const* mirror = nullptr) {
    auto block = allocate_weight_block(n, mirror);
    if (!block) {
        return {};
    }
    return std::move(block->rows);
}

/** @deprecated Prefer mirror_heap_to_block. */
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
