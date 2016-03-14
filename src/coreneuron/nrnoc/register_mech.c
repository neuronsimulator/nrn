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

#include <string.h>
#include "coreneuron/nrnconf.h"
#include "coreneuron/nrnoc/multicore.h"
#include "coreneuron/nrnoc/membdef.h"
#include "coreneuron/nrnoc/nrnoc_decl.h"
#include "coreneuron/nrnmpi/nrnmpi.h"

int secondorder=0;
double t, dt, celsius;

static int memb_func_size_;
static int pointtype = 1; /* starts at 1 since 0 means not point in pnt_map*/
int n_memb_func;

Memb_func* memb_func;
Memb_list* memb_list;
Point_process** point_process;
char* pnt_map;		/* so prop_free can know its a point mech*/
typedef void (*Pfrv)();
BAMech** bamech_;

pnt_receive_t* pnt_receive;	/* for synaptic events. */
pnt_receive_t* pnt_receive_init;
short* pnt_receive_size;
 /* values are type numbers of mechanisms which do net_send call */
int nrn_has_net_event_cnt_;
int* nrn_has_net_event_;
int* nrn_prop_param_size_;
int* nrn_prop_dparam_size_;
int* nrn_mech_data_layout_; /* 1 AoS (default), >1 AoSoA, 0 SoA */
int* nrn_dparam_ptr_start_;
int* nrn_dparam_ptr_end_;
short* nrn_is_artificial_;

/* dependency helper filled by calls to hoc_register_dparam_semantics */
/* used when nrn_mech_depend is called */
static int ion_write_depend_size_;
static int** ion_write_depend_;
static void ion_write_depend(int type, int etype);

bbcore_read_t* nrn_bbcore_read_;
void hoc_reg_bbcore_read(int type, bbcore_read_t f) {
  if (type == -1)
    return;

  nrn_bbcore_read_[type] = f;
}

void  add_nrn_has_net_event(int type) {
  if (type == -1)
    return;

  ++nrn_has_net_event_cnt_;
  nrn_has_net_event_ = (int*)erealloc(nrn_has_net_event_, nrn_has_net_event_cnt_*sizeof(int));
  nrn_has_net_event_[nrn_has_net_event_cnt_ - 1] = type;
}

/* values are type numbers of mechanisms which have FOR_NETCONS statement */
int nrn_fornetcon_cnt_; /* how many models have a FOR_NETCONS statement */
int* nrn_fornetcon_type_; /* what are the type numbers */
int* nrn_fornetcon_index_; /* what is the index into the ppvar array */

void add_nrn_fornetcons(int type, int indx) {
  int i;

  if (type == -1)
    return;

  i = nrn_fornetcon_cnt_++;
  nrn_fornetcon_type_ = (int*)erealloc(nrn_fornetcon_type_, (i+1)*sizeof(int));
  nrn_fornetcon_index_ = (int*)erealloc(nrn_fornetcon_index_, (i+1)*sizeof(int));
  nrn_fornetcon_type_[i] = type;
  nrn_fornetcon_index_[i]= indx;
}

/* array is parallel to memb_func. All are 0 except 1 for ARTIFICIAL_CELL */
short* nrn_is_artificial_;
short* nrn_artcell_qindex_;

void  add_nrn_artcell(int type, int qi){
  if (type == -1)
    return;

  nrn_is_artificial_[type] = 1;
  nrn_artcell_qindex_[type] = qi;
}


