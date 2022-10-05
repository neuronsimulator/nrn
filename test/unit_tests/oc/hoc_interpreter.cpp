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
    // This is targeting errors in ModelDB 136095 with #1995
    GIVEN("An objref that gets redeclared with fewer indices") {
        REQUIRE(hoc_oc("objref ncl[1][1][1]\n"
                       "obfunc foo() { localobj x return ncl }\n"
                       "objref ncl\n"
                       "foo()\n") == 0);
    }
}
