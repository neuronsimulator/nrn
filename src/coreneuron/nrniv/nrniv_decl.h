/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#ifndef nrniv_dec_h
#define nrniv_dec_h

#include <vector>
#include <map>
#include "coreneuron/network/netcon.hpp"
namespace coreneuron {

/// Mechanism type to be used from stdindex2ptr and nrn_dblpntr2nrncore (in Neuron)
/// Values of the mechanism types should be negative numbers to avoid any conflict with
/// mechanism types of Memb_list(>0) or time(0) passed from Neuron
enum mech_type {voltage = -1, i_membrane_ = -2};

extern bool cvode_active_;
/// Vector of maps for negative presyns
extern std::vector<std::map<int, PreSyn*> > neg_gid2out;
/// Maps for ouput and input presyns
extern std::map<int, PreSyn*> gid2out;
extern std::map<int, InputPreSyn*> gid2in;

/// InputPreSyn.nc_index_ to + InputPreSyn.nc_cnt_ give the NetCon*
extern std::vector<NetCon*> netcon_in_presyn_order_;
/// Only for setup vector of netcon source gids and mindelay determination
extern std::vector<int*> nrnthreads_netcon_srcgid;
/// Companion to nrnthreads_netcon_srcgid when src gid is negative to allow
/// determination of the NrnThread of the source PreSyn.
extern std::vector<std::vector<int> > nrnthreads_netcon_negsrcgid_tid;

extern void mk_mech(const char* path);
extern void set_globals(const char* path, bool cli_global_seed, int cli_global_seed_value);
extern void mk_netcvode(void);
extern void nrn_p_construct(void);
extern void nrn_setup(const char* filesdat,
                      bool is_mapping_needed,
                      bool run_setup_cleanup = true,
                      const char* datapath = "",
                      const char* restore_path = "",
                      double* mindelay = nullptr);
extern double* stdindex2ptr(int mtype, int index, NrnThread&);
extern void delete_trajectory_requests(NrnThread&);
extern void nrn_cleanup();
extern void nrn_cleanup_ion_map();
extern void BBS_netpar_solve(double);
extern void nrn_mkPatternStim(const char* filename, double tstop);
extern int nrn_extra_thread0_vdata;
extern void nrn_set_extra_thread0_vdata(void);
extern Point_process* nrn_artcell_instantiate(const char* mechname);
extern int nrnmpi_spike_compress(int nspike, bool gidcompress, int xchng);
extern bool nrn_use_bin_queue_;

extern void nrn_outputevent(unsigned char, double);
extern void ncs2nrn_integrate(double tstop);

extern void handle_forward_skip(double forwardskip, int prcellgid);

extern int nrn_set_timeout(int);
extern void nrn_fake_fire(int gid, double spiketime, int fake_out);

extern void netpar_tid_gid2ps(int tid, int gid, PreSyn** ps, InputPreSyn** psi);
extern double set_mindelay(double maxdelay);

extern int nrn_soa_padded_size(int cnt, int layout);

extern int interleave_permute_type;
extern int cellorder_nwarp;
}  // namespace coreneuron
#endif
