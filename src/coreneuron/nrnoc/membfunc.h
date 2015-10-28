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

#ifndef nrn_memb_func_h
#define nrn_memb_func_h

#if defined(__cplusplus)
extern "C" {
#endif

#include "coreneuron/nrnoc/nrnoc_ml.h"

typedef Datum *(*Pfrpdat)(void);

struct NrnThread;

typedef void (*mod_alloc_t)(double*, Datum*, int);
typedef void (*mod_f_t)(struct NrnThread*, Memb_list*, int);
typedef void (*pnt_receive_t)(Point_process*, int, double);

typedef struct Memb_func {
	mod_alloc_t alloc;
	mod_f_t	current;
	mod_f_t	jacob;
	mod_f_t	state;
	mod_f_t	initialize;
	Pfri	destructor;	/* only for point processes */
	Symbol	*sym;
	int vectorized;
	int thread_size_; /* how many Datum needed in Memb_list if vectorized */
	void (*thread_mem_init_)(ThreadDatum*); /* after Memb_list._thread is allocated */
	void (*thread_cleanup_)(ThreadDatum*); /* before Memb_list._thread is freed */
    void (*thread_table_check_)(int, int, double*, Datum*, ThreadDatum*, void*, int);
	int is_point;
	void (*setdata_)(double*, Datum*);
	int* dparam_semantics; /* for nrncore writing. */
} Memb_func;


#define VINDEX	-1
#define CABLESECTION	1
#define MORPHOLOGY	2
#define CAP	3
#define EXTRACELL	5

#define nrnocCONST 1
#define DEP 2
#define STATE 3	/*See init.c and cabvars.h for order of nrnocCONST, DEP, and STATE */

#define BEFORE_INITIAL 0
#define AFTER_INITIAL 1
#define BEFORE_BREAKPOINT 2
#define AFTER_SOLVE 3
#define BEFORE_STEP 4
#define BEFORE_AFTER_SIZE 5 /* 1 more than the previous */
typedef struct BAMech {
	mod_f_t f;
	int type;
	struct BAMech* next;
} BAMech;
extern BAMech** bamech_;

extern int nrn_ion_global_map_size;
extern double **nrn_ion_global_map;

extern Memb_func* memb_func;
extern int n_memb_func;
#define NRNPOINTER 4 /* added on to list of mechanism variables.These are
pointers which connect variables  from other mechanisms via the _ppval array.
*/

#define _AMBIGUOUS 5

extern int* nrn_prop_param_size_;
extern int* nrn_prop_dparam_size_;
extern char* pnt_map;
extern short* nrn_is_artificial_;
extern short* pnt_receive_size;
extern pnt_receive_t* pnt_receive;
extern pnt_receive_t* pnt_receive_init;

extern int nrn_get_mechtype(const char*);
extern int register_mech(const char** m, mod_alloc_t alloc, mod_f_t cur, mod_f_t jacob,
  mod_f_t stat, mod_f_t initialize, int nrnpointerindex, int vectorized
  ); 
extern int point_register_mech(const char**, mod_alloc_t alloc, mod_f_t cur,
  mod_f_t jacob, mod_f_t stat, mod_f_t initialize, int nrnpointerindex,
  void*(*constructor)(), void(*destructor)(), int vectorized
  );
typedef void(*NetBufReceive_t)(struct NrnThread*);
extern void hoc_register_net_receive_buffering(NetBufReceive_t, int);
extern int net_buf_receive_cnt_;
extern int* net_buf_receive_type_;
extern NetBufReceive_t* net_buf_receive_;
extern void nrn_cap_jacob(struct NrnThread*, Memb_list*);
extern void nrn_writes_conc(int, int);
#pragma acc routine seq
extern void nrn_wrote_conc(int, double*, int, int, double**, double, int);
extern void hoc_register_prop_size(int, int, int);
extern void hoc_register_dparam_semantics(int type, int, const char* name);

extern void _nrn_layout_reg(int, int);
extern int* nrn_mech_data_layout_;
extern void _nrn_thread_reg0(int i, void(*f)(ThreadDatum*));
extern void _nrn_thread_reg1(int i, void(*f)(ThreadDatum*));

typedef void (*bbcore_read_t)(double*, int*, int*, int*, int, int, double*, Datum*, ThreadDatum*, struct NrnThread*, double);
extern bbcore_read_t* nrn_bbcore_read_;

extern int nrn_mech_depend(int type, int* dependencies);
extern int nrn_fornetcon_cnt_;
extern int* nrn_fornetcon_type_;
extern int* nrn_fornetcon_index_;
extern void add_nrn_fornetcons(int, int);
extern void add_nrn_artcell(int, int);
extern void add_nrn_has_net_event(int);
extern void net_event(Point_process*, double);
extern void net_send(void**, int, Point_process*, double, double);
extern void artcell_net_send(void**, int, Point_process*, double, double);
extern void hoc_malchk(void); /* just a stub */
extern void* hoc_Emalloc(size_t);

#if defined(__cplusplus)
}
#endif

#endif /* nrn_memb_func_h */
