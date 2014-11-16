COMMENT
/**
 * @file ProbGABAAB.mod
 * @brief 
 * @author king, muller
 * @date 2011-08-17
 * @remark Copyright © BBP/EPFL 2005-2011; All rights reserved. Do not distribute without further notice.
 */
ENDCOMMENT

TITLE GABAAB receptor with presynaptic short-term plasticity 


COMMENT
GABAA receptor conductance using a dual-exponential profile
presynaptic short-term plasticity based on Fuhrmann et al, 2002
Implemented by Srikanth Ramaswamy, Blue Brain Project, March 2009

_EMS (Eilif Michael Srikanth)
Modification of ProbGABAA: 2-State model by Eilif Muller, Michael Reimann, Srikanth Ramaswamy, Blue Brain Project, August 2011
This new model was motivated by the following constraints:

1) No consumption on failure.  
2) No release just after release until recovery.
3) Same ensemble averaged trace as deterministic/canonical Tsodyks-Markram 
   using same parameters determined from experiment.
4) Same quantal size as present production probabilistic model.

To satisfy these constaints, the synapse is implemented as a
uni-vesicular (generalization to multi-vesicular should be
straight-forward) 2-state Markov process.  The states are
{1=recovered, 0=unrecovered}.

For a pre-synaptic spike or external spontaneous release trigger
event, the synapse will only release if it is in the recovered state,
and with probability u (which follows facilitation dynamics).  If it
releases, it will transition to the unrecovered state.  Recovery is as
a Poisson process with rate 1/Dep.

This model satisys all of (1)-(4).


ENDCOMMENT


NEURON {
    THREADSAFE
	POINT_PROCESS ProbGABAAB_EMS
	RANGE tau_r_GABAA, tau_d_GABAA, tau_r_GABAB, tau_d_GABAB 
	RANGE Use, u, Dep, Fac, u0, Rstate, tsyn_fac, u
	RANGE i,i_GABAA, i_GABAB, g_GABAA, g_GABAB, g, e_GABAA, e_GABAB, GABAB_ratio
        RANGE A_GABAA_step, B_GABAA_step, A_GABAB_step, B_GABAB_step
	NONSPECIFIC_CURRENT i
    BBCOREPOINTER rng
    RANGE synapseID, verboseLevel
}

PARAMETER {
	tau_r_GABAA  = 0.2   (ms)  : dual-exponential conductance profile
	tau_d_GABAA = 8   (ms)  : IMPORTANT: tau_r < tau_d
    tau_r_GABAB  = 3.5   (ms)  : dual-exponential conductance profile :Placeholder value from hippocampal recordings SR
	tau_d_GABAB = 260.9   (ms)  : IMPORTANT: tau_r < tau_d  :Placeholder value from hippocampal recordings 
	Use        = 1.0   (1)   : Utilization of synaptic efficacy (just initial values! Use, Dep and Fac are overwritten by BlueBuilder assigned values) 
	Dep   = 100   (ms)  : relaxation time constant from depression
	Fac   = 10   (ms)  :  relaxation time constant from facilitation
	e_GABAA    = -80     (mV)  : GABAA reversal potential
    e_GABAB    = -97     (mV)  : GABAB reversal potential
    gmax = .001 (uS) : weight conversion factor (from nS to uS)
    u0 = 0 :initial value of u, which is the running value of release probability
    synapseID = 0
    verboseLevel = 0
	GABAB_ratio = 0 (1) : The ratio of GABAB to GABAA
}

COMMENT
The Verbatim block is needed to generate random nos. from a uniform distribution between 0 and 1 
for comparison with Pr to decide whether to activate the synapse or not
ENDCOMMENT
   
VERBATIM
#include "nrnran123.h"
ENDVERBATIM
  

ASSIGNED {
	v (mV)
	i (nA)
        i_GABAA (nA)
        i_GABAB (nA)
        g_GABAA (uS)
        g_GABAB (uS)
        A_GABAA_step
        B_GABAA_step
        A_GABAB_step
        B_GABAB_step
	g (uS)
	factor_GABAA
        factor_GABAB
        rng

       : Recording these three, you can observe full state of model
       : tsyn_fac gives you presynaptic times, Rstate gives you 
	 : state transitions,
	 : u gives you the "release probability" at transitions 
	 : (attention: u is event based based, so only valid at incoming events)
       Rstate (1) : recovered state {0=unrecovered, 1=recovered}
       tsyn_fac (ms) : the time of the last spike
       u (1) : running release probability


}

