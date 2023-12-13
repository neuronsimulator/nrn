#include <vector>

#include "utils/enumerate.h"

#include <catch2/catch.hpp>


TEST_CASE("apply_to_first", "[Neuron]") {
    std::vector<double> x{1.0, 2.0, 2.0, 3.0};

    apply_to_first(x, 2.0, [](auto it) { *it = 5.0; });
    REQUIRE(x == std::vector<double>({1.0, 5.0, 2.0, 3.0}));
}

TEST_CASE("erase_first", "[Neuron]") {
    std::vector<double> x{1.0, 2.0, 2.0, 3.0};

    erase_first(x, 2.0);
    REQUIRE(x == std::vector<double>({1.0, 2.0, 3.0}));
}

TEST_CASE("reverse", "[Neuron]") {
    std::vector<double> x{1.0, 2.0, 3.0};

    for (auto& i: reverse(x)) {
        i *= -1.0;
    }
    REQUIRE(x == std::vector<double>({-1.0, -2.0, -3.0}));
}

TEST_CASE("reverse; no-copy", "[Neuron]") {
    std::vector<double> x{1.0, 2.0, 3.0};

    auto reverse_iterable = reverse(x);

    for (auto& xx: x) {
        xx *= -1.0;
    }

    for (const auto& xx: reverse_iterable) {
        REQUIRE(xx < 0.0);
    }
}

TEST_CASE("range", "[Neuron]") {
    std::vector<double> x{1.0, 2.0, 3.0};

    std::vector<std::size_t> v{};
    for (std::size_t i: range(x)) {
        v.push_back(i);
    }
    REQUIRE(v == std::vector<size_t>{0, 1, 2});
}

TEST_CASE("enumerate", "[Neuron]") {
    std::vector<double> x{1.0, 2.0, 3.0};

    int j = 0;
    for (auto&& [i, elem]: enumerate(x)) {
        if (i == 0)
            REQUIRE(elem == 1.0);
        if (i == 1)
            REQUIRE(elem == 2.0);
        if (i == 2)
            REQUIRE(elem == 3.0);
        REQUIRE(i == j);
        ++j;
    }
}

TEST_CASE("renumerate", "[Neuron]") {
    std::vector<double> x{1.0, 2.0, 3.0};

    int j = x.size() - 1;
    for (auto&& [i, elem]: renumerate(x)) {
        if (i == 0)
            REQUIRE(elem == 1.0);
        if (i == 1)
            REQUIRE(elem == 2.0);
        if (i == 2)
            REQUIRE(elem == 3.0);
        REQUIRE(i == j);
        --j;
    }
}
