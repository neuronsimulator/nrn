COMMENT
/**
 * @file ProbAMPANMDA_EMS.mod
 * @brief 
 * @author king, muller, reimann, ramaswamy
 * @date 2011-08-17
 * @remark Copyright © BBP/EPFL 2005-2011; All rights reserved. Do not distribute without further notice.
 */
ENDCOMMENT

TITLE Probabilistic AMPA and NMDA receptor with presynaptic short-term plasticity 


COMMENT
AMPA and NMDA receptor conductance using a dual-exponential profile
presynaptic short-term plasticity as in Fuhrmann et al. 2002

_EMS (Eilif Michael Srikanth)
Modification of ProbAMPANMDA: 2-State model by Eilif Muller, Michael Reimann, Srikanth Ramaswamy, Blue Brain Project, August 2011
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
        POINT_PROCESS ProbAMPANMDA_EMS
        RANGE tau_r_AMPA, tau_d_AMPA, tau_r_NMDA, tau_d_NMDA
        RANGE Use, u, Dep, Fac, u0, mg, Rstate, tsyn_fac, u
        RANGE i, i_AMPA, i_NMDA, g_AMPA, g_NMDA, g, e, NMDA_ratio
        RANGE A_AMPA_step, B_AMPA_step, A_NMDA_step, B_NMDA_step
        NONSPECIFIC_CURRENT i
        BBCOREPOINTER rng
        RANGE synapseID, verboseLevel
}

PARAMETER {


        tau_r_AMPA = 0.2   (ms)  : dual-exponential conductance profile
        tau_d_AMPA = 1.7    (ms)  : IMPORTANT: tau_r < tau_d
        tau_r_NMDA = 0.29   (ms) : dual-exponential conductance profile
        tau_d_NMDA = 43     (ms) : IMPORTANT: tau_r < tau_d
        Use = 1.0   (1)   : Utilization of synaptic efficacy (just initial values! Use, Dep and Fac are overwritten by BlueBuilder assigned values) 
        Dep = 100   (ms)  : relaxation time constant from depression
        Fac = 10   (ms)  :  relaxation time constant from facilitation
        e = 0     (mV)  : AMPA and NMDA reversal potential
        mg = 1   (mM)  : initial concentration of mg2+
        gmax = .001 (uS) : weight conversion factor (from nS to uS)
        u0 = 0 :initial value of u, which is the running value of release probability
        synapseID = 0
        verboseLevel = 0
	NMDA_ratio = 0.71 (1) : The ratio of NMDA to AMPA
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
        i_AMPA (nA)
        i_NMDA (nA)
        g_AMPA (uS)
        g_NMDA (uS)
        g (uS)
        factor_AMPA
        factor_NMDA
        A_AMPA_step
        B_AMPA_step
        A_NMDA_step
        B_NMDA_step
        rng
        mggate

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

        A_AMPA       : AMPA state variable to construct the dual-exponential profile - decays with conductance tau_r_AMPA
        B_AMPA       : AMPA state variable to construct the dual-exponential profile - decays with conductance tau_d_AMPA
        A_NMDA       : NMDA state variable to construct the dual-exponential profile - decays with conductance tau_r_NMDA
        B_NMDA       : NMDA state variable to construct the dual-exponential profile - decays with conductance tau_d_NMDA
}

