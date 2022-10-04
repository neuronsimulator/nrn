#include <catch2/catch.hpp>

#include <string_view>

// This does lots of macro hackery, not safe to include it before other headers...
#include <InterViews/regexp.h>

// Make sure that there is no short string optimisation and we allocate exactly
// the right size of heap buffer
std::unique_ptr<char[]> string_as_char_array(std::string_view string) {
    auto ret = std::make_unique<char[]>(string.size() + 1);
    std::copy(string.begin(), string.end(), ret.get());
    ret[string.size()] = '\0';
    return ret;
}

SCENARIO("Test regular expression utilities", "[NEURON][InterViews][Regex]") {
    // This is targetting AddressSantizer errors that were seen when executing
    // https://github.com/ModelDBRepository/136095
    GIVEN("A string and pattern from ModelDB ID 136095") {
        constexpr auto pattern = "^[do]\\[";
        constexpr std::string_view text{"d[1][1]"};
        Regexp regex{pattern};
        auto const text_as_char_array = string_as_char_array(text);
        WHEN("We search for the pattern in the string") {
            regex.Search(text_as_char_array.get(), text.size(), 0, text.size());
            THEN("The returned value should be correct") {
                // dsfsd
            }
        }
    }
}
