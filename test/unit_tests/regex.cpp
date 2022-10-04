#include <catch2/catch.hpp>

#include <iostream>
#include <memory>
#include <string_view>

// This does lots of macro hackery, not safe to include it before other headers...
#include <InterViews/regexp.h>

// Make sure that there is no short string optimisation and we allocate exactly
// the right size of heap buffer plus `pad_before` bytes at the beginning
std::unique_ptr<char[]> string_as_char_array(std::string_view string, std::size_t pad_before = 0) {
    auto const size = pad_before + string.size() + 1;
    auto ret = std::make_unique<char[]>(size);
    std::fill(ret.get(), std::next(ret.get(), pad_before), '\0');
    std::copy(string.begin(), string.end(), std::next(ret.get(), pad_before));
    ret[size - 1] = '\0';
    return ret;
}

SCENARIO("Test regular expression utilities", "[NEURON][InterViews][Regex]") {
    // This is targetting AddressSantizer errors that were seen when executing
    // https://github.com/ModelDBRepository/136095
    GIVEN("A string and pattern from ModelDB ID 136095") {
        constexpr auto pattern = "^[do]\\[";
        constexpr auto* const text_cstr = "d[1][1]";
        constexpr std::string_view text{text_cstr};
        Regexp regex{pattern};
        auto const padding = GENERATE(1, 0);
        WHEN("We use a heap-allocated buffer with " + std::to_string(padding) +
             " valid characters before the start of the string") {
            auto const array = string_as_char_array(text, padding);
            THEN("The returned value should be correct") {
                REQUIRE(
                    regex.Search(std::next(array.get(), padding), text.size(), 0, text.size()) ==
                    0);
            }
        }
    }
}
