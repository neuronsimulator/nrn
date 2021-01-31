/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#ifndef nrnmpispike_h
#define nrnmpispike_h

#if NRNMPI

#ifndef nrn_spikebuf_size
#define nrn_spikebuf_size 0
#endif

namespace coreneuron {

#if nrn_spikebuf_size > 0
struct NRNMPI_Spikebuf {
    int nspike;
    int gid[nrn_spikebuf_size];
    double spiketime[nrn_spikebuf_size];
};
#endif

#define icapacity_ nrnmpi_i_capacity_
#define spikeout_  nrnmpi_spikeout_
#define spikein_   nrnmpi_spikein_
#define nout_      nrnmpi_nout_
#define nin_       nrnmpi_nin_
extern int nout_;
extern int* nin_;
extern int icapacity_;
extern NRNMPI_Spike* spikeout_;
extern NRNMPI_Spike* spikein_;

#define spfixout_       nrnmpi_spikeout_fixed_
#define spfixin_        nrnmpi_spikein_fixed_
#define spfixin_ovfl_   nrnmpi_spikein_fixed_ovfl_
#define localgid_size_  nrnmpi_localgid_size_
#define ag_send_size_   nrnmpi_ag_send_size_
#define ag_send_nspike_ nrnmpi_send_nspike_
#define ovfl_capacity_  nrnmpi_ovfl_capacity_
#define ovfl_           nrnmpi_ovfl_
extern int localgid_size_;  /* bytes */
extern int ag_send_size_;   /* bytes */
extern int ag_send_nspike_; /* spikes */
extern int ovfl_capacity_;  /* spikes */
extern int ovfl_;           /* spikes */
extern unsigned char* spfixout_;
extern unsigned char* spfixin_;
extern unsigned char* spfixin_ovfl_;

#if nrn_spikebuf_size > 0
#define spbufout_ nrnmpi_spbufout_
#define spbufin_  nrnmpi_spbufin_
extern NRNMPI_Spikebuf* spbufout_;
extern NRNMPI_Spikebuf* spbufin_;
#endif

}  // namespace coreneuron

#endif  // NRNMPI
#endif
