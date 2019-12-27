/* /local/src/master/nrn/src/nrnoc/membdef.h,v 1.2 1995/02/13 20:20:42 hines Exp */

/* numerical parameters */
#define DEF_nseg	1          /* default number of segments per section*/
#define DEF_dt		.025	   /* ms */
#define DEF_secondorder 0	   /* >0 means crank-nicolson. 2 means current
                                   adjusted to t+dt/2 */

/*global parameters */
#define DEF_Ra		35.4	   /* ohm-cm */	/*changed from 34.5 on 1/6/95*/
#define DEF_celsius	6.3	   /* deg-C */

#define DEF_vrest	-65.	   /* mV */

#if SEJNOWSKI
/* fsyn/fcon */
#define DEF_spikethresh	-10.	   /* mV (fcon) */
#define DEF_tablesize   200	   /* size of lookup table for alpha fxns (both) */
#define DEF_max_syn     10000	   /* max synapses per fsyn call */
#define DEF_max_con     1000	   /* sqrt of max connectivity for fcon */
#define MIN_DELAY       .5         /* min delay for randomly generated delays (fcon) */
#define MIN_GMAX        0.         /* min gmax for randomly generated gmax (both) */
#define MIN_ISI 	1.0        /* min interval between syn activation (msec) (both) */
#define QLEN		10	   /* max number of spikes in epspq (fcon) */
/* for use with flag() */
#define SYN_TYPE_FLAG   (int)1     /* Stimulus type (fsyn) */
#define TRAIN_FLAG      (int)2     /* Stimulus train (fsyn) */
#define DENSE_FLAG      (int)1     /* Randomize pre, post or both (fcon) */
#define SELF_CON_FLAG   (int)2     /* Allow for self connections (fcon) */
#define CON_TYPE_FLAG   (int)3     /* Stimulus type (fcon) */

/* used by parameter dump when dump_flag == 1. (see pdump.c) */
#define LINE_LEN        78         /* line length */
#define IFSEC_INDENT    4          /* how far to indent ifsec */
#define MECH_INDENT     22         /* how far from begin of line to indent mechs */
#define MAX_BLANK       MECH_INDENT + 10 /* max blanks stored for use in indenting */

#endif

/* old point process parameters */
/* fclamp */
#define DEF_clamp_resist 1e-3	   /* megohm */

/* Parameters that are used in mechanism _alloc() procedures */
/* cable */
#define DEF_L		100. 	   /* microns */
#define DEF_rallbranch	1.

/* morphology */
#define DEF_diam	500. 	   /* microns */

/* capacitance */
#define	DEF_cm		1.	   /* uF/cm^2 */

/* fast passive (e_p and g_p)*/
#define DEF_e		DEF_vrest  /* mV */
#define DEF_g		5.e-4	   /* S/cm^2 */

/* na_ion */
#define DEF_nai		10.	   /* mM */
#define DEF_nao		140.	   /* mM */
#define	DEF_ena		(115. + DEF_vrest) /* mV */

/* k_ion */
#define DEF_ki		54.4	   /* mM */
#define DEF_ko		2.5	   /* mM */
#define	DEF_ek		(-12. + DEF_vrest) /* mV */

/* ca_ion -> any program that uses DEF_eca must include <math.h> */
#define DEF_cai		5.e-5	   /* mM */
#define DEF_cao		2.	   /* mM */
#include <math.h>
#define	DEF_eca		12.5 *log(DEF_cao / DEF_cai) /* mV */ 

/* default ion values */
#define DEF_ioni	1.	   /* mM */
#define DEF_iono	1.	   /* mM */
#define DEF_eion	0.	   /* mV */
