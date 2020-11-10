#define CATCH_CONFIG_RUNNER

#include <catch2/catch.hpp>

//extern "C" {
#include <ocfunc.h>
#include <code.h>
#include <section.h>
#include <neuron.h>

extern void nrn_threads_create(int, int);
extern void nrn_threads_free();
extern int ivocmain_session(int, const char**, const char**, int);

extern int nrn_main_launch;
extern int nrn_nobanner_;
//} // extern "C"

/// Needed for compilation
void modl_reg() { }
extern int nrn_nthread;
extern NrnThread* nrn_threads;

extern int nrn_use_fast_imem;
extern int use_cachevec;

int main( int argc, char* argv[] ) {
    // global setup...
    nrn_main_launch = 2;
    int argc_nompi = 2;
    const char* argv_nompi[] = {"NEURON", "-nogui"};
    nrn_nobanner_ = 1;

    ivocmain_session(argc_nompi, argv_nompi, NULL, 0);
#undef run
    int result = Catch::Session().run( argc, argv );
#define run hoc_run
    // global clean-up...

    return result;
}


SCENARIO("Test fast_imem calculation", "[Neuron][fast_imem]") {
    GIVEN("A section") {

        hoc_oc("create s\n");

        WHEN("fast_imem and cachevec is allocated") {
            nrn_use_fast_imem = true;
            use_cachevec = 1;
            nrn_fast_imem_alloc();
            THEN("nrn_fast_imem should not be nullptr") {
                for(int it = 0; it < nrn_nthread; ++it) {
                    NrnThread *nt = &nrn_threads[it];
                    REQUIRE(nt->_nrn_fast_imem != nullptr);
                }
            }
        }

        WHEN("fast_imem is created") {
            hoc_oc("objref cvode\n"
                   "cvode = new CVode()\n"
                   "cvode.use_fast_imem(1)\n");
            WHEN("iinitialize and run nrn_calc_fast_imem") {
                hoc_oc("finitialize(-65)\n");
                for(NrnThread* nt = nrn_threads; nt < nrn_threads + nrn_nthread; ++nt) {
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

        hoc_oc("delete_section()");
    }
}
