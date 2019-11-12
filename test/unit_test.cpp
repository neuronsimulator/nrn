#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>

extern "C" {
#include "../src/oc/ocfunc.h"
#include "../src/oc/code.h"
#include "../src/nrnoc/section.h"
#include "../src/nrnoc/neuron.h"

extern void nrn_threads_create(int, int);
extern void nrn_threads_free();
}

/// Needed for compilation
void modl_reg() { }

TEST_CASE("Test hoc interpreter", "[Neuron][hoc_interpreter]") {
    hoc_init_space();
    hoc_pushx(4.0);
    hoc_pushx(5.0);
    hoc_add();
    REQUIRE( hoc_xpop() == 9.0 );
}

SCENARIO("Test fast_imem calculation", "[Neuron][fast_imem]") {
    GIVEN("An NrnThread") {
        nrn_threads_create(1, 0);
        for(int it = 0; it < nrn_nthread; ++it) {
            NrnThread* nt = &nrn_threads[it];
            nt->end = 1;
            nt->_actual_rhs = (double*)ecalloc(nt->end, sizeof(double));
            nt->_actual_d = (double*)ecalloc(nt->end, sizeof(double));
            nt->_actual_area = (double*)ecalloc(nt->end, sizeof(double));
            for(int i = 0; i < nt->end; i++) {
                nt->_actual_rhs[i] = 0.1;
                nt->_actual_d[i] = 0.1;
                nt->_actual_area[i] = 0.1;
            }
        }

        WHEN("fast_imem and cachevec is enabled") {
            nrn_use_fast_imem = true;
            use_cachevec = 1;
        }

        AND_WHEN("fast_mem is allocated") {
            nrn_fast_imem_alloc();
            for(int it = 0; it < nrn_nthread; ++it) {
                NrnThread *nt = &nrn_threads[it];
                REQUIRE(nt->_nrn_fast_imem != nullptr);
            }

            THEN("calculate fast_imem") {
                for(int it = 0; it < nrn_nthread; ++it) {
                    NrnThread* nt = &nrn_threads[it];
                    for(int i = 0; i < nt->end; i++) {
                        nt->_nrn_fast_imem->_nrn_sav_rhs[i] = 0.1;
                        nt->_nrn_fast_imem->_nrn_sav_d[i] = 0.1;
                    }
                    nrn_calc_fast_imem(nt);
                }
                REQUIRE( nrn_threads[0]._nrn_fast_imem->_nrn_sav_rhs[0] == Approx( 0.00011 ) );
            }

            AND_THEN("clear NrnThreads") {
                nrn_threads_free();

                for (int it = 0; it < nrn_nthread; ++it) {
                    NrnThread *nt = &nrn_threads[it];
                    REQUIRE( nt->_ml_list == nullptr );
                    REQUIRE( nt->tml == nullptr );
                    for(int i=0; i < BEFORE_AFTER_SIZE; ++i) {
                        REQUIRE( nt->tbl[i] == nullptr );
                    }
                    REQUIRE( nt->_ecell_memb_list == nullptr );
                    REQUIRE( nt->_ecell_children == nullptr );
                    REQUIRE( nt->_sp13mat == nullptr );
                    REQUIRE( nt->_nrn_fast_imem == nullptr );
                    REQUIRE( nt->_actual_v == nullptr );
                    REQUIRE( nt->_actual_area == nullptr );
                    REQUIRE( nt->end == 0 );
                    REQUIRE( nt->ncell == 0 );
                    REQUIRE( nt->_vcv == nullptr );
                }

                free(nrn_threads);
                nrn_threads = nullptr;
                nrn_nthread = 0;

                REQUIRE( nrn_threads == nullptr );
                REQUIRE( nrn_nthread == 0 );
            }
        }
    }
}