void alloc_mech(int n) {
	memb_func_size_ = n;
	n_memb_func = n;
	memb_func = (Memb_func*)ecalloc(memb_func_size_, sizeof(Memb_func));
	memb_list = (Memb_list*)ecalloc(memb_func_size_, sizeof(Memb_list));
	point_process = (Point_process**)ecalloc(memb_func_size_, sizeof(Point_process*));
	pnt_map = (char*)ecalloc(memb_func_size_, sizeof(char));
	pnt_receive = (pnt_receive_t*)ecalloc(memb_func_size_, sizeof(pnt_receive_t));
	pnt_receive_init = (pnt_receive_t*)ecalloc(memb_func_size_, sizeof(pnt_receive_t));
	pnt_receive_size = (short*)ecalloc(memb_func_size_, sizeof(short));
	nrn_is_artificial_ = (short*)ecalloc(memb_func_size_, sizeof(short));
	nrn_artcell_qindex_ = (short*)ecalloc(memb_func_size_, sizeof(short));
	nrn_prop_param_size_ = (int*)ecalloc(memb_func_size_, sizeof(int));
	nrn_prop_dparam_size_ = (int*)ecalloc(memb_func_size_, sizeof(int));
	nrn_mech_data_layout_ = (int*)ecalloc(memb_func_size_, sizeof(int));
	{int i; for (i=0; i < memb_func_size_; ++i) { nrn_mech_data_layout_[i] = 1; }}
	nrn_dparam_ptr_start_ = (int*)ecalloc(memb_func_size_, sizeof(int));
	nrn_dparam_ptr_end_ = (int*)ecalloc(memb_func_size_, sizeof(int));
	nrn_bbcore_read_ = (bbcore_read_t*)ecalloc(memb_func_size_, sizeof(bbcore_read_t));
	bamech_ = (BAMech**)ecalloc(BEFORE_AFTER_SIZE, sizeof(BAMech*));
}

void initnrn() {
	secondorder = DEF_secondorder;	/* >0 means crank-nicolson. 2 means currents
				   adjusted to t+dt/2 */
	t = 0;		/* msec */
	dt = DEF_dt;	/* msec */
	celsius = DEF_celsius;	/* degrees celsius */
}

/* if vectorized then thread_data_size added to it */
int register_mech(const char** m, mod_alloc_t alloc, mod_f_t cur, mod_f_t jacob,
  mod_f_t stat, mod_f_t initialize, int nrnpointerindex, int vectorized
  ) {
	int type;	/* 0 unused, 1 for cable section */
	(void)nrnpointerindex; /*unused*/

	type = nrn_get_mechtype(m[1]);

        // No mechanism in the .dat files
        if (type == -1)
           return type;

	assert(type);
#ifdef DEBUG
	printf("register_mech %s %d\n", m[1], type);
#endif
	nrn_dparam_ptr_start_[type] = 0; /* fill in later */
	nrn_dparam_ptr_end_[type] = 0; /* fill in later */
	memb_func[type].sym = (char*)emalloc(strlen(m[1])+1);
	strcpy(memb_func[type].sym, m[1]);
	memb_func[type].current = cur;
	memb_func[type].jacob = jacob;
	memb_func[type].alloc = alloc;
	memb_func[type].state = stat;
	memb_func[type].initialize = initialize;
	memb_func[type].destructor = (Pfri)0;
#if VECTORIZE
	memb_func[type].vectorized = vectorized ? 1:0;
	memb_func[type].thread_size_ = vectorized ? (vectorized - 1) : 0;
	memb_func[type].thread_mem_init_ = (void*)0;
	memb_func[type].thread_cleanup_ = (void*)0;
	memb_func[type].thread_table_check_ = (void*)0;
	memb_func[type].is_point = 0;
	memb_func[type].setdata_ = (void*)0;
	memb_func[type].dparam_semantics = (int*)0;
	memb_list[type].nodecount = 0;
	memb_list[type]._thread = (ThreadDatum*)0;
#endif
	return type;
}

void nrn_writes_conc(int type, int unused) {
  static int lastion = EXTRACELL+1;
  (void)unused; /* unused */
  if (type == -1)
    return;

#if 0
	printf("%s reordered from %d to %d\n", memb_func[type].sym->name, type, lastion);
#endif
  if (nrn_is_ion(type)) {
    ++lastion;
  }
}

