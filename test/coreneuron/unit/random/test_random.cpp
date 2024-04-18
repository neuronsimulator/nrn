/*
# =============================================================================
# Copyright (c) 2016 - 2024 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/
#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "coreneuron/utils/randoms/nrnran123.h"

using namespace coreneuron;

TEST_CASE("random123 smoke test") {
    const int KEY_1 = 1;
    const int KEY_2 = 2;
    const int NUM_STREAMS = 20;
    const int NUM_SAMPLES = 1000;
    nrnran123_State* s;
    //    const double EPSILON = 0.00001;

    /*
        // random123 is a fully deterministic PRNG; it should generate the following stream of
       doubles
        // with (1,2) initial key:

        double ref[] = {0.724128,  0.507056, 0.446231, 0.331509, 0.84732,  0.0942423, 0.5401,
                        0.0496035, 0.286901, 0.777649, 0.584096, 0.468876, 0.545774,  0.584586,
                        0.630361,  0.875837, 0.703636, 0.853706, 0.173552, 0.917358};
    */

    const int res_size = NUM_SAMPLES * NUM_STREAMS;
    double res[res_size];

//    nrn_pragma_omp(target teams distribute parallel for map(tofrom: res[0:res_size]) is_device_ptr(s))
//    nrn_pragma_acc(parallel loop copy(res [0:res_size]) deviceptr(s))
    for (int i = 0; i < NUM_STREAMS; i++) {
        s = nrnran123_newstream(KEY_1, i);
        nrnran123_setseq(s, 0, 0);

//        nrn_pragma_acc(loop seq)
        for (int j = 0; j < NUM_SAMPLES; j++) {
            double val = nrnran123_dblpick(s);
            res[i * NUM_SAMPLES + j] = val;
        }

        //        nrnran123_deletestream(s);
    }

    // there should be no duplicates
    std::set<double> check_set;

    for (int i = 0; i < NUM_STREAMS * NUM_SAMPLES; i++) {
        double d = res[i];
        size_t old_size = check_set.size();
        check_set.insert(res[i]);
        size_t new_size = check_set.size();

        if (old_size == new_size) {
            std::cerr << "Duplicate found! i = " << i << ", d = " << d << std::endl;
//            FAIL("Duplicate found!");
        }
        //        REQUIRE(delta < EPSILON);
    }
    REQUIRE(check_set.size() == NUM_SAMPLES * NUM_STREAMS);
}
