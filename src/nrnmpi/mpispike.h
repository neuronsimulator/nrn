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

#if nrn_spikebuf_size > 0
#define spbufout_ nrnmpi_spbufout_
#define spbufin_  nrnmpi_spbufin_
extern NRNMPI_Spikebuf* spbufout_;
extern NRNMPI_Spikebuf* spbufin_;
#endif


#endif
