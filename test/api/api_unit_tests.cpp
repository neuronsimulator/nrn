#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>

#include "neuronapi.h"
#include "test_common.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>

extern "C" void modl_reg(){/* No modl_reg */};

TEST_CASE("Test calling API", "[NEURON][public-api]") {
    static const char* argv[] = {"netcon", "-nogui", "-nopython", nullptr};
    nrn_init(3, argv);

    auto r = nrn_function_call("unix_mac_pc", NRN_NO_ARGS);
    REQUIRE(r.d >= 1);

    // experiment with returned objects
    auto tv = nrn_object_new_NoArgs("Vector");
    r = nrn_method_call(tv, "indgen", "ddd", 10., 50., 5.);
    REQUIRE(r.type == NrnResultType::NRN_OBJECT);
    r = nrn_method_call(tv, "c", NRN_NO_ARGS);
    REQUIRE(r.type == NrnResultType::NRN_OBJECT);
    auto v_copy = nrn_result_get_object(&r);
    r = nrn_method_call(v_copy, "reverse", NRN_NO_ARGS);
    auto reversed = nrn_result_get_object(&r);
    r = nrn_method_call(reversed, "get", NRN_ARG_DOUBLE, 1.);
    int elem = nrn_result_get_double(&r);
    REQUIRE(elem == 45);

    // Some function types are not supported
    auto r2 = nrn_function_call("neuronhome", NRN_NO_ARGS);
    REQUIRE(r2.type == NrnResultType::NRN_ERR);
}
