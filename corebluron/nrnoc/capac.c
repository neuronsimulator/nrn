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

#include "corebluron/nrnconf.h"
#include "corebluron/nrnoc/multicore.h"
#include "corebluron/nrnoc/membdef.h"

static const char *mechanism[] = { "0", "capacitance", "cm",0, "i_cap", 0,0 };
static void cap_alloc(double*, Datum*, int type);
static void cap_init(NrnThread*, Memb_list*, int);

#define nparm 2

void capac_reg_(void) {
	int mechtype;
	/* all methods deal with capacitance in special ways */
	register_mech(mechanism, cap_alloc, (mod_f_t)0, (mod_f_t)0, (mod_f_t)0, (mod_f_t)cap_init, -1, 1);
	mechtype = nrn_get_mechtype(mechanism[1]);
	hoc_register_prop_size(mechtype, nparm, 0);
}

#define cm  vdata[i*nparm]
#define i_cap  vdata[i*nparm+1]

/*
cj is analogous to 1/dt for cvode and daspk
for fixed step second order it is 2/dt and
for pure implicit fixed step it is 1/dt
It used to be static but is now a thread data variable
*/

void nrn_cap_jacob(NrnThread* _nt, Memb_list* ml) {
	int count = ml->nodecount;
	int i;
	double *vdata = ml->data;
	double cfac = .001 * _nt->cj;
	{ /*if (use_cachevec) {*/
		int* ni = ml->nodeindices;
		for (i=0; i < count; i++) {
			VEC_D(ni[i]) += cfac*cm;
		}
	}
}

static void cap_init(NrnThread* _nt, Memb_list* ml, int type ) {
	int count = ml->nodecount;
	double *vdata = ml->data;
	int i;
	(void)_nt; (void)type; /* unused */
	for (i=0; i < count; ++i) {
		i_cap = 0;
	}
}

void nrn_capacity_current(NrnThread* _nt, Memb_list* ml) {
	int count = ml->nodecount;
	double *vdata = ml->data;
	int i;
	double cfac = .001 * _nt->cj;
	/* since rhs is dvm for a full or half implicit step */
	/* (nrn_update_2d() replaces dvi by dvi-dvx) */
	/* no need to distinguish secondorder */
		int* ni = ml->nodeindices;
		for (i=0; i < count; i++) {
			i_cap = cfac*cm*VEC_RHS(ni[i]);
		}
}

/* the rest can be constructed automatically from the above info*/

static void cap_alloc(double* data, Datum* pdata, int type) {
	(void)pdata; (void)type; /* unused */
	data[0] = DEF_cm;	/*default capacitance/cm^2*/
}
