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

SCENARIO("Test small HOC functions", "[Neuron][hoc_interpreter]") {
    GIVEN("A HOC function returning a local object") {
        REQUIRE(hoc_oc("obfunc foo() {localobj lv1\n"
                       "  lv1 = new Vector(5)\n"
                       "  lv1.indgen()\n"
                       "  return lv1\n"
                       "}\n") == 0);
        WHEN("It is called") {
            auto const code = hoc_oc(
                "objref v1\n"
                "v1 = foo()\n");
            THEN("There should be no error") {
                REQUIRE(code == 0);
                AND_THEN("Printing the result should give the expected result") {
                    REQUIRE(hoc_oc("print v1\n") == 0);
                    REQUIRE(hoc_oc("v1.printf()\n") == 0);
                }
            }
        }
    }
}
