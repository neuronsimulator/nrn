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
                auto const model_cache = neuron::acquire_valid_model_cache();
                THEN("The data structures should be frozen") {
                    REQUIRE(node_data.is_frozen());
                    AND_THEN("The data structures should be sorted") {
                        REQUIRE(node_data.is_sorted());
                    }
                }
                THEN("") {}
            }
            AND_THEN("When the cache token goes out of scope") {
                AND_THEN("The data structures should still be sorted") {
                    REQUIRE(node_data.is_sorted());
                }
                AND_THEN("The data structures should no longer be frozen") {
                    REQUIRE_FALSE(node_data.is_frozen());
                }
            }
        }
    }
}
