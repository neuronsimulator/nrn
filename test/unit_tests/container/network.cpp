#include "neuron/cache/model_data.hpp"
#include "neuron/container/network/netcon.hpp"
#include "neuron/container/network/point_process.hpp"
#include "neuron/container/network/point_process_access.hpp"
#include "neuron/container/network/presyn.hpp"
#include "neuron/container/network/self_event.hpp"
#include "neuron/container/network/sort.hpp"
#include "neuron/container/network/weight_block.hpp"
#include "neuron/container/network/weights.hpp"
#include "neuron/model_data.hpp"
#include "nrn_ansi.h"
#include "section.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <numeric>
#include <optional>
#include <random>
#include <vector>

using namespace neuron::container::network;

TEST_CASE("SOA-backed PointProcess structure",
          "[Neuron][data_structures][network][point_process]") {
    auto& storage = neuron::model().point_processes();
    REQUIRE(storage.size() == 0);

    GIVEN("Default-constructed owning handles") {
        PointProcess::owning_handle a{storage};
        PointProcess::owning_handle b{storage};
        THEN("Defaults match field tags") {
            REQUIRE(a.instance() == -1);
            REQUIRE(a.mech_type() == -1);
            REQUIRE(a.thread_id() == -1);
            REQUIRE(storage.size() == 2);
        }
        THEN("Fields can be set and read back via non-owning handles") {
            a.instance() = 7;
            a.mech_type() = 42;
            a.thread_id() = 1;
            auto ha = a.non_owning_handle();
            REQUIRE(ha.instance() == 7);
            REQUIRE(ha.mech_type() == 42);
            REQUIRE(ha.thread_id() == 1);
        }
        THEN("Destroying an owner invalidates non-owning handles to that row") {
            auto ha = a.non_owning_handle();
            REQUIRE(ha.id());
            {
                PointProcess::owning_handle tmp{std::move(a)};
                REQUIRE(ha.id());
                // tmp destroyed here
            }
            REQUIRE_FALSE(ha.id());
            REQUIRE(storage.size() == 1);  // only b remains
        }
    }

    GIVEN("Several rows and a reverse permutation") {
        constexpr int n = 8;
        std::vector<PointProcess::owning_handle> rows;
        rows.reserve(n);
        std::vector<int> ref_instance(n);
        for (int i = 0; i < n; ++i) {
            rows.emplace_back(storage);
            rows.back().instance() = 100 + i;
            rows.back().mech_type() = i;
            rows.back().thread_id() = i % 3;
            ref_instance[i] = 100 + i;
        }
        {
            auto token = storage.issue_frozen_token();
            storage.mark_as_sorted(token);
        }
        REQUIRE(storage.is_sorted());

        WHEN("A rotate reverse-permutation is applied") {
            std::vector<std::size_t> perm(n);
            std::iota(perm.begin(), perm.end(), 0);
            std::rotate(perm.begin(), std::next(perm.begin()), perm.end());
            storage.apply_reverse_permutation(std::move(perm));

            THEN("Handles still report the logical field values") {
                for (int i = 0; i < n; ++i) {
                    REQUIRE(rows[i].instance() == ref_instance[i]);
                    REQUIRE(rows[i].mech_type() == i);
                    REQUIRE(rows[i].thread_id() == i % 3);
                }
            }
            THEN("Underlying storage order differs from original creation order") {
                bool same = true;
                for (int i = 0; i < n; ++i) {
                    if (storage.get<PointProcess::field::Instance>(i) != ref_instance[i]) {
                        same = false;
                        break;
                    }
                }
                REQUIRE_FALSE(same);
            }
        }

        WHEN("A random reverse-permutation is applied") {
            std::vector<std::size_t> perm(n);
            std::iota(perm.begin(), perm.end(), 0);
            std::mt19937 g{42};
            std::shuffle(perm.begin(), perm.end(), g);
            storage.apply_reverse_permutation(std::move(perm));
            THEN("Handles survive and keep logical values") {
                for (int i = 0; i < n; ++i) {
                    REQUIRE(rows[i].instance() == ref_instance[i]);
                }
            }
        }
    }
}