STATE {
        A_GABAA       : GABAA state variable to construct the dual-exponential profile - decays with conductance tau_r_GABAA
        B_GABAA       : GABAA state variable to construct the dual-exponential profile - decays with conductance tau_d_GABAA
        A_GABAB       : GABAB state variable to construct the dual-exponential profile - decays with conductance tau_r_GABAB
        B_GABAB       : GABAB state variable to construct the dual-exponential profile - decays with conductance tau_d_GABAB
}

INITIAL{

        LOCAL tp_GABAA, tp_GABAB

        VERBATIM
          if (_p_rng)
          {
            nrnran123_setseq((nrnran123_State*)_p_rng, 0, 0);
          }
        ENDVERBATIM

	Rstate=1
	tsyn_fac=0
	u=u0
        
        A_GABAA = 0
        B_GABAA = 0
        
        A_GABAB = 0
        B_GABAB = 0
        
        tp_GABAA = (tau_r_GABAA*tau_d_GABAA)/(tau_d_GABAA-tau_r_GABAA)*log(tau_d_GABAA/tau_r_GABAA) :time to peak of the conductance
        tp_GABAB = (tau_r_GABAB*tau_d_GABAB)/(tau_d_GABAB-tau_r_GABAB)*log(tau_d_GABAB/tau_r_GABAB) :time to peak of the conductance
        
        factor_GABAA = -exp(-tp_GABAA/tau_r_GABAA)+exp(-tp_GABAA/tau_d_GABAA) :GABAA Normalization factor - so that when t = tp_GABAA, gsyn = gpeak
        factor_GABAA = 1/factor_GABAA
        
        factor_GABAB = -exp(-tp_GABAB/tau_r_GABAB)+exp(-tp_GABAB/tau_d_GABAB) :GABAB Normalization factor - so that when t = tp_GABAB, gsyn = gpeak
        factor_GABAB = 1/factor_GABAB
        A_GABAA_step = exp(dt*(( - 1.0 ) / tau_r_GABAA))
        B_GABAA_step = exp(dt*(( - 1.0 ) / tau_d_GABAA))
        A_GABAB_step = exp(dt*(( - 1.0 ) / tau_r_GABAB))
        B_GABAB_step = exp(dt*(( - 1.0 ) / tau_d_GABAB))
}

BREAKPOINT {
	SOLVE state
	
        g_GABAA = gmax*(B_GABAA-A_GABAA) :compute time varying conductance as the difference of state variables B_GABAA and A_GABAA
        g_GABAB = gmax*(B_GABAB-A_GABAB) :compute time varying conductance as the difference of state variables B_GABAB and A_GABAB 
        g = g_GABAA + g_GABAB
        i_GABAA = g_GABAA*(v-e_GABAA) :compute the GABAA driving force based on the time varying conductance, membrane potential, and GABAA reversal
        i_GABAB = g_GABAB*(v-e_GABAB) :compute the GABAB driving force based on the time varying conductance, membrane potential, and GABAB reversal
        i = i_GABAA + i_GABAB
}

PROCEDURE state() {
        A_GABAA = A_GABAA*A_GABAA_step
        B_GABAA = B_GABAA*B_GABAA_step
        A_GABAB = A_GABAB*A_GABAB_step
        B_GABAB = B_GABAB*B_GABAB_step
}


