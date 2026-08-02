#pragma once
/**
 * @file network/indices.hpp
 * @brief Integer index types for network bulk tables (CoreNEURON-shaped).
 *
 * Prefer these over NetCon* / size_t in fanout ranges, weight bases, and other
 * dense tables. Shell pointers remain appropriate for HOC and TQueue
 * DiscreteEvent* polymorphism.
 *
 * See doc/network-soa/heap-free.md.
 */
#include <cstdint>

namespace neuron::container::network {

/** @brief Base row into Weight SoA / flat weight pool (-1 = no weights). */
using weight_index_t = std::int32_t;

/** @brief Index into NetCon SoA or fanout order (-1 = none). */
using netcon_index_t = std::int32_t;

/** @brief Count of NetCons in a fanout or weight arity (non-negative). */
using netcon_count_t = std::int32_t;

/** @brief Sentinel: no weight block / no NetCon. */
inline constexpr weight_index_t invalid_weight_index = -1;
inline constexpr netcon_index_t invalid_netcon_index = -1;

}  // namespace neuron::container::network
