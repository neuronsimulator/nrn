#include "neuron/container/network/point_process.hpp"
#include "neuron/container/network/weights.hpp"
#include "neuron/model_data.hpp"
#include "section.h"

#include <catch2/catch_test_macros.hpp>

#include <numeric>
#include <optional>
#include <random>
#include <vector>

using namespace neuron::container::network;

TEST_CASE("SOA-backed PointProcess structure", "[Neuron][data_structures][network][point_process]") {
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
}
