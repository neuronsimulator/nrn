#define CATCH_CONFIG_RUNNER

#include <catch2/catch.hpp>

#include <ocfunc.h>
#include <code.h>
#include <section.h>
#include <neuron.h>

#include <nrnmpi.h>

extern int ivocmain_session(int, const char**, const char**, int);

extern int nrn_main_launch;
extern int nrn_nobanner_;

/// Needed for compilation
extern "C" void modl_reg() {}
extern int nrn_nthread;
extern NrnThread* nrn_threads;
#if NRNMPI_DYNAMICLOAD
extern void nrnmpi_stubs();
#endif

extern int nrn_how_many_processors();
namespace nrn::test {
int PROCESSORS{0};
int MAX_PROCESSORS{nrn_how_many_processors()};
}  // namespace nrn::test

int main(int argc, char* argv[]) {
    // global setup...
    nrn_main_launch = 2;
    int argc_nompi = 2;
    const char* argv_nompi[] = {"NEURON", "-nogui", nullptr};
    nrn_nobanner_ = 1;

#if NRNMPI
    if (!nrnmpi_use) {
#if NRNMPI_DYNAMICLOAD
        nrnmpi_stubs();
#endif
    }
#endif

    Catch::Session session{};

    using namespace Catch::clara;
    auto cli = session.cli() |
               Opt(nrn::test::PROCESSORS, "number of PROCESSORS to consider")["--processors"](
                   "How many processors are available for perf tests");

    session.cli(cli);

    int returnCode = session.applyCommandLine(argc, argv);
    if (returnCode != 0)  // Indicates a command line error
        return returnCode;

    if (nrn::test::PROCESSORS > 0) {
        std::cout << "[cli][input] --processors=" << nrn::test::PROCESSORS << std::endl;
        nrn::test::MAX_PROCESSORS = std::min(nrn::test::PROCESSORS, nrn_how_many_processors());
    }
    std::cout << "MAX_PROCESSORS=" << nrn::test::MAX_PROCESSORS << std::endl;

    ivocmain_session(argc_nompi, argv_nompi, nullptr, 0);
#undef run
    return session.run();
}
