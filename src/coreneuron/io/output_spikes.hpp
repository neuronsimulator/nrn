/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#pragma once

#include <string>
#include <vector>
#include <utility>
#include "coreneuron/io/reports/nrnreport.hpp"
namespace coreneuron {
void output_spikes(const char* outpath, const SpikesInfo& spikes_info);
void mk_spikevec_buffer(int);

extern std::vector<double> spikevec_time;
extern std::vector<int> spikevec_gid;

void clear_spike_vectors();
void validation(std::vector<std::pair<double, int>>& res);

void spikevec_lock();
void spikevec_unlock();
}  // namespace coreneuron
