#include <algorithm>
#include <vector>

#include "oc_ansi.h"

#include <catch2/catch.hpp>

// This function is the one that is used in all nrn-modeldb-ci
// Keep as is
int cmpdfn(double a, double b) {
    return ((a) <= (b)) ? (((a) == (b)) ? 0 : -1) : 1;
}

TEST_CASE("Test nrn_mlh_gsort output", "[nrn_gsort]") {
    std::vector<double> input{1.2, -2.5, 5.1};

    {
        std::vector<int> indices(input.size());
        // all values from 0 to size - 1
        std::iota(indices.begin(), indices.end(), 0);

        // for comparison
        auto sorted_input = input;
        std::sort(sorted_input.begin(), sorted_input.end());

        SECTION("Test sorting") {
            nrn_mlh_gsort(input.data(), indices.data(), input.size(), cmpdfn);
            for (auto i = 0; i < input.size(); ++i) {
                REQUIRE(sorted_input[i] == input[indices[i]]);
            }
        }
    }

    {
        std::vector<int> indices{2, 1, 1};
        std::vector<int> expected_result{1, 1, 2};  // as -2,5 < 5.1

        SECTION("Test sorting with repeated indices") {
            nrn_mlh_gsort(input.data(), indices.data(), input.size(), cmpdfn);
            for (auto i = 0; i < input.size(); ++i) {
                REQUIRE(indices[i] == expected_result[i]);
            }
        }
    }
}
