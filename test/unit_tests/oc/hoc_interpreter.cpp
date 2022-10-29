#include <catch2/catch.hpp>

#include <hocdec.h>
#include <ocfunc.h>
#include <code.h>

TEST_CASE("Test hoc interpreter", "[Neuron][hoc_interpreter]") {
    hoc_init_space();
    hoc_pushx(4.0);
    hoc_pushx(5.0);
    hoc_add();
    REQUIRE(hoc_xpop() == 9.0);
}

SCENARIO("Test small HOC functions", "[NEURON][hoc_interpreter]") {
    // This is targeting AddressSanitizer errors that were seen in #1995
    GIVEN("Test calling a function that returns an Object that lived on the stack") {
        constexpr auto vector_size = 5;
        REQUIRE(hoc_oc(("obfunc foo() {localobj lv1\n"
                        "  lv1 = new Vector(" +
                        std::to_string(vector_size) +
                        ")\n"
                        "  lv1.indgen()\n"
                        "  return lv1\n"
                        "}\n")
                           .c_str()) == 0);
        WHEN("It is called") {
            REQUIRE(hoc_oc("objref v1\n"
                           "v1 = foo()\n") == 0);
            THEN("The returned value should be correct") {
                auto const i = GENERATE_COPY(range(1, vector_size));
                REQUIRE(hoc_oc(("hoc_ac_ = v1.x[" + std::to_string(i) + "]\n").c_str()) == 0);
                REQUIRE(hoc_ac_ == i);
            }
        }
    }
}
