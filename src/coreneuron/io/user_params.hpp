#pragma once

namespace coreneuron {

/// This structure is data needed is several part of nrn_setup, phase1 and phase2.
/// Before it was globals variables, group them to give them as a single argument.
/// They have for the most part, nothing related to each other.
struct UserParams {
    UserParams(int ngroup_, int* gidgroups_, int* imult_,
        const char* path_, const char* restore_path_)
    : ngroup(ngroup_)
    , gidgroups(gidgroups_)
    , imult(imult_)
    , path(path_)
    , restore_path(restore_path_)
    , file_reader(ngroup_)
    {}

    /// direct memory mode with neuron, do not open files
    /// Number of local cell groups
    const int ngroup;
    /// Array of cell group numbers (indices)
    const int* const gidgroups;
    /// Array of duplicate indices. Normally, with nrn_setup_multiple=1,
    //   they are ngroup values of 0.
    const int* const imult;
    /// path to dataset file
    const char* const path;
    /// Dataset path from where simulation is being restored
    const char* const restore_path;
    std::vector<FileHandler> file_reader;
};
}  // namespace coreneuron
