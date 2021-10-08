#include <sys/time.h>
#include "utils.hpp"
#include "coreneuron/mpi/nrnmpi.h"
#include "coreneuron/mpi/core/nrnmpi.hpp"
#include "coreneuron/apps/corenrn_parameters.hpp"

namespace coreneuron {
void nrn_abort(int errcode) {
#if NRNMPI
    if (corenrn_param.mpi_enable && nrnmpi_initialized()) {
        nrnmpi_abort(errcode);
    } else
#endif
    {
        abort();
    }
}

void nrn_fatal_error(const char* msg) {
    if (nrnmpi_myid == 0) {
        printf("%s\n", msg);
    }
    nrn_abort(-1);
}

double nrn_wtime() {
#if NRNMPI
    if (corenrn_param.mpi_enable) {
        return nrnmpi_wtime();
    } else
#endif
    {
        struct timeval time1;
        gettimeofday(&time1, nullptr);
        return (time1.tv_sec + time1.tv_usec / 1.e6);
    }
}
}  // namespace coreneuron
