#include "neuron/cache/model_data.hpp"
// Need this for the implementation of rotate
#include "neuron/container/soa_container_impl.hpp"
#include "../model_test_utils.hpp"

#include <catch2/catch.hpp>

TEST_CASE("Cached model data", "[Neuron][data_structures][cache]") {
    GIVEN("A list of nodes with integer voltages and permuted storage") {
        auto nodes_and_reference_voltages = neuron::test::get_nodes_and_reference_voltages();
        auto& nodes = std::get<0>(nodes_and_reference_voltages);
        auto& reference_voltages = std::get<1>(nodes_and_reference_voltages);
        auto& node_data = neuron::model().node_data();
        node_data.rotate(node_data.size() / 2);
        REQUIRE_FALSE(node_data.is_sorted());
        {
            // Mark the data as sorted
            auto sorted_token = node_data.get_sorted_token();
            REQUIRE(node_data.is_sorted());
        }
        // sorted_token went out of scope, but in this unit test we know that
        // nothing has changed since then -- so the data are still sorted
        REQUIRE(node_data.is_sorted());
        WHEN("Cached data is requested for the model") {
            {
                // This is not a toy implementation and would call NEURON's real
                // sorting algorithm if the data were not already "sorted"
                // above. The real algorithm depends on other global state that
                // is not set up in the unit tests.
                auto const model_cache = neuron::acquire_valid_model_cache();
                THEN("The data structures should be frozen") {
                    REQUIRE(node_data.is_frozen());
                    AND_THEN("The data structures should be sorted") {
                        REQUIRE(node_data.is_sorted());
                    }
                }
                AND_THEN("The cache data should match the model data") {
                    // These should point to the entries of the voltage storage
                    auto const& voltage_ptrs = model_cache->node_data.voltage_ptrs;
                    // Re-calculate the reference values
                    std::vector<double const*> reference_ptrs{};
                    auto const& voltage_storage =
                        std::as_const(node_data).get<neuron::container::Node::field::Voltage>();
                    std::transform(voltage_storage.begin(),
                                   voltage_storage.end(),
                                   std::back_inserter(reference_ptrs),
                                   [](auto& v) { return &v; });
                    // Check that the vector we created on the fly contains the
                    // same pointers as the vector in the model cache
                    REQUIRE(std::equal(voltage_ptrs.begin(),
                                       voltage_ptrs.end(),
                                       reference_ptrs.begin(),
                                       reference_ptrs.end()));
                }
            }
            AND_WHEN("The cache token goes out of scope") {
                THEN("The data structures should still be sorted") {
                    REQUIRE(node_data.is_sorted());
                }
                AND_THEN("The data structures should no longer be frozen") {
                    REQUIRE_FALSE(node_data.is_frozen());
                }
            }
        }
    }
}
