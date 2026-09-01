#pragma once
/**
 * @file network/sort.hpp
 * @brief Sort / repack network SoA containers inside nrn_ensure_model_data_are_sorted.
 *
 * Design: doc/network-soa-phase0.md §6, §8.2–§8.4.
 */
#include "neuron/cache/model_data.hpp"
#include "neuron/model_data.hpp"

namespace neuron::container::network {

/**
 * @brief Partition and repack network SoA for integration.
 *
 * Order (after nodes + mechanisms are already sorted):
 *   1. PointProcess by thread
 *   2. Weight blocks contiguous per NetCon, packed by target thread
 *   3. NetCon by (target thread, src PreSyn)
 *   4. PreSyn by thread + rebuild NcIndex/NcCount fanout ranges
 *
 * @param cache Working model cache (thread offsets filled here).
 * @param pp_token Sole frozen token for PointProcess storage.
 * @param w_token  Sole frozen token for Weight storage.
 * @param nc_token Sole frozen token for NetCon storage.
 * @param ps_token Sole frozen token for PreSyn storage.
 */
void sort_network_data(neuron::cache::Model& cache,
                       PointProcess::storage::frozen_token_type& pp_token,
                       Weight::storage::frozen_token_type& w_token,
                       NetCon::storage::frozen_token_type& nc_token,
                       PreSyn::storage::frozen_token_type& ps_token);

}  // namespace neuron::container::network
