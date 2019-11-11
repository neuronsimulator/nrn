#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>

extern "C" {
#include "../src/oc/ocfunc.h"
#include "../src/oc/code.h"
#include "../src/nrnoc/section.h"
#include "../src/nrnoc/neuron.h"
#include "../src/nrnoc/multicore.h"

#define CACHELINE_ALLOC(name, type, size) name = (type*)nrn_cacheline_alloc((void**)&name, size*sizeof(type))

void nrn_threads_create(int, int);
extern void nrn_fast_imem_alloc();
extern void nrn_matrix_node_alloc(void);
extern void nrn_calc_fast_imem(NrnThread *);
}

extern int nrn_nthread;
extern NrnThread* nrn_threads;

int nrn_use_fast_imem = true;
int use_cachevec = 1;

void modl_reg() {

}

TEST_CASE("Test hoc interpreter", "[Neuron][hoc_interpeter]") {
    hoc_init_space();
    hoc_pushx(4.0);
    hoc_pushx(5.0);
    hoc_add();
    REQUIRE( hoc_xpop() == 9.0);
}

TEST_CASE("Test fast_imem calculation", "[Neuron][fast_imem]") {
    INFO("Test starting");
    //CACHELINE_ALLOC(nrn_threads, NrnThread, nthreads);
    INFO("nrn_nthread = " << nrn_nthread);
    nrn_threads_create(1, 0);
    INFO("nrn_nthread = " << nrn_nthread);
    INFO("nrn_threads = " << nrn_threads);
    INFO("nrn_threads + nrn_nthread = " << nrn_threads + nrn_nthread);
    for(NrnThread* nt = nrn_threads; nt < nrn_threads + nrn_nthread; ++nt) {
        INFO("nt = " << nt);
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
    INFO("NrnThread setup done");
    nrn_fast_imem_alloc();
    INFO("nrn_fast_imem_alloc setup done");
    for(NrnThread* nt = nrn_threads; nt < nrn_threads + nrn_nthread; ++nt) {
        INFO("nt = " << nt);
        nrn_calc_fast_imem(nt);
        for(int i = 0; i < nt->end; i++) {
            INFO("i = " << i);
            INFO("nt->_nrn_fast_imem->_nrn_sav_d[i] = " << nt->_nrn_fast_imem->_nrn_sav_d[i]);
            CHECK(nt->_nrn_fast_imem->_nrn_sav_d[i] != 0.1);
        }
    }
    REQUIRE( 0 == 1 );
}