TEST_CASE("Point_process dual-write into network SoA",
          "[Neuron][data_structures][network][point_process][dualwrite]") {
    auto& storage = neuron::model().point_processes();
    auto const before = storage.size();
    GIVEN("A default-constructed Point_process shell") {
        // Allocates an SoA row via Point_process ctor dual-write map.
        auto* pp = new Point_process{};
        THEN("SoA size grows by one and fields are defaults until prop is set") {
            REQUIRE(storage.size() == before + 1);
            REQUIRE(pp->_soa_id);
            REQUIRE(nrn_point_process_soa_row(pp) >= 0);
            auto h = neuron::container::network::point_process_soa(pp);
            REQUIRE(h.mech_type() == -1);
            REQUIRE(h.instance() == -1);
            REQUIRE(h.thread_id() == -1);
        }
        WHEN("The Point_process is destroyed") {
            delete pp;
            THEN("The SoA row is released") {
                REQUIRE(storage.size() == before);
            }
        }
    }
}

TEST_CASE("SOA-backed Weight structure", "[Neuron][data_structures][network][weights]") {
    auto& storage = neuron::model().weights();
    REQUIRE(storage.size() == 0);

    GIVEN("A contiguous weight block (NetCon-style)") {
        constexpr int weight_count = 5;
        std::vector<Weight::owning_handle> block;
        block.reserve(weight_count);
        for (int k = 0; k < weight_count; ++k) {
            block.emplace_back(storage);
            block.back().value() = static_cast<double>(k + 1);  // exact binary values
        }
        REQUIRE(storage.size() == static_cast<std::size_t>(weight_count));

        THEN("data_handle to Value is modern and yields correct values") {
            auto dh = block[2].value_handle();
            REQUIRE(dh);
            REQUIRE(dh.refers_to_a_modern_data_structure());
            REQUIRE(dh.refers_to<Weight::field::Value>(storage));
            REQUIRE(*dh == 3.0);
        }

        WHEN("The weight storage is reverse-permuted") {
            std::vector<std::size_t> perm(weight_count);
            std::iota(perm.begin(), perm.end(), 0);
            std::rotate(perm.begin(), std::next(perm.begin()), perm.end());
            storage.apply_reverse_permutation(std::move(perm));

            THEN("Handles still report the logical values") {
                for (int k = 0; k < weight_count; ++k) {
                    REQUIRE(block[k].value() == static_cast<double>(k + 1));
                }
            }
            THEN("data_handle still works after permute") {
                auto dh = block[0].value_handle();
                REQUIRE(*dh == 1.0);
            }
        }

        WHEN("The owning block is destroyed") {
            auto dh = block[0].value_handle();
            REQUIRE(dh);
            block.clear();
            THEN("data_handles become invalid and storage is empty") {
                REQUIRE_FALSE(dh);
                REQUIRE(storage.size() == 0);
            }
        }
    }

    GIVEN("allocate_weight_rows dual-write helper") {
        double heap[3] = {1.5, 2.5, 3.5};
        auto rows = Weight::allocate_weight_rows(3, heap);
        THEN("SoA mirrors the heap values") {
            REQUIRE(rows.size() == 3);
            REQUIRE(storage.size() == 3);
            REQUIRE(rows[0].value() == 1.5);
            REQUIRE(rows[1].value() == 2.5);
            REQUIRE(rows[2].value() == 3.5);
        }
        WHEN("heap is updated and remirrored") {
            heap[1] = 9.0;
            Weight::mirror_weights_to_soa(rows, heap, 3);
            REQUIRE(rows[1].value() == 9.0);
        }
    }
}