void _nrn_layout_reg(int type, int layout) {
	nrn_mech_data_layout_[type] = layout;
}

void hoc_register_prop_size(int type, int psize, int dpsize) {
  int pold, dpold;
  if (type == -1)
    return;  

  pold = nrn_prop_param_size_[type];
  dpold = nrn_prop_dparam_size_[type];
  if (psize != pold || dpsize != dpold) {
    printf("%s prop sizes differ psize %d %d   dpsize %d %d\n", memb_func[type].sym, psize, pold, dpsize, dpold);
    exit(1);
  }
  nrn_prop_param_size_[type] = psize;
  nrn_prop_dparam_size_[type] = dpsize;
  if (dpsize) {
    memb_func[type].dparam_semantics = (int*)ecalloc(dpsize, sizeof(int));
  }
}
void hoc_register_dparam_semantics(int type, int ix, const char* name) {
	/* needed for SoA to possibly reorder name_ion and some "pointer" pointers. */
	/* only interested in area, iontype, cvode_ieq,
	   netsend, pointer, pntproc, bbcorepointer
	   xx_ion and #xx_ion which will get
	   a semantics value of -1, -2, -3,
	   -4, -5, -6, -7,
	   type, and type+1000 respectively
	*/
	if (strcmp(name, "area") == 0) {
		memb_func[type].dparam_semantics[ix] = -1;
	}else if (strcmp(name, "iontype") == 0) {
		memb_func[type].dparam_semantics[ix] = -2;
	}else if (strcmp(name, "cvodeieq") == 0) {
		memb_func[type].dparam_semantics[ix] = -3;
	}else if (strcmp(name, "netsend") == 0) {
		memb_func[type].dparam_semantics[ix] = -4;
	}else if (strcmp(name, "pointer") == 0) {
		memb_func[type].dparam_semantics[ix] = -5;
	}else if (strcmp(name, "pntproc") == 0) {
		memb_func[type].dparam_semantics[ix] = -6;
	}else if (strcmp(name, "bbcorepointer") == 0) {
		memb_func[type].dparam_semantics[ix] = -7;
	}else{
		int etype;
		int i = 0;
		if (name[0] == '#') { i = 1; }
		etype = nrn_get_mechtype(name+i);
		memb_func[type].dparam_semantics[ix] = etype + i*1000;
/* note that if style is needed (i==1), then we are writing a concentration */
		if (i) {
			ion_write_depend(type, etype);
		}
	}
#if 0
	printf("dparam semantics %s ix=%d %s %d\n", memb_func[type].sym,
	  ix, name, memb_func[type].dparam_semantics[ix]);
#endif
}

/* only ion type ion_write_depend_ are non-NULL */
/* and those are array of integers with first integer being array size */
/* and remaining size-1 integers containing the mechanism types that write concentrations to that ion */
static void ion_write_depend(int type, int etype) {
	int size, i;
	if (ion_write_depend_size_ < n_memb_func) {
		ion_write_depend_ = (int**)erealloc(ion_write_depend_, n_memb_func*sizeof(int*));
		for(i = ion_write_depend_size_; i < n_memb_func; ++i) {
			ion_write_depend_[i] = NULL;
		}
		ion_write_depend_size_ = n_memb_func;
	}
	size = 2;
	if (ion_write_depend_[etype]) {
		size = ion_write_depend_[etype][0] + 1;
	}
	ion_write_depend_[etype] = (int*)erealloc(ion_write_depend_[etype], size*sizeof(int));
	ion_write_depend_[etype][0] = size;
	ion_write_depend_[etype][size-1] = type;
}

static int depend_append(int idep, int* dependencies, int deptype, int type) {
	/* append only if not already in dependencies and != type*/
	int add, i;
	add = 1;
	if (deptype == type) { return idep; }
	for (i=0; i < idep; ++i) {
		if (deptype == dependencies[i]) {
			add = 0;
			break;
		}
	}
	if (add) {
		dependencies[idep++] = deptype;
	}
	return idep;
}

