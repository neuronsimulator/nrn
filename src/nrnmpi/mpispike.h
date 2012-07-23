#ifndef nrnmpispike_h
#define nrnmpispike_h

#ifndef nrn_spikebuf_size
#define nrn_spikebuf_size 0
#endif

#if nrn_spikebuf_size > 0
typedef struct {
	int nspike;
	int gid[nrn_spikebuf_size];
	double spiketime[nrn_spikebuf_size];
} NRNMPI_Spikebuf;
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#define icapacity_ nrnmpi_i_capacity_
#define spikeout_ nrnmpi_spikeout_
#define spikein_ nrnmpi_spikein_
#define nout_ nrnmpi_nout_
#define nin_ nrnmpi_nin_
extern int nout_;
extern int* nin_;
extern int icapacity_;
extern NRNMPI_Spike* spikeout_;
extern NRNMPI_Spike* spikein_;

#define spfixout_ nrnmpi_spikeout_fixed_
#define spfixin_ nrnmpi_spikein_fixed_
#define spfixin_ovfl_ nrnmpi_spikein_fixed_ovfl_
#define localgid_size_ nrnmpi_localgid_size_
#define ag_send_size_ nrnmpi_ag_send_size_
#define ag_send_nspike_ nrnmpi_send_nspike_
#define ovfl_capacity_ nrnmpi_ovfl_capacity_
#define ovfl_ nrnmpi_ovfl_
extern int localgid_size_; /* bytes */
extern int ag_send_size_; /* bytes */
extern int ag_send_nspike_; /* spikes */
extern int ovfl_capacity_; /* spikes */
extern int ovfl_; /* spikes */
extern unsigned char* spfixout_;
extern unsigned char* spfixin_;
extern unsigned char* spfixin_ovfl_;

#if nrn_spikebuf_size > 0
#define spbufout_ nrnmpi_spbufout_
#define spbufin_ nrnmpi_spbufin_
extern NRNMPI_Spikebuf* spbufout_;
extern NRNMPI_Spikebuf* spbufin_;
#endif

#if defined(__cplusplus)
}
#endif

#endif
