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
