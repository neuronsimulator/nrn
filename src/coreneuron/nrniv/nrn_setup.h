#ifndef _H_NRNSETUP_
#define _H_NRNSETUP_

#include <string>
#include "coreneuron/nrnoc/multicore.h"

static int ngroup_w;
static int* gidgroups_w;
static const char* path_w;
static data_reader* file_reader_w;
static bool byte_swap_w;

static void read_phase1(data_reader &F,NrnThread& nt);
static void read_phase2(data_reader &F, NrnThread& nt);
static void setup_ThreadData(NrnThread& nt);

namespace coreneuron {

    /// Reading phase number.
    enum phase {one=1, two};

    /// Get the phase number in form of the string.
    template<phase P>
    inline std::string getPhaseName();

    template<>
    inline std::string getPhaseName<one>(){
        return "1";
    }

    template<>
    inline std::string getPhaseName<two>(){
        return "2";
    }


    /// Reading phase selector.
    template<phase P>
    inline void read_phase_aux(data_reader &F, NrnThread& nt);

    template<>
    inline void read_phase_aux<one>(data_reader &F, NrnThread& nt){
        read_phase1(F, nt);
    }

    template<>
    inline void read_phase_aux<two>(data_reader &F, NrnThread& nt){
        read_phase2(F, nt);
    }


    /// Reading phase wrapper for each neuron group.
    template<phase P>
    inline void* phase_wrapper_w(NrnThread* nt){
        int i = nt->id;
        char fnamebuf[1000];
        if (i < ngroup_w) {
          sd_ptr fname = sdprintf(fnamebuf, sizeof(fnamebuf), std::string("%s/%d_"+getPhaseName<P>()+".dat").c_str(), path_w, gidgroups_w[i]);
          file_reader_w[i].open(fname, byte_swap_w);
          read_phase_aux<P>(file_reader_w[i], *nt);
          file_reader_w[i].close();
          if (P == 2) {
            setup_ThreadData(*nt);
          }
        }
        return NULL;
    }


    /// Specific phase reading executed by threads.
    template<phase P>
    inline static void phase_wrapper(){
        nrn_multithread_job(phase_wrapper_w<P>);
    }


}

#endif
