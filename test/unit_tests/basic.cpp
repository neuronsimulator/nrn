#include "code.h"
#include "neuron.h"
#include "ocfunc.h"
#include "section.h"

#include <catch2/catch.hpp>

SCENARIO("Test fast_imem calculation", "[Neuron][fast_imem]") {
    GIVEN("A section") {
        REQUIRE(hoc_oc("create s\n") == 0);
        WHEN("fast_imem and cachevec is allocated") {
            nrn_use_fast_imem = true;
            use_cachevec = 1;
            nrn_fast_imem_alloc();
            THEN("nrn_fast_imem should not be nullptr") {
                for (int it = 0; it < nrn_nthread; ++it) {
                    NrnThread* nt = &nrn_threads[it];
                    REQUIRE(nt->_nrn_fast_imem != nullptr);
                }
            }
        }

        WHEN("fast_imem is created") {
            REQUIRE(hoc_oc("objref cvode\n"
                           "cvode = new CVode()\n"
                           "cvode.use_fast_imem(1)\n") == 0);
            WHEN("iinitialize and run nrn_calc_fast_imem") {
                hoc_oc("finitialize(-65)\n");
                for (NrnThread* nt = nrn_threads; nt < nrn_threads + nrn_nthread; ++nt) {
                    nrn_calc_fast_imem(nt);
                }
                THEN("The current in this section is 0") {
                    for (NrnThread* nt = nrn_threads; nt < nrn_threads + nrn_nthread; ++nt) {
                        for (int i = 0; i < nt->end; ++i) {
                            REQUIRE(nt->_nrn_fast_imem->_nrn_sav_rhs[i] == 0.0);
                        }
                    }
                }
            }
        }

        REQUIRE(hoc_oc("delete_section()") == 0);
    }
}

TEST_CASE("Test return code of execerror", "[NEURON][execerror]") {
    REQUIRE(hoc_oc("execerror(\"test error\")") > 0);
}

// AddressSanitizer seems to intercept the mallinfo[2]() system calls and return
// null values from them.
#ifndef NRN_ASAN_ENABLED
TEST_CASE("Test nrn_mallinfo returns non-zero", "[NEURON][nrn_mallinfo]") {
    SECTION("HOC") {
        REQUIRE(
            hoc_oc(
                "if (nrn_mallinfo(0) <= 0) { execerror(\"nrn_mallinfo returned <= 0 in HOC\") }") ==
            0);
    }
    SECTION("C++") {
        REQUIRE(nrn_mallinfo(0) > 0);
    }
}
#endif
