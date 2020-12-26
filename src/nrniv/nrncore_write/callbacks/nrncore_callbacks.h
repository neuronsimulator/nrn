#ifndef NRN_NRNCORE_CALLBACKS_H
#define NRN_NRNCORE_CALLBACKS_H

#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib>
// includers need several pieces of info for nrn_get_partrans_setup_info
#include "partrans.h"

typedef void*(*CNB)(...);
typedef struct core2nrn_callback_t {
    const char* name;
    CNB f;
} core2nrn_callback_t;

// Mechanism type to be used from stdindex2ptr (in CoreNeuron) and nrn_dblpntr2nrncore.
// Values of the mechanism types should be negative numbers to avoid any conflict with
// mechanism types of Memb_list(>0) or time(0) passed to CoreNeuron
enum mech_type {voltage = -1, i_membrane_ = -2};

class Memb_list;
class NrnThread;
class CellGroup;
class DatumIndices;

extern "C" {

void write_memb_mech_types_direct(std::ostream& s);
void part2_clean();

int get_global_int_item(const char* name);
void* get_global_dbl_item(void* p, const char* & name, int& size, double*& val);


void nrnthread_group_ids(int* groupids);
int nrnthread_dat1(int tid, int& n_presyn, int& n_netcon,
                   int*& output_gid, int*& netcon_srcgid,
                   std::vector<int>& netcon_negsrcgid_tid);
int nrnthread_dat2_1(int tid, int& ngid, int& n_real_gid, int& nnode, int& ndiam,
                     int& nmech, int*& tml_index, int*& ml_nodecount, int& nidata,
                     int& nvdata, int& nweight);
int nrnthread_dat2_2(int tid, int*& v_parent_index, double*& a, double*& b,
                     double*& area, double*& v, double*& diamvec);
int nrnthread_dat2_mech(int tid, size_t i, int dsz_inst, int*& nodeindices,
                        double*& data, int*& pdata);
int nrnthread_dat2_3(int tid, int nweight, int*& output_vindex, double*& output_threshold,
                     int*& netcon_pnttype, int*& netcon_pntindex, double*& weights, double*& delays);
int nrnthread_dat2_corepointer(int tid, int& n);
int nrnthread_dat2_corepointer_mech(int tid, int type,
                                    int& icnt, int& dcnt, int*& iarray, double*& darray);
int nrnthread_dat2_vecplay(int tid, int& n);
int nrnthread_dat2_vecplay_inst(int tid, int i, int& vptype, int& mtype,
                                int& ix, int& sz, double*& yvec, double*& tvec);

int* datum2int(int type, Memb_list* ml, NrnThread& nt, CellGroup& cg, DatumIndices& di, int ml_vdata_offset);
}

extern "C" {
void nrnthread_get_trajectory_requests(int tid, int& bsize, int& ntrajec, void**& vpr, int*& types, int*& indices, double**& pvars, double**& varrays);
void nrnthread_trajectory_values(int tid, int n_pr, void** vpr, double t);
void nrnthread_trajectory_return(int tid, int n_pr, int vecsz, void** vpr, double t);
}

extern "C" {
int nrnthread_all_spike_vectors_return(std::vector<double>& spiketvec, std::vector<int>& spikegidvec);
void nrnthreads_all_weights_return(std::vector<double*>& weights);
size_t nrnthreads_type_return(int type, int tid, double*& data, double**& mdata);
}

static core2nrn_callback_t cnbs[]  = {
        {"nrn2core_group_ids_", (CNB)nrnthread_group_ids},
        {"nrn2core_mkmech_info_", (CNB)write_memb_mech_types_direct},
        {"nrn2core_get_global_dbl_item_", (CNB)get_global_dbl_item},
        {"nrn2core_get_global_int_item_", (CNB)get_global_int_item},
        {"nrn2core_get_partrans_setup_info_", (CNB)nrn_get_partrans_setup_info},
        {"nrn2core_get_dat1_", (CNB)nrnthread_dat1},
        {"nrn2core_get_dat2_1_", (CNB)nrnthread_dat2_1},
        {"nrn2core_get_dat2_2_", (CNB)nrnthread_dat2_2},
        {"nrn2core_get_dat2_mech_", (CNB)nrnthread_dat2_mech},
        {"nrn2core_get_dat2_3_", (CNB)nrnthread_dat2_3},
        {"nrn2core_get_dat2_corepointer_", (CNB)nrnthread_dat2_corepointer},
        {"nrn2core_get_dat2_corepointer_mech_", (CNB)nrnthread_dat2_corepointer_mech},
        {"nrn2core_get_dat2_vecplay_", (CNB)nrnthread_dat2_vecplay},
        {"nrn2core_get_dat2_vecplay_inst_", (CNB)nrnthread_dat2_vecplay_inst},
        {"nrn2core_part2_clean_", (CNB)part2_clean},

        {"nrn2core_get_trajectory_requests_", (CNB)nrnthread_get_trajectory_requests},
        {"nrn2core_trajectory_values_", (CNB)nrnthread_trajectory_values},
        {"nrn2core_trajectory_return_", (CNB)nrnthread_trajectory_return},

        {"nrn2core_all_spike_vectors_return_", (CNB)nrnthread_all_spike_vectors_return},
        {"nrn2core_all_weights_return_", (CNB)nrnthreads_all_weights_return},
        {"nrn2core_type_return_", (CNB)nrnthreads_type_return},
        {NULL, NULL}
};


void map_coreneuron_callbacks(void* handle);

#endif //NRN_NRNCORE_CALLBACKS_H
