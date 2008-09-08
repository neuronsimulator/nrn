#include <../../nrnconf.h>
/* /local/src/master/nrn/src/nrnoc/clamp.c,v 1.2 1997/08/15 13:04:10 hines Exp */

/* modified from fstim.c */
/* 4/9/2002 modified to conform to new treeset.c */

/*
fclamp(maxlevel, loc)
 allocates space for maxlevel level changes all at location loc
 previously existing clamp is released.
 All clamp levels are initialized to clamp_resist series resistance.
 and 0 duration.  After the final level change the clamp is off.

fclamp(i, duration, vc)
 the maxlevel level changes concatenate. This function specifies
 the magnitude and duration of the ith level.

fclampv()
 the value of the clamp potential at the global time t
 
fclampi()
 the clamp current at the global time t

*/

#include "neuron.h"
#include "section.h"

static double loc;
static Node *pnd;
static Section* sec;
static double gtemp;
static maxlevel = 0;		/* size of clamp array */
static double *duration, *vc, *tswitch; /* maxlevel of them */
static int oldsw = 0;
static double clampval();

extern double *getarg(), chkarg();
extern char *secname();

#define nt_t nrn_threads->_t

print_clamp() {
	int i;
	
	if (maxlevel == 0) return;
	Printf("%s fclamp(%d, %g) /* Second arg is location */\n\
/* fclamp( #, duration(ms), magnitude(mV)) ; clamp_resist = %g */\n", secname(sec),
	maxlevel, loc, clamp_resist);
	for (i=0; i<maxlevel; i++) {
		Printf("   fclamp(%2d,%13g,%14g)\n", i, duration[i], vc[i]);
	}
}

fclampv() {

	if (maxlevel) {
		ret(clampval());
	}else{
		ret(0.);
	}
}

fclampi() {
	double v;
	
	if (maxlevel) {
		v = clampval();
		if (gtemp) {
			ret( -(NODEV(pnd) - v)/clamp_resist );
		}else{
			ret(0.);
		}
	}else{
		ret(0.);
	}
}
	
fclamp() {
	int i;

	if (nrn_nthread > 1) {
		hoc_execerror("fsyn does not allow threads", "");
	}
	/* two args are maxlevel, loc */
	/* three args are level, duration, vc */
	i = chkarg(1, 0., 10000.);
	if (ifarg(3)) {
		int num;
		if (i >= maxlevel) {
			hoc_execerror("level index out of range", (char *)0);
		}
		duration[i] = chkarg(2, 0., 1e21);
		vc[i] = *getarg(3);

		tswitch[0] = -1e-9;
		for (num = 0; num < maxlevel; num++) {
			tswitch[num+1] = tswitch[num] + duration[num];
		}
		oldsw = 0;
		ret(tswitch[maxlevel-1]);
		return;
	} else {
		free_clamp();
		maxlevel = i;
		if (maxlevel) {
			duration = (double *)emalloc((unsigned)(maxlevel * sizeof(double)));
			vc = (double *)emalloc((unsigned)(maxlevel * sizeof(double)));
			tswitch = (double *)emalloc((unsigned)((maxlevel + 1) * sizeof(double)));
			for (i = 0; i<maxlevel; i++) {
				duration[i] = 0.;
				vc[i] = 0.;
				tswitch[i] = -1e-9;
			}
			tswitch[maxlevel] = -1e-9;
			loc = chkarg(2, 0., 1.);
			sec = chk_access();
			section_ref(sec);
			clamp_prepare();
		}
	}
	ret(0.);
}

free_clamp() {
	if (maxlevel) {
		free((char *)duration);
		free((char *)vc);
		free((char *)tswitch);
		maxlevel = 0;
		section_unref(sec);
		sec = 0;
	}
}
	
clamp_prepare()	/*fill in the section info*/
{
	Node *node_ptr();
	double area;

	if (!maxlevel) {
		return;
	}
	if (sec->prop) {
		pnd = node_ptr(sec, loc, &area);
	}else{
		free_clamp();
		return;
	}
	if (clamp_resist <= 0) {
		hoc_execerror("clamp_resist must be > 0 in megohms", (char *)0);
	}
}

activclamp_rhs() {
	double v;
	if (!maxlevel) {
		return;
	}
	v = clampval();
#if EXTRACELLULAR
	if (pnd->extnode) {
		NODERHS(pnd) += gtemp*(v - NODEV(pnd) - pnd->extnode->v[0]);
	}else{
		NODERHS(pnd) += gtemp*(v - NODEV(pnd));	
	}
		
#else
	NODERHS(pnd) += gtemp*(v - NODEV(pnd));
#endif
}

activclamp_lhs() {
	double v;
	if (!maxlevel) {
		return;
	}
	NODED(pnd) += gtemp;
}

static double
clampval() {
	gtemp = 1.e2/clamp_resist/NODEAREA(pnd);
	for (;;) {
#if CVODE
		at_time(nrn_threads, tswitch[oldsw]);
#endif
		if (nt_t < tswitch[oldsw]) {
			if (oldsw == 0) {
				break;
			}
#if CVODE
/* for cvode the other was inefficient since t is non-monotonic */
			--oldsw;
#else
			oldsw = 0;
#endif
		}else{
			if (nt_t < tswitch[oldsw + 1]) {
				break;
			} else {
				if (++oldsw == maxlevel) {
					--oldsw;
					gtemp = 0.;
					break;
				}
			}
		}
	}
	return vc[oldsw];
}
