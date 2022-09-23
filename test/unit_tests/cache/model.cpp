#include "neuron/cache/model_data.hpp"
// Need this for the implementation of rotate
#include "neuron/container/soa_container_impl.hpp"
#include "../model_test_utils.hpp"

#include <catch2/catch.hpp>

namespace {
std::vector<double const*> get_reference_voltage_pointers(
    neuron::container::Node::storage const& node_data) {
    std::vector<double const*> reference_ptrs{};
    auto const& voltage_storage =
        std::as_const(node_data).get<neuron::container::Node::field::Voltage>();
    std::transform(voltage_storage.begin(),
                   voltage_storage.end(),
                   std::back_inserter(reference_ptrs),
                   [](auto& v) { return &v; });
    return reference_ptrs;
}
}  // namespace

TEST_CASE("Cached model data", "[Neuron][data_structures][cache]") {
    GIVEN("A list of nodes with integer voltages and permuted storage") {
        auto nodes_and_reference_voltages = neuron::test::get_nodes_and_reference_voltages();
        auto& nodes = std::get<0>(nodes_and_reference_voltages);
        auto& reference_voltages = std::get<1>(nodes_and_reference_voltages);
        auto& node_data = neuron::model().node_data();
        node_data.rotate(node_data.size() / 2);
        auto const assert_frozen_and_sorted = [&node_data](bool frozen, bool sorted) {
            THEN((sorted ? "The data are sorted" : "The data are not sorted")) {
                REQUIRE(node_data.is_sorted() == sorted);
                AND_THEN((frozen ? "The data are frozen" : "The data are not frozen")) {
                    REQUIRE(node_data.is_frozen() == frozen);
                }
            }
        };
        assert_frozen_and_sorted(false, false);
        WHEN("We decree that the data are sorted") {
            {
                auto sorted_token = node_data.get_sorted_token();
                assert_frozen_and_sorted(true, true);
            }
            // sorted_token went out of scope, but in this unit test we know that
            // nothing has changed since then -- so the data are still sorted
            AND_WHEN("The sorted token goes out of scope") {
                assert_frozen_and_sorted(false, true);
            }
            AND_WHEN("Cached data is requested for the model") {
            {
                // This is not a toy implementation and would call NEURON's real
                // sorting algorithm if the data were not already "sorted"
                // above. The real algorithm depends on other global state that
                // is not set up in the unit tests.
                auto const model_cache = neuron::acquire_valid_model_cache();
                assert_frozen_and_sorted(true, true);
                THEN("The cache data should match the model data") {
                    // These should point to the entries of the voltage storage
                    auto const& cache_ptrs = model_cache->node_data.voltage_ptrs;
                    // Re-calculate the reference values
                    auto const reference_ptrs = get_reference_voltage_pointers(node_data);
                    // Check that the vector we created on the fly contains the
                    // same pointers as the vector in the model cache
                    REQUIRE(std::equal(cache_ptrs.begin(),
                                       cache_ptrs.end(),
                                       reference_ptrs.begin(),
                                       reference_ptrs.end()));
                }
                AND_WHEN("There is a nested request for cached data") {
                    auto const other_model_cache = neuron::acquire_valid_model_cache();
                    assert_frozen_and_sorted(true, true);
                }
                AND_WHEN("The nested cache goes out of scope") {
                    assert_frozen_and_sorted(true, true);
                }
            }
            AND_WHEN("The cache token goes out of scope") {
                assert_frozen_and_sorted(false, true);
            }
        }
        }
    }
    GIVEN("Some nodes with integer voltages") {
        auto& node_data = neuron::model().node_data();
        auto [nodes, reference_voltages] = neuron::test::get_nodes_and_reference_voltages();
        WHEN("Some nodes are added and deleted") {
            auto const i = GENERATE(3, 5, 7);
            // add some new nodes to the end
            for (auto j = 0; j < i * i; ++j) {
                nodes.emplace_back();
            }
            // erase some near the middle
            for (auto j = 0; j < i; ++j) {
                nodes.erase(std::next(nodes.begin(), nodes.size() / 2));
            }
            THEN("The storage should not be sorted") {
                REQUIRE_FALSE(node_data.is_sorted());
            }
            // permute the underlying storage
            node_data.rotate(node_data.size() / 2);
            AND_THEN("After permutation the storage should still not be sorted") {
                REQUIRE_FALSE(node_data.is_sorted());
            }
            // mark it as sorted to avoid calling the "real" sort algorithm
            { auto const token = node_data.get_sorted_token(); }
            AND_THEN("The cache data should be correct") {
                // get cache data for the newly permuted model
                auto const model_cache = neuron::acquire_valid_model_cache();
                auto const& cache_ptrs = model_cache->node_data.voltage_ptrs;
                auto const reference_ptrs = get_reference_voltage_pointers(node_data);
                REQUIRE(std::equal(cache_ptrs.begin(),
                                   cache_ptrs.end(),
                                   reference_ptrs.begin(),
                                   reference_ptrs.end()));
            }
        }
    }
}
