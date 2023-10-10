#include <../../nrnconf.h>
#include "code.h"
#include "neuron.h"
#include "ocfunc.h"
#include "section.h"
#if HAVE_IV
#include "ivoc.h"
#endif

#include <catch2/catch.hpp>

SCENARIO("Test fast_imem calculation", "[Neuron][fast_imem]") {
    GIVEN("A section") {
        REQUIRE(hoc_oc("create s\n") == 0);
        WHEN("fast_imem is allocated") {
            nrn_use_fast_imem = true;
            nrn_fast_imem_alloc();
            THEN("nrn_fast_imem should not be nullptr") {
                for (int it = 0; it < nrn_nthread; ++it) {
                    REQUIRE(nrn_threads[it].node_sav_d_storage());
                    REQUIRE(nrn_threads[it].node_sav_rhs_storage());
                }
            }
        }

        WHEN("fast_imem is created") {
            REQUIRE(hoc_oc("objref cvode\n"
                           "cvode = new CVode()\n"
                           "cvode.use_fast_imem(1)\n") == 0);
            WHEN("iinitialize and run nrn_calc_fast_imem") {
                REQUIRE(hoc_oc("finitialize(-65)\n") == 0);
                for (NrnThread* nt = nrn_threads; nt < nrn_threads + nrn_nthread; ++nt) {
                    nrn_calc_fast_imem(nt);
                }
                THEN("The current in this section is 0") {
                    for (NrnThread* nt = nrn_threads; nt < nrn_threads + nrn_nthread; ++nt) {
                        auto const vec_sav_rhs = nt->node_sav_rhs_storage();
                        for (int i = 0; i < nt->end; ++i) {
                            REQUIRE(vec_sav_rhs[i] == 0.0);
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

#if HAVE_IV
TEST_CASE("Test Oc::run(cmd)", "[NEURON]") {
    Oc oc;
    REQUIRE(oc.run("foo", 1) == 1);
    REQUIRE(oc.run("foo", 0) == 1);
}
#endif

// AddressSanitizer seems to intercept the mallinfo[2]() system calls and return
// null values from them. ThreadSanitizer seems to do the same.
#if !defined(NRN_ASAN_ENABLED) && !defined(NRN_TSAN_ENABLED)
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