/* return list of types that this type depends on (10 should be more than enough) */
/* dependencies must be an array that is large enough to hold that array */
/* number of dependencies is returned */
int nrn_mech_depend(int type, int* dependencies) {
	int i, dpsize, idep, deptype;
	int* ds;
	dpsize = nrn_prop_dparam_size_[type];
	ds = memb_func[type].dparam_semantics;
	idep = 0;
	if (ds) for (i=0; i < dpsize; ++i) {
		if (ds[i] > 0 && ds[i] < 1000) {
			int idepnew;
			int* iwd;
			deptype = ds[i];
			idepnew = depend_append(idep, dependencies, deptype, type);
			iwd = ion_write_depend_ ? ion_write_depend_[deptype] : 0;
			if (idepnew > idep && iwd) {
				int size, j;
				size = iwd[0];
				for (j=1; j < size; ++j) {
					idepnew=depend_append(idepnew, dependencies, iwd[j], type);
				}
			}
			idep = idepnew;
		}
	}
	return idep;
}

void register_destructor(Pfri d) {
	memb_func[n_memb_func - 1].destructor = d;
}

int point_reg_helper(Symbol* s2) {
  int type;
  type = nrn_get_mechtype(s2);

  // No mechanism in the .dat files
  if (type == -1)
    return type;

  pnt_map[type] = pointtype;
  memb_func[type].is_point = 1;

  return pointtype++;
}

int point_register_mech(const char** m,
  mod_alloc_t alloc, mod_f_t cur, mod_f_t jacob,
  mod_f_t stat, mod_f_t initialize, int nrnpointerindex,
  void*(*constructor)(), void(*destructor)(),
  int vectorized
){
	Symbol* s;
	(void)constructor; (void)destructor; /* unused */
	s = (char*)m[1];
	register_mech(m, alloc, cur, jacob, stat, initialize, nrnpointerindex, vectorized);
	return point_reg_helper(s);
}

void _modl_cleanup(){}

int state_discon_allowed_;
int state_discon_flag_ = 0;
void state_discontinuity(int i, double* pd, double d) {
	(void)i; /* unused */
	if (state_discon_allowed_ && state_discon_flag_ == 0) {
		*pd = d;
/*printf("state_discontinuity t=%g pd=%lx d=%g\n", t, (long)pd, d);*/
	}
}

void hoc_reg_ba(int mt, mod_f_t f, int type) {
  BAMech* bam;
  if (type == -1)
    return;  

  switch (type) { /* see bablk in src/nmodl/nocpout.c */
	case 11: type = BEFORE_BREAKPOINT; break;
	case 22: type = AFTER_SOLVE; break;
	case 13: type = BEFORE_INITIAL; break;
	case 23: type = AFTER_INITIAL; break;
	case 14: type = BEFORE_STEP; break;
	default:
printf("before-after processing type %d for %s not implemented\n", type, memb_func[mt].sym);
		nrn_exit(1);
  }
  bam = (BAMech*)emalloc(sizeof(BAMech));
  bam->f = f;
  bam->type = mt;
  bam->next = bamech_[type];
  bamech_[type] = bam;
}

void _nrn_thread_reg0(int i, void(*f)(ThreadDatum*)) {
  if (i == -1)
    return;

  memb_func[i].thread_cleanup_ = f;
}

void _nrn_thread_reg1(int i, void(*f)(ThreadDatum*)) {
  if (i == -1)
    return;

  memb_func[i].thread_mem_init_ = f;
}

void _nrn_thread_table_reg(int i, void(*f)(int, int, double*, Datum*, ThreadDatum*, void*, int)) {
  if (i == -1)
    return;

  memb_func[i].thread_table_check_ = f;
}

void _nrn_setdata_reg(int i, void(*call)(double*, Datum*)) {
  if (i == -1)
    return;

  memb_func[i].setdata_ = call;
}
