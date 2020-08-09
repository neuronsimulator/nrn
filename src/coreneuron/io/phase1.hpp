#pragma once

#include <vector>

#include "coreneuron/io/nrn_filehandler.hpp"
#include "coreneuron/utils/nrnmutdec.h"

namespace coreneuron {

struct NrnThread;

class Phase1 {
    public:
    void read_file(FileHandler& F);
    void read_direct(int thread_id);
#ifdef _OPENMP
    void populate(NrnThread& nt, int imult, OMP_Mutex& mut);
#else
    void populate(NrnThread& nt, int imult);
#endif

    private:
    void shift_gids(int imult);
    void add_extracon(NrnThread& nt, int imult);

    std::vector<int> output_gids;
    std::vector<int> netcon_srcgids;
};

}  // namespace coreneuron
