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

#ifndef nrniv_dec_h
#define nrniv_dec_h

#include <vector>
#include <map>
#include "coreneuron/nrniv/netcon.h"
#include "coreneuron/utils/endianness.h"
#include "coreneuron/nrniv/nrnoptarg.h"
namespace coreneuron {
extern int cvode_active_;
/// Vector of maps for negative presyns
extern std::vector<std::map<int, PreSyn*> > neg_gid2out;
/// Maps for ouput and input presyns
extern std::map<int, PreSyn*> gid2out;
extern std::map<int, InputPreSyn*> gid2in;

/// InputPreSyn.nc_index_ to + InputPreSyn.nc_cnt_ give the NetCon*
extern std::vector<NetCon*> netcon_in_presyn_order_;
/// Only for setup vector of netcon source gids
extern std::vector<int*> netcon_srcgid;

extern void mk_mech(const char* path);
extern void set_globals(const char* path, bool cli_global_seed, int cli_global_seed_value);
extern void mk_netcvode(void);
extern void nrn_p_construct(void);
extern void nrn_setup(const char* filesdat,
                      bool is_mapping_needed,
                      int byte_swap,
                      bool run_setup_cleanup = true);
extern int nrn_setup_multiple;
extern int nrn_setup_extracon;
extern void nrn_cleanup(bool clean_ion_global_map = true);
extern void BBS_netpar_solve(double);
extern void nrn_mkPatternStim(const char* filename);
extern int nrn_extra_thread0_vdata;
extern void nrn_set_extra_thread0_vdata(void);
extern Point_process* nrn_artcell_instantiate(const char* mechname);
extern int nrnmpi_spike_compress(int nspike, bool gidcompress, int xchng);
extern bool nrn_use_bin_queue_;
extern int nrn_need_byteswap;

extern void nrn_outputevent(unsigned char, double);
extern void ncs2nrn_integrate(double tstop);

extern void handle_forward_skip(double forwardskip, int prcellgid);

extern int nrn_set_timeout(int);
extern void nrn_fake_fire(int gid, double spiketime, int fake_out);

extern void netpar_tid_gid2ps(int tid, int gid, PreSyn** ps, InputPreSyn** psi);
extern double set_mindelay(double maxdelay);

extern int nrn_soa_padded_size(int cnt, int layout);

extern int use_interleave_permute;
extern int cellorder_nwarp;
}  // namespace coreneuron
#endif
