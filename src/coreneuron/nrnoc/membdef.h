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
