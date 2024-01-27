/*
# =============================================================================
# Copyright (c) 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/
#include "coreneuron/apps/corenrn_parameters.hpp"
#include "coreneuron/gpu/nrn_acc_manager.hpp"
#include "coreneuron/permute/cellorder.hpp"
#include "coreneuron/permute/node_permute.h"
#include "coreneuron/sim/multicore.hpp"

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <iostream>
#include <functional>
#include <map>
#include <random>
#include <utility>
#include <vector>

using namespace coreneuron;

struct SolverData {
    std::vector<double> d, rhs;
    std::vector<int> parent_index;
};

constexpr auto magic_index_value = -2;
constexpr auto magic_double_value = std::numeric_limits<double>::lowest();

enum struct SolverImplementation {
    CellPermute0_CPU,
    CellPermute0_GPU,
    CellPermute1_CPU,
    CellPermute1_GPU,
    CellPermute2_CPU,
    CellPermute2_GPU,
    CellPermute2_CUDA
};

std::ostream& operator<<(std::ostream& os, SolverImplementation impl) {
    if (impl == SolverImplementation::CellPermute0_CPU) {
        return os << "SolverImplementation::CellPermute0_CPU";
    } else if (impl == SolverImplementation::CellPermute0_GPU) {
        return os << "SolverImplementation::CellPermute0_GPU";
    } else if (impl == SolverImplementation::CellPermute1_CPU) {
        return os << "SolverImplementation::CellPermute1_CPU";
    } else if (impl == SolverImplementation::CellPermute1_GPU) {
        return os << "SolverImplementation::CellPermute1_GPU";
    } else if (impl == SolverImplementation::CellPermute2_CPU) {
        return os << "SolverImplementation::CellPermute2_CPU";
    } else if (impl == SolverImplementation::CellPermute2_GPU) {
        return os << "SolverImplementation::CellPermute2_GPU";
    } else if (impl == SolverImplementation::CellPermute2_CUDA) {
        return os << "SolverImplementation::CellPermute2_CUDA";
    } else {
        throw std::runtime_error("Invalid SolverImplementation");
    }
}

struct ToyModelConfig {
    int num_threads{1};
    int num_cells{1};
    int num_segments_per_cell{3};
    std::function<double(int, int)> produce_a{[](auto, auto) { return 3.14159; }},
        produce_b{[](auto, auto) { return 42.0; }}, produce_d{[](auto, auto) { return 7.0; }},
        produce_rhs{[](auto, auto) { return -16.0; }};
};

// TODO include some global lock as a sanity check (only one instance of
// SetupThreads should exist at any given time)
struct SetupThreads {
    SetupThreads(SolverImplementation impl, ToyModelConfig config = {}) {
        corenrn_param.cuda_interface = false;
        corenrn_param.gpu = false;
        switch (impl) {
        case SolverImplementation::CellPermute0_GPU:
            corenrn_param.gpu = true;
            [[fallthrough]];
        case SolverImplementation::CellPermute0_CPU:
            interleave_permute_type = 0;
            break;
        case SolverImplementation::CellPermute1_GPU:
            corenrn_param.gpu = true;
            [[fallthrough]];
        case SolverImplementation::CellPermute1_CPU:
            interleave_permute_type = 1;
            break;
        case SolverImplementation::CellPermute2_CUDA:
            corenrn_param.cuda_interface = true;
            [[fallthrough]];
        case SolverImplementation::CellPermute2_GPU:
            corenrn_param.gpu = true;
            [[fallthrough]];
        case SolverImplementation::CellPermute2_CPU:
            interleave_permute_type = 2;
            break;
        }
        use_solve_interleave = interleave_permute_type > 0;
        nrn_threads_create(config.num_threads);
        create_interleave_info();
        int num_cells_remaining{config.num_cells}, total_cells{};
        for (auto ithread = 0; ithread < nrn_nthread; ++ithread) {
            auto& nt = nrn_threads[ithread];
            // How many cells to distribute on this thread, trying to get the right
            // total even if num_threads does not exactly divide num_cells.
            nt.ncell = num_cells_remaining / (nrn_nthread - ithread);
            total_cells += nt.ncell;
            num_cells_remaining -= nt.ncell;
            // How many segments are there in this thread?
            nt.end = nt.ncell * config.num_segments_per_cell;
            auto const padded_size = nrn_soa_padded_size(nt.end, 0);
            // Allocate one big block because the GPU data transfer code assumes this.
            nt._ndata = padded_size * 4;
            nt._data = static_cast<double*>(emalloc_align(nt._ndata * sizeof(double)));
            auto* vec_rhs = (nt._actual_rhs = nt._data + 0 * padded_size);
            auto* vec_d = (nt._actual_d = nt._data + 1 * padded_size);
            auto* vec_a = (nt._actual_a = nt._data + 2 * padded_size);
            auto* vec_b = (nt._actual_b = nt._data + 3 * padded_size);
            auto* parent_indices =
                (nt._v_parent_index = static_cast<int*>(emalloc_align(padded_size * sizeof(int))));
            // Magic value to check against later.
            std::fill(parent_indices, parent_indices + nt.end, magic_index_value);
            // Put all the root nodes first, then put the other segments
            // in blocks. i.e. ABCDAAAABBBBCCCCDDDD
            auto const get_index = [ncell = nt.ncell,
                                    nseg = config.num_segments_per_cell](auto icell, auto iseg) {
                if (iseg == 0) {
                    return icell;
                } else {
                    return ncell + icell * (nseg - 1) + iseg - 1;
                }
            };
            for (auto icell = 0; icell < nt.ncell; ++icell) {
                for (auto iseg = 0; iseg < config.num_segments_per_cell; ++iseg) {
                    auto const global_index = get_index(icell, iseg);
                    vec_a[global_index] = config.produce_a(icell, iseg);
                    vec_b[global_index] = config.produce_b(icell, iseg);
                    vec_d[global_index] = config.produce_d(icell, iseg);
                    vec_rhs[global_index] = config.produce_rhs(icell, iseg);
                    // 0th element is the root node, which has no parent
                    // other elements are attached in a binary tree configuration
                    // |      0      |
                    // |    /   \    |
                    // |   1     2   |
                    // |  / \   / \  |
                    // | 3   4 5   6 |
                    // TODO: include some other topologies, e.g. a long straight line, or
                    // an unbalanced tree.
                    auto const parent_id = iseg ? get_index(icell, (iseg - 1) / 2) : -1;
                    parent_indices[global_index] = parent_id;
                }
            }
            // Check we didn't mess up populating any parent indices
            for (auto i = 0; i < nt.end; ++i) {
                REQUIRE(parent_indices[i] != magic_index_value);
                // Root nodes should come first for --cell-permute=0
                if (i < nt.ncell) {
                    REQUIRE(parent_indices[i] == -1);
                }
            }
            if (interleave_permute_type) {
                nt._permute = interleave_order(nt.id, nt.ncell, nt.end, parent_indices);
                REQUIRE(nt._permute);
                permute_data(vec_a, nt.end, nt._permute);
                permute_data(vec_b, nt.end, nt._permute);
                // This isn't done in CoreNEURON because these are reset every
                // time step, but permute d/rhs here so that the initial values
                // set by produce_d and produce_rhs are propagated consistently
                // to all of the solver implementations.
                permute_data(vec_d, nt.end, nt._permute);
                permute_data(vec_rhs, nt.end, nt._permute);
                // index values change as well as ordering
                permute_ptr(parent_indices, nt.end, nt._permute);
                node_permute(parent_indices, nt.end, nt._permute);
            }
        }
        if (impl == SolverImplementation::CellPermute0_GPU) {
            std::cout << "CellPermute0_GPU is a nonstandard configuration, copying data to the "
                         "device may produce warnings:";
        }
        if (corenrn_param.gpu) {
            setup_nrnthreads_on_device(nrn_threads, nrn_nthread);
        }
        if (impl == SolverImplementation::CellPermute0_GPU) {
            std::cout << "\n...no more warnings expected" << std::endl;
        }
        // Make sure we produced the number of cells we were aiming for
        REQUIRE(total_cells == config.num_cells);
        REQUIRE(num_cells_remaining == 0);
    }

    ~SetupThreads() {
        if (corenrn_param.gpu) {
            delete_nrnthreads_on_device(nrn_threads, nrn_nthread);
        }
        for (auto& nt: *this) {
            free_memory(std::exchange(nt._data, nullptr));
            delete[] std::exchange(nt._permute, nullptr);
            free_memory(std::exchange(nt._v_parent_index, nullptr));
        }
        destroy_interleave_info();
        nrn_threads_free();
    }

    auto dump_solver_data() {
        std::vector<SolverData> ret{static_cast<std::size_t>(nrn_nthread)};
        // Sync the solver data from GPU to host
        update_nrnthreads_on_host(nrn_threads, nrn_nthread);
        // Un-permute the data in and store it in ret.{d,parent_index,rhs}
        for (auto i = 0; i < nrn_nthread; ++i) {
            auto& nt = nrn_threads[i];
            auto& sd = ret[i];
            sd.d.resize(nt.end, magic_double_value);
            sd.parent_index.resize(nt.end, magic_index_value);
            sd.rhs.resize(nt.end, magic_double_value);
            auto* inv_permute = nt._permute ? inverse_permute(nt._permute, nt.end) : nullptr;
            for (auto i = 0; i < nt.end; ++i) {
                // index in permuted vectors
                auto const p_i = nt._permute ? nt._permute[i] : i;
                // parent index in permuted vectors
                auto const p_parent = nt._v_parent_index[p_i];
                // parent index in unpermuted vectors (i.e. on the same scale as `i`)
                auto const parent = p_parent == -1
                                        ? -1
                                        : (inv_permute ? inv_permute[p_parent] : p_parent);
                // Save the values to the de-permuted return structure
                sd.d[i] = nt._actual_d[p_i];
                sd.parent_index[i] = parent;
                sd.rhs[i] = nt._actual_rhs[p_i];
            }
            delete[] inv_permute;
            for (auto i = 0; i < nt.end; ++i) {
                REQUIRE(sd.d[i] != magic_double_value);
                REQUIRE(sd.parent_index[i] != magic_index_value);
                REQUIRE(sd.rhs[i] != magic_double_value);
            }
        }
        return ret;
    }

    void solve() {
        for (auto& thread: *this) {
            nrn_solve_minimal(&thread);
        }
    }

    NrnThread* begin() const {
        return nrn_threads;
    }
    NrnThread* end() const {
        return nrn_threads + nrn_nthread;
    }
};

template <typename... Args>
auto solve_and_dump(Args&&... args) {
    SetupThreads threads{std::forward<Args>(args)...};
    threads.solve();
    return threads.dump_solver_data();
}

auto active_implementations() {
    // These are always available
    std::vector<SolverImplementation> ret{SolverImplementation::CellPermute0_CPU,
                                          SolverImplementation::CellPermute1_CPU,
                                          SolverImplementation::CellPermute2_CPU};
#ifdef CORENEURON_ENABLE_GPU
    // Consider making these steerable via a runtime switch in GPU builds
    ret.push_back(SolverImplementation::CellPermute0_GPU);
    ret.push_back(SolverImplementation::CellPermute1_GPU);
    ret.push_back(SolverImplementation::CellPermute2_GPU);
    ret.push_back(SolverImplementation::CellPermute2_CUDA);
#endif
    return ret;
}

void compare_solver_data(
    std::map<SolverImplementation, std::vector<SolverData>> const& solver_data) {
    // CellPermute0_CPU is the simplest version of the solver, it should always
    // be present and it's a good reference to use
    constexpr auto ref_impl = SolverImplementation::CellPermute0_CPU;
    REQUIRE(solver_data.find(ref_impl) != solver_data.end());
    auto const& ref_data = solver_data.at(ref_impl);
    for (auto const& [impl, impl_data]: solver_data) {
        // Must have compatible numbers of threads.
        REQUIRE(impl_data.size() == ref_data.size());
        std::cout << "Comparing " << impl << " to " << ref_impl << std::endl;
        for (auto n_thread = 0ul; n_thread < impl_data.size(); ++n_thread) {
            // Must have compatible numbers of segments/data entries
            REQUIRE(impl_data[n_thread].d.size() == ref_data[n_thread].d.size());
            REQUIRE(impl_data[n_thread].parent_index.size() ==
                    ref_data[n_thread].parent_index.size());
            REQUIRE(impl_data[n_thread].rhs.size() == ref_data[n_thread].rhs.size());
            CHECK_THAT(impl_data[n_thread].d, Catch::Approx(ref_data[n_thread].d));
            REQUIRE(impl_data[n_thread].parent_index == ref_data[n_thread].parent_index);
            CHECK_THAT(impl_data[n_thread].rhs, Catch::Approx(ref_data[n_thread].rhs));
        }
    }
}

template <typename... Args>
auto compare_all_active_implementations(Args&&... args) {
    std::map<SolverImplementation, std::vector<SolverData>> solver_data;
    for (auto impl: active_implementations()) {
        solver_data[impl] = solve_and_dump(impl, std::forward<Args>(args)...);
    }
    compare_solver_data(solver_data);
    return solver_data;
}

// *Roughly* tuned to accomodate NVHPC 22.3 at -O0; the largest differences come
// from the pseudorandom seeded tests.
constexpr double default_tolerance = 2e-11;

TEST_CASE("SingleCellAndThread", "[solver][single-thread]") {
    constexpr std::size_t segments = 32;
    ToyModelConfig config{};
    config.num_segments_per_cell = segments;
    auto const solver_data = compare_all_active_implementations(config);
    for (auto const& [impl, data]: solver_data) {
        REQUIRE(data.size() == 1);  // nthreads
        REQUIRE(data[0].d.size() == segments);
        REQUIRE(data[0].parent_index.size() == segments);
        REQUIRE(data[0].rhs.size() == segments);
    }
}

TEST_CASE("UnbalancedCellSingleThread", "[solver][single-thread]") {
    ToyModelConfig config{};
    config.num_segments_per_cell = 19;  // not a nice round number
    compare_all_active_implementations(config);
}

TEST_CASE("LargeCellSingleThread", "[solver][single-thread]") {
    ToyModelConfig config{};
    config.num_segments_per_cell = 4096;
    compare_all_active_implementations(config);
}

TEST_CASE("ManySmallCellsSingleThread", "[solver][single-thread]") {
    ToyModelConfig config{};
    config.num_cells = 1024;
    compare_all_active_implementations(config);
}

TEST_CASE("ManySmallCellsMultiThread", "[solver][multi-thread]") {
    ToyModelConfig config{};
    config.num_cells = 1024;
    config.num_threads = 2;
    compare_all_active_implementations(config);
}

auto random_config() {
    std::mt19937_64 gen{42};
    ToyModelConfig config{};
    config.produce_a = [g = gen, d = std::normal_distribution{1.0, 0.1}](int icell,
                                                                         int iseg) mutable {
        return d(g);
    };
    config.produce_b = [g = gen, d = std::normal_distribution{7.0, 0.2}](int, int) mutable {
        return d(g);
    };
    config.produce_d = [g = gen, d = std::normal_distribution{-0.1, 0.01}](int, int) mutable {
        return d(g);
    };
    config.produce_rhs = [g = gen, d = std::normal_distribution{-15.0, 2.0}](int, int) mutable {
        return d(g);
    };
    return config;
}

TEST_CASE("LargeCellSingleThreadRandom", "[solver][single-thread][random]") {
    auto config = random_config();
    config.num_segments_per_cell = 4096;
    compare_all_active_implementations(config);
}

TEST_CASE("ManySmallCellsSingleThreadRandom", "[solver][single-thread][random]") {
    auto config = random_config();
    config.num_cells = 1024;
    compare_all_active_implementations(config);
}
