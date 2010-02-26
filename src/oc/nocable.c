#include <../../nrnconf.h>

#include <nrnrtuse.h>
#if NRN_REALTIME
double t, dt;
void nrn_fixed_step(){}
void nrn_fake_step() {}
#endif

int stoprun;
clear_sectionlist(){}
add_section(){}
install_sectionlist(){}
connect_obsec_syntax(){}
connectsection(){}
connectpointer(){}
sec_access(){}
range_const(){}
range_interpolate(){}
rangevareval(){}
rangevarevalpointer(){}
mech_access(){}
mech_uninsert(){}
sec_access_push(){}
sec_access_pop(){}
sec_access_temp(){}
rangepoint(){}
forall_section(){}
connect_point_process_pointer(){} nrn_cppp(){}
hoc_ifsec(){}
ob_sec_access() {double hoc_xpop(); hoc_xpop();}
sec_access_object(){}
sectionlist_decl(){}
forall_sectionlist(){}
hoc_ifseclist(){}
hoc_nrn_load_dll(){}
hoc_push_sectionlist(){}
simpleconnectsection(){}
range_interpolate_single(){}
int nrn_is_cable(){ return 0; }
int nrn_is_artificial(type)int type; {return 0;}
static char dummystring[1];
static char* pdummystring;
int hoc_secname() {
	hoc_ret();
	pdummystring = dummystring;
	hoc_pushstr(&pdummystring);
}
void hoc_construct_point(ob) void* ob; {}
void nrn_shape_update() {}
char *nrn_version(i) int i; {static char* s = "not NEURON"; return s;}
char *nrn_minor_version;
char *nrn_branch_name;
char *nrn_tree_version;
char *nrn_version_changeset;
char *nrn_version_date;
double* nrn_recalc_ptr(double* pd) { return pd; }
void nrn_hoc_lock() {}
void nrn_hoc_unlock() {}

#include "nrnmpiuse.h"
#if NRN_MUSIC
void nrnmusic_init(int* parg, char*** pargv){}
void nrnmusic_terminate(){}
#endif
