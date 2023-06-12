// Has to be included early to stop NEURON's macros wreaking havoc
#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

#include "code.h"
#include "neuron.h"
#include "nrnmpi.h"
#include "ocfunc.h"
#include "section.h"

#include <exception>

int ivocmain_session(int, const char**, const char**, int);
extern int nrn_main_launch;
extern int nrn_nobanner_;

/// Needed for compilation
extern "C" void modl_reg() {}
#if NRNMPI_DYNAMICLOAD
void nrnmpi_stubs();
#endif

extern int nrn_how_many_processors();
namespace nrn::test {
int PROCESSORS{0};
int MAX_PROCESSORS{nrn_how_many_processors()};
}  // namespace nrn::test

#ifdef NRN_COVERAGE_ENABLED
// works with AppleClang 14, other sources suggest __gcov_flush.
extern "C" void __gcov_dump();
#endif
namespace {
void new_handler() {
#ifdef NRN_COVERAGE_ENABLED
    __gcov_dump();
#endif
}
}  // namespace

int main(int argc, char* argv[]) {
    std::set_terminate(new_handler);

    // global setup...
    nrn_main_launch = 2;
    int argc_nompi = 2;
    const char* argv_nompi[] = {"NEURON", "-nogui", nullptr};
    nrn_nobanner_ = 1;

#if NRNMPI && NRNMPI_DYNAMICLOAD
    if (!nrnmpi_use) {
        nrnmpi_stubs();
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
