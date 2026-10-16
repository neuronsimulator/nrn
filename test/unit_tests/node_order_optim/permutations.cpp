#include "node_order_optim/permute_utils.hpp"

#include <catch2/catch_test_macros.hpp>

#include <vector>

TEST_CASE("Permutation algorithms", "[Neuron][node_order_optim][permute]") {
    GIVEN("A vector of elements and a permutation vector") {
        const std::vector<int> values{49, 32, 17, 29};
        const std::vector<int> forward_permutation{3, 2, 0, 1};
        const std::vector<int> inverse_permutation{2, 3, 1, 0};
        THEN("forward_permute is done correctly") {
            std::vector<int> forward_permutted_values{29, 17, 49, 32};
            auto values_copy{values};
            forward_permute(values_copy, forward_permutation);
            REQUIRE(values_copy == forward_permutted_values);
        }
        THEN("Inversion of the permute vector is done correctly") {
            REQUIRE(inverse_permute_vector(forward_permutation) == inverse_permutation);
            /// Make sure that pinv[p[i]] = i = p[pinv[i]]
            const std::vector<int> index{0, 1, 2, 3};
            auto permutation_copy{forward_permutation};
            forward_permute(permutation_copy, inverse_permutation);
            REQUIRE(permutation_copy == index);
            auto inverse_permutation_copy{inverse_permutation};
            forward_permute(inverse_permutation_copy, forward_permutation);
            REQUIRE(inverse_permutation_copy == index);
        }
    }
}
