/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#ifndef output_spikes_h
#define output_spikes_h

#include <string>
#include <vector>
#include <utility>
namespace coreneuron {
void output_spikes(const char* outpath, const std::string& population_name);
void mk_spikevec_buffer(int);

extern std::vector<double> spikevec_time;
extern std::vector<int> spikevec_gid;

void clear_spike_vectors();
void validation(std::vector<std::pair<double, int> >& res);

void spikevec_lock();
void spikevec_unlock();
}  // namespace coreneuron
#endif
