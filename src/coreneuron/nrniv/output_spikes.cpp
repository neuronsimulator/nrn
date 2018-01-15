/*
Copyright (c) 2016, Blue Brain Project
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <iostream>
#include <sstream>
#include <string.h>
#include <stdexcept>  // std::lenght_error
#include <vector>
#include "coreneuron/nrnconf.h"
#include "coreneuron/nrniv/nrniv_decl.h"
#include "coreneuron/nrniv/output_spikes.h"
#include "coreneuron/nrnmpi/nrnmpi.h"
#include "coreneuron/nrniv/nrnmutdec.h"
#include "coreneuron/nrnmpi/nrnmpi_impl.h"
#include "coreneuron/nrnmpi/nrnmpidec.h"

std::vector<double> spikevec_time;
std::vector<int> spikevec_gid;

static MUTDEC

    void
    mk_spikevec_buffer(int sz) {
    try {
        spikevec_time.reserve(sz);
        spikevec_gid.reserve(sz);
    } catch (const std::length_error& le) {
        std::cerr << "Lenght error" << le.what() << std::endl;
    }
    MUTCONSTRUCT(1);
}

void spikevec_lock() {
    MUTLOCK
}

void spikevec_unlock() {
    MUTUNLOCK
}

#if NRNMPI
/** Write generated spikes to out.dat using mpi parallel i/o.
 *  \todo : MPI related code should be factored into nrnmpi.c
 *          Check spike record length which is set to 64 chars
 */
void output_spikes_parallel(const char* outpath) {
    std::stringstream ss;
    ss << outpath << "/out.dat";
    std::string fname = ss.str();

    // remove if file already exist
    if (nrnmpi_myid == 0) {
        remove(fname.c_str());
    }
    nrnmpi_barrier();

    // each spike record in the file is time + gid (64 chars sufficient)
    const int SPIKE_RECORD_LEN = 64;
    unsigned num_spikes = spikevec_gid.size();
    unsigned num_bytes = (sizeof(char) * num_spikes * SPIKE_RECORD_LEN);
    char* spike_data = (char*)malloc(num_bytes);

    if (spike_data == NULL) {
        printf("Error while writing spikes due to memory allocation\n");
        return;
    }

    // empty if no spikes
    strcpy(spike_data, "");

    // populate buffer with all spike entries
    char spike_entry[SPIKE_RECORD_LEN];
    for (unsigned i = 0; i < num_spikes; i++) {
        snprintf(spike_entry, 64, "%.8g\t%d\n", spikevec_time[i], spikevec_gid[i]);
        strcat(spike_data, spike_entry);
    }

    // calculate offset into global file. note that we don't write
    // all num_bytes but only "populated" buffer
    unsigned long num_chars = strlen(spike_data);
    unsigned long offset = 0;

    // global offset into file
    MPI_Exscan(&num_chars, &offset, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);

    // write to file using parallel mpi i/o
    MPI_File fh;
    MPI_Status status;

    // ibm mpi (bg-q) expects char* instead of const char* (even though it's standard)
    int op_status = MPI_File_open(MPI_COMM_WORLD, (char*)fname.c_str(),
                                  MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &fh);
    if (op_status != MPI_SUCCESS && nrnmpi_myid == 0) {
        std::cerr << "Error while opening spike output file " << fname << std::endl;
        abort();
    }

    op_status = MPI_File_write_at_all(fh, offset, spike_data, num_chars, MPI_BYTE, &status);
    if (op_status != MPI_SUCCESS && nrnmpi_myid == 0) {
        std::cerr << "Error while writing spike output " << std::endl;
        abort();
    }

    MPI_File_close(&fh);
}
#endif

void output_spikes_serial(const char* outpath) {
    std::stringstream ss;
    ss << outpath << "/out.dat";
    std::string fname = ss.str();

    // remove if file already exist
    remove(fname.c_str());

    FILE* f = fopen(fname.c_str(), "w");
    if (!f && nrnmpi_myid == 0) {
        std::cout << "WARNING: Could not open file for writing spikes." << std::endl;
        return;
    }

    for (unsigned i = 0; i < spikevec_gid.size(); ++i)
        if (spikevec_gid[i] > -1)
            fprintf(f, "%.8g\t%d\n", spikevec_time[i], spikevec_gid[i]);

    fclose(f);
}

void output_spikes(const char* outpath) {
#if NRNMPI
    if (nrnmpi_initialized()) {
        output_spikes_parallel(outpath);
    } else {
        output_spikes_serial(outpath);
    }
#else
    output_spikes_serial(outpath);
#endif
}

void validation(std::vector<std::pair<double, int> >& res) {
    for (unsigned i = 0; i < spikevec_gid.size(); ++i)
        if (spikevec_gid[i] > -1)
            res.push_back(std::make_pair(spikevec_time[i], spikevec_gid[i]));
}
