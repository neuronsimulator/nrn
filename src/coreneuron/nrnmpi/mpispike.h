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

#ifndef nrnmpispike_h
#define nrnmpispike_h

#if NRNMPI

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
#define spbufin_ nrnmpi_spbufin_
extern NRNMPI_Spikebuf* spbufout_;
extern NRNMPI_Spikebuf* spbufin_;
#endif


#endif // NRNMPI

#if defined(__cplusplus)
}
#endif

#endif
