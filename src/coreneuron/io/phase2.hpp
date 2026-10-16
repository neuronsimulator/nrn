/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#pragma once

#include "coreneuron/io/nrn_filehandler.hpp"
#include "coreneuron/io/user_params.hpp"
#include "coreneuron/utils/ivocvect.hpp"

#include <memory>
#include <cstdint>

namespace coreneuron {
struct NrnThread;
struct NrnThreadMembList;
struct Memb_func;
struct Memb_list;
struct NrnThreadChkpnt;

class Phase2 {
  public:
    void read_file(FileHandler& F, const NrnThread& nt);
    void read_direct(int thread_id, const NrnThread& nt);
    void populate(NrnThread& nt, const UserParams& userParams);

    std::vector<int> preSynConditionEventFlags;

    // All of this is public for nrn_checkpoint
    struct EventTypeBase {
        double time;
    };
    struct NetConType_: public EventTypeBase {
        int netcon_index;
    };
    struct SelfEventType_: public EventTypeBase {
        int target_type;
        int point_proc_instance;
        int target_instance;
        double flag;
        int movable;
        int weight_index;
    };
    struct PreSynType_: public EventTypeBase {
        int presyn_index;
    };
    struct NetParEvent_: public EventTypeBase {};
    struct PlayRecordEventType_: public EventTypeBase {
        int play_record_type;
        int vecplay_index;
    };

    struct VecPlayContinuous_ {
        int vtype;
        int mtype;
        int ix;
        IvocVect yvec;
        IvocVect tvec;

        int last_index;
        int discon_index;
        int ubound_index;
    };
    std::vector<VecPlayContinuous_> vec_play_continuous;
    int patstim_index;

    std::vector<std::pair<int, std::shared_ptr<EventTypeBase>>> events;

  private:
    void check_mechanism();
    void transform_int_data(int elem0,
                            int nodecount,
                            int* pdata,
                            int i,
                            int dparam_size,
                            int layout,
                            int n_node_);
    void set_net_send_buffer(Memb_list** ml_list, const std::vector<int>& pnt_offset);
    void restore_events(FileHandler& F);
    void fill_before_after_lists(NrnThread& nt, const std::vector<Memb_func>& memb_func);
    void pdata_relocation(const NrnThread& nt, const std::vector<Memb_func>& memb_func);
    void set_dependencies(const NrnThread& nt, const std::vector<Memb_func>& memb_func);
    void handle_weights(NrnThread& nt, int n_netcon, NrnThreadChkpnt& ntc);
    void get_info_from_bbcore(NrnThread& nt,
                              const std::vector<Memb_func>& memb_func,
                              NrnThreadChkpnt& ntc);
    void set_vec_play(NrnThread& nt, NrnThreadChkpnt& ntc);

    int n_real_cell;
    int n_output;
    int n_real_output;
    int n_node;
    int n_diam;  // 0 if not needed, else n_node
    int n_mech;
    std::vector<int> mech_types;
    std::vector<int> nodecounts;
    int n_idata;
    int n_vdata;
    int* v_parent_index;
    /* TO DO: when this is fixed use it like that
    std::vector<double> actual_a;
    std::vector<double> actual_b;
    std::vector<double> actual_area;
    std::vector<double> actual_v;
    std::vector<double> actual_diam;
    */
    double* _data;
    struct TML {
        std::vector<int> nodeindices;
        std::vector<int> pdata;
        int type;
        std::vector<int> iArray;
        std::vector<double> dArray;
        std::vector<int> pointer2type;
        std::vector<uint32_t> nmodlrandom{};
    };
    std::vector<TML> tmls;
    std::vector<int> output_vindex;
    std::vector<double> output_threshold;
    std::vector<int> pnttype;
    std::vector<int> pntindex;
    std::vector<double> weights;
    std::vector<double> delay;
    int num_point_process;
};
}  // namespace coreneuron
