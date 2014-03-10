#ifndef nrn_memb_func_h
#define nrn_memb_func_h

#if defined(__cplusplus)
extern "C" {
#endif

#include "corebluron/nrnoc/nrnoc_ml.h"

typedef Datum *(*Pfrpdat)(void);

struct NrnThread;

typedef void (*mod_alloc_t)(double*, Datum*, int);
typedef void (*mod_f_t)(struct NrnThread*, Memb_list*, int);

#define NULL_ALLOC (mod_alloc_t)0
#define NULL_CUR (mod_f_t)0
#define NULL_STATE (mod_f_t)0
#define NULL_INITIALIZE (mod_f_t)0

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
    void (*thread_table_check_)(double*, Datum*, ThreadDatum*, void*, int);
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

extern Memb_func* memb_func;
extern int n_memb_func;
#if VECTORIZE
extern Memb_list* memb_list;
/* for finitialize, order is same up through extracellular, then ions,
then mechanisms that write concentrations, then all others. */
extern short* memb_order_; 
#endif
#define NRNPOINTER 4 /* added on to list of mechanism variables.These are
pointers which connect variables  from other mechanisms via the _ppval array.
*/

#define _AMBIGUOUS 5

extern int* nrn_prop_param_size_;
extern int* nrn_prop_dparam_size_;
extern char* pnt_map;
extern short* nrn_is_artificial_;
extern short* pnt_receive_size;

extern int nrn_get_mechtype(const char*);
extern int register_mech(const char** m, mod_alloc_t alloc, mod_f_t cur, mod_f_t jacob,
  mod_f_t stat, mod_f_t initialize, int nrnpointerindex, int vectorized
  ); 
extern int point_register_mech(const char**, mod_alloc_t alloc, mod_f_t cur,
  mod_f_t jacob, mod_f_t stat, mod_f_t initialize, int nrnpointerindex,
  void*(*constructor)(), void(*destructor)(), int vectorized
  );
extern void nrn_cap_jacob(struct NrnThread*, Memb_list*);
extern void nrn_writes_conc(int, int);
extern void hoc_register_prop_size(int, int, int);
extern void _nrn_thread_reg0(int i, void(*f)(ThreadDatum*));
extern void _nrn_thread_reg1(int i, void(*f)(ThreadDatum*));

typedef int (*bbcore_read_t)(void*, int, double*, Datum*, ThreadDatum*, struct NrnThread*);
extern bbcore_read_t* nrn_bbcore_read_;

extern int nrn_fornetcon_cnt_;
extern int* nrn_fornetcon_type_;
extern int* nrn_fornetcon_index_;
extern void add_nrn_fornetcons(int, int);

#if defined(__cplusplus)
}
#endif

#endif /* nrn_memb_func_h */
