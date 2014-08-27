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

#include "corebluron/nrniv/netcon.h"
#include "corebluron/utils/endianness.h"


#if defined(__cplusplus)
extern "C" {
#endif

extern int nrn_mallinfo(void);
extern void mk_mech(const char* fname);
extern void mk_netcvode(void);
extern void nrn_p_construct(void);
extern void nrn_setup(int ngroup, int* gidgroups, const char *path, enum endian::endianness file_endian);
extern double BBS_netpar_mindelay(double maxdelay);
extern void BBS_netpar_solve(double);
extern void nrn_mkPatternStim(const char* filename);
extern int nrn_extra_thread0_vdata;
extern void nrn_set_extra_thread0_vdata(void);
extern Point_process* nrn_artcell_instantiate(const char* mechname);

extern void nrn_alloc_gid2out(int size, int poolsize);
extern void nrn_alloc_gid2in(int size, int poolsize);
extern int input_gid_register(int gid);
extern int input_gid_associate(int gid, InputPreSyn* psi);
extern void BBS_gid2ps(int gid, PreSyn** ps, InputPreSyn** psi);

// only used in nrn_setup.cpp but implemented in netpar.cpp since that
// is where Gid2PreSyn hash tables are defined.
extern void netpar_tid_gid2ps_alloc(int nth);
extern void netpar_tid_gid2ps_alloc_item(int ith, int size, int poolsize);
extern void netpar_tid_gid2ps_free();
extern void netpar_tid_gid2ps(int tid, int gid, PreSyn** ps, InputPreSyn** psi);
extern void netpar_tid_set_gid2node(int tid, int gid, int nid);
extern void netpar_tid_cell(int tid, int gid, PreSyn* ps);


extern NetCon* BBS_gid_connect(int gid, Point_process* target, NetCon&);
extern void BBS_cell(int gid, PreSyn* ps);
extern void BBS_set_gid2node(int gid, int rank);
extern void nrn_cleanup_presyn(DiscreteEvent*);
extern void nrn_outputevent(unsigned char, double);
extern void ncs2nrn_integrate(double tstop);
extern void nrn_pending_selfqueue(double, NrnThread*);
extern size_t output_presyn_size(void);
extern size_t input_presyn_size(void);

extern NetCon** netcon_in_presyn_order_;

#if defined(__cplusplus)
}
#endif

#endif