NET_RECEIVE (weight, weight_GABAA, weight_GABAB, Psurv, tsyn (ms)){
    LOCAL result
    weight_GABAA = weight
    weight_GABAB = weight*GABAB_ratio
    : Locals:
    : Psurv - survival probability of unrecovered state
    : tsyn - time since last surival evaluation.


    INITIAL{
		tsyn=t
    }

        : calc u at event-
        if (Fac > 0) {
                u = u*exp(-(t - tsyn_fac)/Fac) :update facilitation variable if Fac>0 Eq. 2 in Fuhrmann et al.
           } else {
                  u = Use  
           } 
           if(Fac > 0){
                  u = u + Use*(1-u) :update facilitation variable if Fac>0 Eq. 2 in Fuhrmann et al.
           }    

	   : tsyn_fac knows about all spikes, not only those that released
	   : i.e. each spike can increase the u, regardless of recovered state.
	   tsyn_fac = t

	   : recovery

	   if (Rstate == 0) {
	   : probability of survival of unrecovered state based on Poisson recovery with rate 1/tau
	          Psurv = exp(-(t-tsyn)/Dep)
		  result = urand()
		  if (result>Psurv) {
		         Rstate = 1     : recover      

                         if( verboseLevel > 0 ) {
                             printf( "Recovered! %f at time %g: Psurv = %g, urand=%g\n", synapseID, t, Psurv, result )
                         }

		  }
		  else {
		         : survival must now be from this interval
		         tsyn = t
                         if( verboseLevel > 0 ) {
                             printf( "Failed to recover! %f at time %g: Psurv = %g, urand=%g\n", synapseID, t, Psurv, result )
                         }
		  }
           }	   
	   
	   if (Rstate == 1) {
   	          result = urand()
		  if (result<u) {
		  : release!
   		         tsyn = t
			 Rstate = 0

                         A_GABAA = A_GABAA + weight_GABAA*factor_GABAA
                         B_GABAA = B_GABAA + weight_GABAA*factor_GABAA
                         A_GABAB = A_GABAB + weight_GABAB*factor_GABAB
                         B_GABAB = B_GABAB + weight_GABAB*factor_GABAB
                         
                         if( verboseLevel > 0 ) {
                             printf( "Release! %f at time %g: vals %g %g %g \n", synapseID, t, A_GABAA, weight_GABAA, factor_GABAA )
                         }
		  		  
		  }
		  else {
		         if( verboseLevel > 0 ) {
			     printf("Failure! %f at time %g: urand = %g\n", synapseID, t, result )
		         }

		  }

	   }

        

}


PROCEDURE setRNG() {
VERBATIM
    {
#if !NRNBBCORE
	nrnran123_State** pv = (nrnran123_State**)(&_p_rng);
	if (*pv) {
		nrnran123_deletestream(*pv);
		*pv = (nrnran123_State*)0;
	} 
	if (ifarg(2)) {
		*pv = nrnran123_newstream((uint32_t)*getarg(1), (uint32_t)*getarg(2));
	}
#endif
    }
ENDVERBATIM
}

FUNCTION urand() {
VERBATIM
	double value;
	if (_p_rng) {
		/*
		:Supports separate independent but reproducible streams for
		: each instance.
		*/
		value = nrnran123_negexp((nrnran123_State*)_p_rng);
		//printf("random stream for this simulation = %lf\n",value);
		return value;
	}else{
                value = 0.0;
//		assert(0);
	}
	_lurand = value;
ENDVERBATIM
}

VERBATIM
static void bbcore_write(double* x, int* d, int* xx, int* offset, _threadargsproto_) {
	if (d) {
		uint32_t* di = ((uint32_t*)d) + *offset;
		nrnran123_State** pv = (nrnran123_State**)(&_p_rng);
		nrnran123_getids(*pv, di, di+1);
//printf("ProbGABAAB_EMS bbcore_write %d %d\n", di[0], di[1]);
	}
        *offset += 2;
}
static void bbcore_read(double* x, int* d, int* xx, int* offset, _threadargsproto_) {
	assert(!_p_rng);
	uint32_t* di = ((uint32_t*)d) + *offset;
	nrnran123_State** pv = (nrnran123_State**)(&_p_rng);
	*pv = nrnran123_newstream(di[0], di[1]);
//printf("ProbGABAAB_EMS bbcore_read %d %d\n", di[0], di[1]);
	*offset += 2;
}
ENDVERBATIM

FUNCTION toggleVerbose() {
    verboseLevel = 1 - verboseLevel
    toggleVerbose = verboseLevel
}