INITIAL{

        LOCAL tp_AMPA, tp_NMDA

        VERBATIM
          if (_p_rng)
          {
            nrnran123_setseq((nrnran123_State*)_p_rng, 0, 0);
          }
        ENDVERBATIM

	Rstate=1
	tsyn_fac=0
	u=u0
        
        A_AMPA = 0
        B_AMPA = 0
        
        A_NMDA = 0
        B_NMDA = 0
        
        tp_AMPA = (tau_r_AMPA*tau_d_AMPA)/(tau_d_AMPA-tau_r_AMPA)*log(tau_d_AMPA/tau_r_AMPA) :time to peak of the conductance
        tp_NMDA = (tau_r_NMDA*tau_d_NMDA)/(tau_d_NMDA-tau_r_NMDA)*log(tau_d_NMDA/tau_r_NMDA) :time to peak of the conductance
        
        factor_AMPA = -exp(-tp_AMPA/tau_r_AMPA)+exp(-tp_AMPA/tau_d_AMPA) :AMPA Normalization factor - so that when t = tp_AMPA, gsyn = gpeak
        factor_AMPA = 1/factor_AMPA
        
        factor_NMDA = -exp(-tp_NMDA/tau_r_NMDA)+exp(-tp_NMDA/tau_d_NMDA) :NMDA Normalization factor - so that when t = tp_NMDA, gsyn = gpeak
        factor_NMDA = 1/factor_NMDA

        A_AMPA_step = exp(dt*(( - 1.0 ) / tau_r_AMPA))
        B_AMPA_step = exp(dt*(( - 1.0 ) / tau_d_AMPA))
        A_NMDA_step = exp(dt*(( - 1.0 ) / tau_r_NMDA))
        B_NMDA_step = exp(dt*(( - 1.0 ) / tau_d_NMDA))
}

BREAKPOINT {

        SOLVE state
        mggate = 1 / (1 + exp(0.062 (/mV) * -(v)) * (mg / 3.57 (mM))) :mggate kinetics - Jahr & Stevens 1990
        g_AMPA = gmax*(B_AMPA-A_AMPA) :compute time varying conductance as the difference of state variables B_AMPA and A_AMPA
        g_NMDA = gmax*(B_NMDA-A_NMDA) * mggate :compute time varying conductance as the difference of state variables B_NMDA and A_NMDA and mggate kinetics
        g = g_AMPA + g_NMDA
        i_AMPA = g_AMPA*(v-e) :compute the AMPA driving force based on the time varying conductance, membrane potential, and AMPA reversal
        i_NMDA = g_NMDA*(v-e) :compute the NMDA driving force based on the time varying conductance, membrane potential, and NMDA reversal
        i = i_AMPA + i_NMDA
}

PROCEDURE state() {
        A_AMPA = A_AMPA*A_AMPA_step
        B_AMPA = B_AMPA*B_AMPA_step
        A_NMDA = A_NMDA*A_NMDA_step
        B_NMDA = B_NMDA*B_NMDA_step
}


NET_RECEIVE (weight,weight_AMPA, weight_NMDA, Psurv, tsyn (ms)){
        LOCAL result
        weight_AMPA = weight
        weight_NMDA = weight * NMDA_ratio
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
                         A_AMPA = A_AMPA + weight_AMPA*factor_AMPA
                         B_AMPA = B_AMPA + weight_AMPA*factor_AMPA
                         A_NMDA = A_NMDA + weight_NMDA*factor_NMDA
                         B_NMDA = B_NMDA + weight_NMDA*factor_NMDA
                         
                         if( verboseLevel > 0 ) {
                             printf( "Release! %f at time %g: vals %g %g %g %g\n", synapseID, t, A_AMPA, weight_AMPA, factor_AMPA, weight )
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
//printf("ProbAMPANMDA_EMS bbcore_write %d %d\n", di[0], di[1]);
	}
	*offset += 2;
}
static void bbcore_read(double* x, int* d, int* xx, int* offset, _threadargsproto_) {
	assert(!_p_rng);
	uint32_t* di = ((uint32_t*)d) + *offset;
        if (di[0] != 0 || di[1] != 0)
        {
	  nrnran123_State** pv = (nrnran123_State**)(&_p_rng);
	  *pv = nrnran123_newstream(di[0], di[1]);
        }
//printf("ProbAMPANMDA_EMS bbcore_read %d %d\n", di[0], di[1]);
	*offset += 2;
}
ENDVERBATIM


FUNCTION toggleVerbose() {
    verboseLevel = 1-verboseLevel
}
