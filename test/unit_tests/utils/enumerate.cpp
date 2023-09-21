// Must appear before `enumerate.h` otherwise, std::begin and std::rbegin are
// undefined. This should be fixed by including the correct headers in
// `utils/enumerate.h`.
#include <vector>

#include "utils/enumerate.h"

#include <catch2/catch.hpp>



TEST_CASE("reverse", "[Neuron]") {
  std::vector<double> x{1.0, 2.0, 3.0};

  // test/unit_tests/utils/enumerate.cpp:15:26: error: cannot bind non-const
  // lvalue reference of type ‘double&’ to an rvalue of type ‘double’
  // 15 |   for(auto& i : reverse(x)) {
  //    |                          ^

  // for(auto& i : reverse(x)) {
  //     i *= -1.0;
  // }

}

TEST_CASE("reverse; no-copy", "[Neuron]") {
  std::vector<double> x{1.0, 2.0, 3.0};

  auto reverse_iterable = reverse(x);

  for(auto& xx : x) {
    xx *= -1.0;
  }

  size_t i = 0;
  for(const auto& xx : reverse_iterable) {
      REQUIRE(xx < 0.0);
  }
}
