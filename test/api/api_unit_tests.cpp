#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "neuronapi.h"
#include "test_common.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>


TEST_CASE( "Test calling API", "[NEURON][public-api]" ) {

    static const char* argv[] = {"netcon", "-nogui", "-nopython", nullptr};
    nrn_init(3, argv);

    auto x = nrn_function_call("unix_mac_pc", NRN_NO_ARGS);
    REQUIRE(x >= 1);

    auto y = nrn_function_call("nrnversion", NRN_NO_ARGS);

    // experiment with returned objects
    auto tv = nrn_object_new_NoArgs("Vector");
    nrn_method_call(tv, "indgen", "ddd", 10., 50., 5.);
    auto other_v_res = nrn_method_call(tv, "c", NRN_NO_ARGS);
    auto other_v = nrn_result_get_object(&other_v_res);
    nrn_method_call(other_v, "reverse", NRN_NO_ARGS);
    auto res = nrn_method_call(other_v, "get", NRN_ARG_DOUBLE, 1.);
    int elem = nrn_result_get_double(&res);
    REQUIRE(elem == 45);
}
