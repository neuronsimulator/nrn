#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>

#include "../src/oc/hoc.h"
#include "../src/oc/oc_ansi.h"
#include "../src/nrnoc/section.h"
#include "../src/nrnoc/neuron.h"
#include "../src/nrnoc/multicore.h"

#define CACHELINE_ALLOC(name,type,size) name = (type*)nrn_cacheline_alloc((void**)&name, size*sizeof(type))

extern void nrn_threads_create(int, int);
extern void nrn_fast_imem_alloc();
extern void nrn_matrix_node_alloc(void);
extern void nrn_calc_fast_imem(NrnThread*);

unsigned int Factorial( unsigned int number ) {
    return number <= 1 ? number : Factorial(number-1)*number;
}

int nthreads = 2;
int nrn_use_fast_imem = true;
NrnThread* nrn_threads = nullptr;
int use_cachevec = 1;

TEST_CASE( "Factorials are computed", "[factorial]" ) {
    REQUIRE( Factorial(1) == 1 );
    REQUIRE( Factorial(2) == 2 );
    REQUIRE( Factorial(3) == 6 );
    REQUIRE( Factorial(10) == 3628800 );
}


TEST_CASE("Test fast_imem calculation", "[Neuron][fast_imem]") {
    INFO("Test starting");
    CACHELINE_ALLOC(nrn_threads, NrnThread, nthreads);

    for(NrnThread* nt = nrn_threads; nt < nrn_threads + nthreads; ++nt) {
        nt->end = 10;
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
    for(NrnThread* nt = nrn_threads; nt < nrn_threads + nthreads; ++nt) {
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
