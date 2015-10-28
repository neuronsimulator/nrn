/*
Copyright (c) 2014 EPFL-BBP, All rights reserved.

THIS SOFTWARE IS PROVIDED BY THE BLUE BRAIN PROJECT "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE BLUE BRAIN PROJECT
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef nrniv_dec_h
#define nrniv_dec_h

#include "coreneuron/nrniv/netcon.h"
#include "coreneuron/utils/endianness.h"


#if defined(__cplusplus)
extern "C" {
#endif

extern void mk_mech(const char* fname);
extern void mk_netcvode(void);
extern void nrn_p_construct(void);
extern void nrn_setup(const char *path, const char *filesdat, int byte_swap, int threading);
extern void nrn_cleanup();
extern double BBS_netpar_mindelay(double maxdelay);
extern void BBS_netpar_solve(double);
extern void nrn_mkPatternStim(const char* filename);
extern int nrn_extra_thread0_vdata;
extern void nrn_set_extra_thread0_vdata(void);
extern Point_process* nrn_artcell_instantiate(const char* mechname);
extern int nrn_need_byteswap;

extern void nrn_reset_gid2out(void);
extern void nrn_reset_gid2in(void);
extern int input_gid_register(int gid);
extern int input_gid_associate(int gid, InputPreSyn* psi);

// only used in nrn_setup.cpp but implemented in netpar.cpp since that
// is where int<->PreSyn* and int<->InputPreSyn* maps are defined.
extern void netpar_tid_gid2ps_alloc(int nth);
extern void netpar_tid_gid2ps_free();
extern void netpar_tid_gid2ps(int tid, int gid, PreSyn** ps, InputPreSyn** psi);
extern void netpar_tid_set_gid2node(int tid, int gid, int nid, PreSyn* ps);

extern void nrn_cleanup_presyn(DiscreteEvent*);
extern void nrn_outputevent(unsigned char, double);
extern void ncs2nrn_integrate(double tstop);
extern size_t output_presyn_size(void);
extern size_t input_presyn_size(void);

extern void handle_forward_skip(double forwardskip, int prcellgid);

extern NetCon** netcon_in_presyn_order_;

extern int nrn_set_timeout(int);
extern void nrnmpi_gid_clear(void);

extern int nrn_soa_padded_size(int cnt, int layout);

#if defined(__cplusplus)
}
#endif

#endif
