#ifndef partrans_h
#define partrans_h

struct Memb_list;

namespace nrn_partrans {

#ifndef NRNLONGSGID
#define NRNLONGSGID 0
#endif

#if NRNLONGSGID
    typedef int64_t sgid_t;
#else
    typedef int sgid_t;
#endif

    struct HalfGap_Info {
        int layout;
        int type;
        int ix_vpre; /* AoS index for vpre from beginning of a HalfGap instance */
        int sz;      /* size of a HalfGap instance */
    };
    extern HalfGap_Info* halfgap_info;

    class TransferThreadData {
      public:
        TransferThreadData();
        ~TransferThreadData();
        Memb_list* halfgap_ml;
        int nsrc;             // number of places in outsrc_buf_ voltages get copied to.
        int ntar;             // insrc_indices size (halfgap_ml->nodecount);
        int* insrc_indices;   // halfgap_ml->nodecount indices into insrc_buf_
        int* v_indices;       // indices into NrnThread._actual_v (may have duplications).
        int* outbuf_indices;  // indices into outsrc_buf_
        double* v_gather;     // _actual_v[v_indices]
    };
    extern TransferThreadData* transfer_thread_data_; /* array for threads */

    struct SetupInfo {
        int nsrc;  // number of sources in this thread
        int ntar;  // equal to memb_list nodecount
        int type;
        int ix_vpre;
        sgid_t* sid_src;
        int* v_indices;      // increasing order
        sgid_t* sid_target;  // aleady in memb_list order
    };
    extern SetupInfo* setup_info_; /* array for threads exists only during setup*/

    extern void gap_mpi_setup(int ngroup);
    extern void gap_thread_setup(NrnThread& nt);
    extern void gap_indices_permute(NrnThread& nt);
    extern void gap_update_indices();

    extern double* insrc_buf_;   // Receive buffer for gap voltages
    extern double* outsrc_buf_;  // Send buffer for gap voltages
    extern int *insrccnt_, *insrcdspl_, *outsrccnt_, *outsrcdspl_;
}

#endif /*partrans_h*/
