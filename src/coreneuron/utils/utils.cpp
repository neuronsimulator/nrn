#include <sys/time.h>
#include "utils.hpp"
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
