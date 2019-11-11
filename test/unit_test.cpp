#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>

extern "C" {
#include "../src/oc/ocfunc.h"
#include "../src/oc/code.h"
#include "../src/nrnoc/section.h"
#include "../src/nrnoc/neuron.h"
#include "../src/nrnoc/multicore.h"

extern void nrn_threads_create(int, int);
extern void nrn_fast_imem_alloc();
extern void nrn_calc_fast_imem(NrnThread *);
}

extern int nrn_nthread;
extern NrnThread* nrn_threads;

int nrn_use_fast_imem = true;
int use_cachevec = 1;

void modl_reg() {

}

TEST_CASE("Test hoc interpreter", "[Neuron][hoc_interpreter]") {
    hoc_init_space();
    hoc_pushx(4.0);
    hoc_pushx(5.0);
    hoc_add();
    REQUIRE( hoc_xpop() == 9.0 );
}

TEST_CASE("Test fast_imem calculation", "[Neuron][fast_imem]") {
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
    nrn_fast_imem_alloc();
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
