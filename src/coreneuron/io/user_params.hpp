/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#pragma once

namespace coreneuron {

class CheckPoints;

/// This structure is data needed is several part of nrn_setup, phase1 and phase2.
/// Before it was globals variables, group them to give them as a single argument.
/// They have for the most part, nothing related to each other.
struct UserParams {
    UserParams(std::vector<int>&& cell_groups_,
               const char* path_,
               const char* restore_path_,
               CheckPoints& checkPoints_)
        : cell_groups(std::move(cell_groups_))
        , path(path_)
        , restore_path(restore_path_)
        , file_reader(cell_groups.size())
        , checkPoints(checkPoints_) {}

    /// direct memory mode with neuron, do not open files
    /// Vector of cell group numbers (indices)
    std::vector<int> cell_groups;
    /// path to dataset file
    const char* const path;
    /// Dataset path from where simulation is being restored
    const char* const restore_path;
    std::vector<FileHandler> file_reader;
    CheckPoints& checkPoints;
};
}  // namespace coreneuron