TEST_CASE("SOA-backed NetCon structure", "[Neuron][data_structures][network][netcon]") {
    auto& storage = neuron::model().netcons();
    auto& wstore = neuron::model().weights();
    REQUIRE(storage.size() == 0);

    GIVEN("Default-constructed NetCon SoA rows") {
        NetCon::owning_handle a{storage};
        NetCon::owning_handle b{storage};
        THEN("Defaults match field tags") {
            REQUIRE(a.target() == -1);
            REQUIRE(a.weight_index() == -1);
            REQUIRE(a.weight_count() == 0);
            REQUIRE(a.delay() == 1.0);
            REQUIRE(a.active() == 1);
            REQUIRE(a.src_presyn() == -1);
            REQUIRE(storage.size() == 2);
        }
        THEN("Fields round-trip and survive reverse permutation") {
            a.target() = 3;
            a.weight_index() = 10;
            a.weight_count() = 2;
            a.delay() = 0.5;
            a.active() = 0;
            b.target() = 7;
            b.delay() = 2.0;
            {
                auto token = storage.issue_frozen_token();
                storage.mark_as_sorted(token);
            }
            std::vector<std::size_t> perm{0, 1};
            std::rotate(perm.begin(), std::next(perm.begin()), perm.end());
            storage.apply_reverse_permutation(std::move(perm));
            REQUIRE(a.target() == 3);
            REQUIRE(a.weight_index() == 10);
            REQUIRE(a.weight_count() == 2);
            REQUIRE(a.delay() == 0.5);
            REQUIRE(a.active() == 0);
            REQUIRE(b.target() == 7);
            REQUIRE(b.delay() == 2.0);
        }
    }

    GIVEN("Weight block linked like Phase 2 dual-write") {
        auto wrows = Weight::allocate_weight_rows(2, nullptr);
        wrows[0].value() = 0.1;
        wrows[1].value() = 0.2;
        NetCon::owning_handle nc{storage};
        nc.weight_index() = static_cast<int>(wrows[0].current_row());
        nc.weight_count() = 2;
        THEN("WeightIndex points at first SoA weight row") {
            REQUIRE(wstore.get<Weight::field::Value>(nc.weight_index()) == 0.1);
            REQUIRE(wstore.get<Weight::field::Value>(nc.weight_index() + 1) == 0.2);
        }
    }
}

TEST_CASE("Weight index materialize/store (Phase 4 SelfEvent path)",
          "[Neuron][data_structures][network][selfevent]") {
    auto& store = neuron::model().weights();
    REQUIRE(store.size() == 0);
    auto rows = Weight::allocate_weight_rows(3, nullptr);
    rows[0].value() = 1.0;
    rows[1].value() = 2.0;
    rows[2].value() = 3.0;
    int const base = static_cast<int>(rows[0].current_row());
    double buf[3]{};
    neuron::container::network::SelfEventFields::materialize_weight_block(base, 3, buf);
    REQUIRE(buf[0] == 1.0);
    REQUIRE(buf[1] == 2.0);
    REQUIRE(buf[2] == 3.0);
    buf[1] = 9.0;
    neuron::container::network::SelfEventFields::store_weight_block(base, 3, buf);
    REQUIRE(rows[1].value() == 9.0);
}

TEST_CASE("SOA-backed PreSyn structure and fanout ranges",
          "[Neuron][data_structures][network][presyn]") {
    auto& storage = neuron::model().presyns();
    REQUIRE(storage.size() == 0);

    GIVEN("Default PreSyn SoA rows") {
        PreSyn::owning_handle a{storage};
        PreSyn::owning_handle b{storage};
        THEN("Defaults match field tags") {
            REQUIRE(a.threshold() == 10.0);
            REQUIRE(a.gid() == -1);
            REQUIRE(a.nc_index() == -1);
            REQUIRE(a.nc_count() == 0);
            REQUIRE(a.output_index() == -1);
            REQUIRE(a.thvar_row() == -1);
            REQUIRE(a.thread_id() == -1);
            REQUIRE(storage.size() == 2);
        }
        THEN("Fanout range fields survive reverse permutation") {
            // Simulate CoreNEURON-style contiguous fanout ranges in a global order.
            a.nc_index() = 0;
            a.nc_count() = 3;
            a.threshold() = -20.;
            a.gid() = 7;
            b.nc_index() = 3;
            b.nc_count() = 2;
            b.gid() = 11;
            {
                auto token = storage.issue_frozen_token();
                storage.mark_as_sorted(token);
            }
            std::vector<std::size_t> perm{0, 1};
            std::rotate(perm.begin(), std::next(perm.begin()), perm.end());
            storage.apply_reverse_permutation(std::move(perm));
            REQUIRE(a.nc_index() == 0);
            REQUIRE(a.nc_count() == 3);
            REQUIRE(a.threshold() == -20.);
            REQUIRE(a.gid() == 7);
            REQUIRE(b.nc_index() == 3);
            REQUIRE(b.nc_count() == 2);
            REQUIRE(b.gid() == 11);
        }
    }
}

