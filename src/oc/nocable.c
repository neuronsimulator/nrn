#include <../../nrnconf.h>
#include <hocdec.h>

#include <nrnrtuse.h>
#if NRN_REALTIME
double t, dt;
void nrn_fixed_step(void){}
void nrn_fake_step(void) {}
#endif

int stoprun;
void clear_sectionlist(void) {}
void add_section(void) {}
void install_sectionlist(void) {}
void connect_obsec_syntax(void) {}
void connectsection(void) {}
void connectpointer(void) {}
void sec_access(void) {}
void range_const(void) {}
void range_interpolate(void) {}
void rangevareval(void) {}
void rangevarevalpointer(void) {}
void mech_access(void) {}
void mech_uninsert(void) {}
void sec_access_push(void) {}
void hoc_sec_internal_push(void) {}
void* hoc_sec_internal_name2ptr(const char* s, int flag) { return NULL; }
void sec_access_pop(void) {}
void sec_access_temp(void) {}
void rangepoint(void) {}
void forall_section(void) {}
void connect_point_process_pointer(){}
void nrn_cppp(void) {}
void hoc_ifsec(void) {}
void ob_sec_access(void) {double hoc_xpop(); hoc_xpop();}
void sec_access_object(void) {}
void sectionlist_decl(void) {}
void forall_sectionlist(void) {}
void hoc_ifseclist(void) {}
void hoc_nrn_load_dll(void) {}
void hoc_push_sectionlist(void) {}
void simpleconnectsection(void) {}
void range_interpolate_single(void) {}
int nrn_is_cable(void) { return 0; }
int nrn_is_artificial(int type) {return 0;}
static char dummystring[1];
static char* pdummystring;
void hoc_secname(void) {
	hoc_ret();
	pdummystring = dummystring;
	hoc_pushstr(&pdummystring);
}
void hoc_construct_point(Object *ob, int i) {}
void nrn_shape_update(void) {}
char *nrn_version(int i) {static char* s = "not NEURON"; return s;}
char *nrn_minor_version;
char *nrn_branch_name;
char *nrn_tree_version;
char *nrn_version_changeset;
char *nrn_version_date;
double* nrn_recalc_ptr(double* pd) { return pd; }
void nrn_hoc_lock(void) {}
void nrn_hoc_unlock(void) {}

#include "nrnmpiuse.h"
#if NRN_MUSIC
void nrnmusic_init(int* parg, char*** pargv){}
void nrnmusic_terminate(){}
#endif
