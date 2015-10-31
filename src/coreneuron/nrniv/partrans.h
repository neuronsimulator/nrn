#ifndef partrans_h
#define partrans_h

struct Memb_list;

namespace nrn_partrans {

  struct HalfGap_Info {
    int layout;
    int ix_vpre; /* AoS index for vpre from beginning of a HalfGap instance */
    int sz; /* size of a HalfGap instance */
  };
  extern HalfGap_Info* halfgap_info;

  struct TransferThreadData {
    Memb_list* halfgap_ml;
    int* insrc_indices; // halfgap_ml->nodecount indices into insrc_buf_
    int nsrc; // number of places in outsrc_buf_ voltages get copied to.
    int* v_indices; // indices into NrnThread._actual_v (may have duplications).
  };
  extern TransferThreadData* transfer_thread_data_; /* array size nrn_nthread */

  
  extern double* insrc_buf_; // Receive buffer for gap voltages
  extern double* outsrc_buf_; // Send buffer for gap voltages
  extern int *insrccnt_, *insrcdspl_, *outsrccnt_, *outsrcdspl_;

}

#endif /*partrans_h*/