TEST_CASE("Network SoA sort partitions PointProcess by thread",
          "[Neuron][data_structures][network][sort]") {
    auto& pp = neuron::model().point_processes();
    auto& w = neuron::model().weights();
    auto& nc = neuron::model().netcons();
    auto& ps = neuron::model().presyns();

    // Intentionally create in non-thread order: tid 1, 0, 1, 0
    std::vector<PointProcess::owning_handle> rows;
    rows.emplace_back(pp);
    rows.back().thread_id() = 1;
    rows.back().instance() = 10;
    rows.emplace_back(pp);
    rows.back().thread_id() = 0;
    rows.back().instance() = 20;
    rows.emplace_back(pp);
    rows.back().thread_id() = 1;
    rows.back().instance() = 11;
    rows.emplace_back(pp);
    rows.back().thread_id() = 0;
    rows.back().instance() = 21;

    auto w0 = Weight::allocate_weight_rows(1, nullptr);
    w0[0].value() = 1.0;
    Weight::owning_handle orphan{w};
    orphan.value() = 99.0;
    auto w1 = Weight::allocate_weight_rows(1, nullptr);
    w1[0].value() = 2.0;

    extern int nrn_nthread;
    {
        auto pp_tok = pp.issue_frozen_token();
        auto w_tok = w.issue_frozen_token();
        auto nc_tok = nc.issue_frozen_token();
        auto ps_tok = ps.issue_frozen_token();

        neuron::cache::Model cache{};
        cache.thread.resize(static_cast<std::size_t>(std::max(nrn_nthread, 1)));

        neuron::container::network::sort_network_data(cache, pp_tok, w_tok, nc_tok, ps_tok);

        REQUIRE(pp.is_sorted());
        REQUIRE(w.is_sorted());
        REQUIRE(nc.is_sorted());
        REQUIRE(ps.is_sorted());

        // Handles keep logical values after partition.
        REQUIRE(rows[0].thread_id() == 1);
        REQUIRE(rows[0].instance() == 10);
        REQUIRE(rows[1].thread_id() == 0);
        REQUIRE(rows[1].instance() == 20);

        if (nrn_nthread >= 2) {
            // Storage order: thread 0 rows then thread 1 rows.
            REQUIRE(pp.get<PointProcess::field::ThreadId>(0) == 0);
            REQUIRE(pp.get<PointProcess::field::ThreadId>(1) == 0);
            REQUIRE(pp.get<PointProcess::field::ThreadId>(2) == 1);
            REQUIRE(pp.get<PointProcess::field::ThreadId>(3) == 1);
            REQUIRE(cache.thread[0].point_process_offset == 0);
            REQUIRE(cache.thread[1].point_process_offset == 2);
        }
    }  // freeze tokens released before destroying owning handles
}

TEST_CASE("nrn_ensure_model_data_are_sorted freezes network containers",
          "[Neuron][data_structures][network][sort]") {
    // Needs a minimal model (same precondition as other ensure_sorted unit tests).
    REQUIRE(hoc_oc("create s\nfinitialize(-65)\n") == 0);
    neuron::model().point_processes().mark_as_unsorted();
    neuron::model().weights().mark_as_unsorted();
    neuron::model().netcons().mark_as_unsorted();
    neuron::model().presyns().mark_as_unsorted();
    {
        auto token = nrn_ensure_model_data_are_sorted();
        REQUIRE(neuron::model().point_processes().is_sorted());
        REQUIRE(neuron::model().weights().is_sorted());
        REQUIRE(neuron::model().netcons().is_sorted());
        REQUIRE(neuron::model().presyns().is_sorted());
        REQUIRE(neuron::cache::model);
        REQUIRE_FALSE(token.point_process_tokens.empty());
        REQUIRE_FALSE(token.weight_tokens.empty());
        REQUIRE_FALSE(token.netcon_tokens.empty());
        REQUIRE_FALSE(token.presyn_tokens.empty());
    }  // release freeze before structural cleanup
    REQUIRE(hoc_oc("delete_section()\n") == 0);
}
