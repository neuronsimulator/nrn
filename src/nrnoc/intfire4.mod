COMMENT
Authors:    Original (taum > taui2 > taui1 > taue)   :   M. Hines and N.T. Carnevale
            Improved (taui2 > taue and taui2 > taui1):   R.A.J. van Elburg

Artificial cell with fast exponential decay excitatory current and
slower alpha function like inhibitory current and arbitrary membrane time constant.
Fires when membrane state = 1. On firing, only the membrane state returns to 0

Although the four states, e, i1, i2, m are available, plotting will be
more pleasing if one uses the function E, I, and M

Fast excitatory current, E(t), has single exponential decay with time constant, taue.

Slower inhibitory current, I2(t), is alpha function like and has two time contants,
taui1 and  taui2.

The leaky integrator membrane has time constant taum.
The total current is the input to this integrator.

Cell fires when state m passes 1
and after firing, m = 0.

Require  taui2 > taue and  taui2 > taui1
or in other words ki1 > ki2 and ke > ki2 

States are normalized so that an excitatory weight of 1 produces
a maximum e of 1 and a maximum m of 1
Also an inhibitory weight of -1 produces a maximum i2 of -1
and a maximum m of -1.

Basic equations:
 dE(t)/dt  = -ke*E(t)
 dI1(t)/dt = -ki1*I1(t)
 dI2(t)/dt = -ki2*I2(t) + ai1*I1(t)
 dM(t)/dt  = -km*M(t) + ae*E(t) + ai2*I2(t)

The initial conditions are e, i1, i2, and m

The analytic solution is

E(t) = e*exp(-ke*t)
I1(t) = i1*exp(-ki1*t)
I2(t) = i2*exp(-ki2*t) + bi1*i1*(exp(-ki2*t) - exp(-ki1*t))
M(t) = m * exp(-km*t)
	+ be*e*(exp(-km*t) - exp(-ke*t))
	+ bi2*i2*(exp(-km*t) - exp(-ki2*t))
	+ bi2*bi1*i1 * (exp(-km*t) - exp(-ki2*t))
	- bi2*bi1*i1*(ki2 - km)/(ki1 - km)*(exp(-km*t) - exp(-ki1*t))

The derivation of a lower bound on the first passage time based on Newton 
iteration is possible if we assume that taue<=taui2, i.e. that the 
inhibition outlasts excitation. The validity of the Newton iteration for setting 
a lower bound on threshold passage time follows from a physical analysis of the 
different contributing currents. The parameters taui1 are involved in controlling 
the time course of the growth of the inhibitory current. Therefore the taui1
associated current contributes to a growth of inhibition and can be ignored when trying to find a 
lower bound for the threshold passage time because it contributes only to its delay. 
The other contributions to the membrane potential change are the 
leak current associated with the membrane time constant taum and the 
exponentially decaying (parts) of the synaptic currents. The leak current is 
maximal close to threshold and can therefore only grow when approaching 
threshold. So if we use the present value in a calculation of threshold passage 
time while we are approaching threshold we will underestimate threshold passage 
time. The exponentially decaying (parts) of the synaptic currents consist of an 
excitatory and an inhibitory part, if we assume taue<taui2 we can see 
that the inhibitory parts outlast the excitatory currents and therefore if we 
assume that m changes according to the present values of these currents we 
underestimate threshold passage time. Because the sum of these currents is m'=
dm/dt and Newton iteration states that m(t)~m(t_0)+m'*(t-t_0) we find that 
Newton iteration underestimates threshold passsage time. If the membrane 
potential moves away from threshold toward rest, the excitatory current is 
smaller then leak current and inhibitory currents combined, because it decays 
faster then the inhibitory current it will also at future times not be able to 
overcome the leak current at the present membrane potential and hence not be 
able to rise above it and reach threshold.

A formal proof based on analysis of the exact solution of this mechanism is possible and can be found in:
    R.A.J. van Elburg and A. van Ooyen, `Generalization of the event-based Carnevale-Hines integration scheme
for integrate-and-fire models', ..., 2008/2009.
and is based on a formal proof for the original version which can be found in:
    Carnevale, N.T.  and Hines, M.L., `The NEURON Book',Cambridge University Press,Cambridge, UK (2006).
ENDCOMMENT

NEURON {
	ARTIFICIAL_CELL IntFire4
	RANGE taue, taui1, taui2, taum, e, i1, i2, m, ae, ai2, ai1
	RANGE nself, nexcite, ninhibit
	GLOBAL eps, taueps
}

PARAMETER {
	taue = 5 (ms)      <0,1e9>
	taui1 = 10 (ms)    <0,1e9>
	taui2 = 20 (ms)    <0,1e9>
	taum = 50 (ms)     <0,1e9>
	ib = 0
	eps = 1e-6         <1e-9,0.01>
	taueps = 0.01      <1e-9,1e9>
}

ASSIGNED {
	e i1 i2 m
	enew i1new i2new mnew
	t0 (ms)
	nself nexcite ninhibit

	ke (1/ms) ki1 (1/ms) ki2 (1/ms) km (1/ms)
	ae (1/ms) ai1 (1/ms) ai2 (1/ms)
	be bi1 bi2
	a b
	tau_swap (ms)
	flag (1)
}

PROCEDURE newstates(d(ms)) {
	LOCAL ee, ei1, ei2, em
	
	ee = exp(-ke*d)
	ei1 = exp(-ki1*d)
	ei2= exp(-ki2*d)
	em = exp(-km*d)
	enew = e*ee
	i1new = i1*ei1
	i2new = i2*ei2 + bi1*i1*(ei2 - ei1)
	mnew = m*em
		+ be*e*(em - ee)
		+ (bi2*i2 + a*i1)*(em - ei2)
		- b*i1*(em - ei1)
}

FUNCTION M() {
	newstates(t - t0)
	M = mnew
}

FUNCTION E() {
	newstates(t - t0)
	E = ae*enew
}

FUNCTION I() {
	newstates(t - t0)
	I = ai2*i2new
}

PROCEDURE update() {
	e = enew
	i1 = i1new
	i2 = i2new
	m = mnew
	t0 = t
}

PROCEDURE fixprecondition() {
    
	: and force assertion for correctness of algorithm
	: Preconditions:
    : taue  < taui2 
    : taui1 < taui2
    : To avoid division by zero errors, the need for alpha function [x*exp(-x)] 
    : solutions  and poor convergence we also impose
    : fabs(taux-tauy) >= taueps
    
    : The checks on the preconditions can lead to 3 consecutive shifts in taum 
    : and a smaller number of shifts in the other parameters to prevent this 
    : shift from bringing taum at or below zero we need to ensure 
    : that we keep sufficient distance from zero.
    if(taui2 < 4*taueps){
        taui2=4*taueps
    }
    if(taui1 < 3*taueps){
        taui1=3*taueps
    }
    
    if(taue < 2*taueps){
        taue=2*taueps
    }
    
    : taue  < taui2  
    if (taue > taui2) { 
        tau_swap=taue
        taue = taui2 - taueps 
        printf("Warning: Adjusted taue from %g  to %g  to ensure taue < taui2\n",tau_swap,taue)        
    } else if (taui2-taue < taueps){
        taue = taui2 - taueps 
    }

    : taui1 < taui2, after this taui2 is fixed
	if (taui1 > taui2) {
        tau_swap=taui2
        taui2=taui1
        taui1=tau_swap
        printf("Warning: Swapped taui1 and taui2\n")   
    }   
    
    : Avoid division by zero errors and poor convergence and impose
    : fabs(taux-tauy) >= taueps
    if (taui2-taui1 < taueps){          :  taui1 < taui2 --> taui2-taui1 is positive
        taui1 = taui2 - taueps 
    }

    if (taum <= taui2) {
        if (taui2 -taum < taueps){      :  (taum <= taui2) --> taui2 -taum is positive definite
            taum=taui2-taueps
        }
    
        if (fabs(taui1 -taum) < taueps){
            taum=taui1-taueps
        }
    
        if (fabs(taui1 -taum) < taueps){
            if(taui1 -taum < 0){
                taum=taui1-taueps
            } else{
                taui1=taum-taueps
            }
        }
    
        if (fabs(taue -taum) < taueps){
            if(taue -taum < 0){
                taum=taue-taueps
            } else{
                taue=taum-taueps
            }
        }
    
        if (fabs(taui1 -taum) < taueps){
            taum=taui1-taueps
        }
    
    } else if (taum-taui2 < taueps){    :  (taum > taui2) --> taum -taui2 is positive 
        taum=taui2+taueps
    }
}

PROCEDURE factors() {
	LOCAL tp
    
	ke=1/taue  ki1=1/taui1  ki2=1/taui2  km=1/taum

	: normalize so the peak magnitude of m is 1 when single e of 1
	tp = log(km/ke)/(km - ke)
	be = 1/(exp(-km*tp) - exp(-ke*tp))
	ae = be*(ke - km)
        :printf("INITIAL be=%g tp=%g \n", be, tp)
	
    : normalize so the peak magnitude of i2 is 1 when single i of 1
	tp = log(ki2/ki1)/(ki2 - ki1)
	bi1 = 1/(exp(-ki2*tp) - exp(-ki1*tp))
	ai1 = bi1*(ki1 - ki2)
    :printf("INITIAL bi1=%g tp=%g \n", bi1, tp)
	
    : normalize so the peak magnitude of m is 1 when single i of 1
	: first set up enough so we can use newstates()
	e = 0
	i1 = 1
	i2 = 0
	m = 0
	bi2 = 1
	ai2 = bi2*(ki2 - km)
	a = bi2*bi1
	b = a*(ki2 - km)/(ki1 - km)
	
    :find the 0 of dm/dt
	tp = search()
	
    : now compute normalization factor and reset constants that depend on it
	newstates(tp)
	bi2 = 1/mnew
	ai2 = bi2*(ki2 - km)
	a = bi2*bi1
	b = a*(ki2 - km)/(ki1 - km)
	
	: now newstates(tp) should set mnew=1
	newstates(tp)
    :printf("INITIAL bi2=%g tp=%g mnew=%g\n", bi2, tp, mnew)
	i1 = 0
}

FUNCTION deriv(d(ms)) (/ms2) { : proportional, not exact derivative but sign true (provided ki1 > ki2)
    deriv= ( - km *exp( - d*km ) + ki2*exp( - d*ki2))/(ki2 - km) -(( - km*exp( - d*km) + ki1*exp( - d*ki1)))/(ki1 - km )
}

FUNCTION search() (ms) {
	LOCAL x, t1, t2
	: should only do this when tau's change
	
    : exponential search for two initial t values for the chord search one factor
    : of 10 separated, with the low value t1 before (deriv(t1) < 0) and the high
    : value t2 after (deriv(t2) > 0) the extremal reached by m due to an isolated
    : i1 current. For this calculation the i1 current is taken to be depolarizing
    : and therefore the extremum is actually a maximum. During the simulation i1 is 
    : constrained to be hyperpolarizing and then the extremum will be a minimum 
    : occuring at the same location.
	t1=1
	flag=0
    if(deriv(t1) < 0  ){
        while ( deriv(t1) < 0 && t1>1e-9 ){
            t2=t1
            t1=t1/10
        }
        if (deriv(t1) < 0) { 
    		printf("Error wrong deriv(t1): t1=%g deriv(t1)=%g\n", t1, deriv(t1))
    		flag=1
            search = 1e-9
    	}
    }else{
        t2=t1
        while (deriv(t2) > 0 && t2<1e9 ){
            t1=t2
            t2=t2*10
        }
    	if (deriv(t2) > 0) { 
    		printf("Error wrong deriv(t2): t2=%g deriv(t2)=%g\n", t2, deriv(t2))
    		flag=1
    		search = 1e9
    	}
    }
    : printf("search  t1=%g x1=%g t2=%g x2=%g\n", t1, deriv(t1), t2, deriv(t2))
    
    
    : chord search for the  maximum in m(t) 
    while (t2 - t1 > 1e-6 && flag==0) {
		search = (t1+t2)/2
		x = deriv(search)
		if (x > 0) {
			t1 = search
		}else{
			t2 = search
		}
        :printf("search  t1=%g x1=%g t2=%g x2=%g\n", t1, deriv(t1), t2, deriv(t2))
	}			
}


INITIAL {
    
	fixprecondition()
	factors()
	e = 0
	i1 = 0
	i2 = 0
	m = 0
	t0 = t
	net_send(firetimebound(), 1)

	nself=0
	nexcite=0
	ninhibit=0
}

NET_RECEIVE (w) {
	newstates(t-t0)
	update()	
    :printf("event %g t=%g e=%g i1=%g i2=%g m=%g\n", flag, t, e, i1, i2, m)
	if (m > 1-eps) { : time to fire
        :printf("fire\n")
		net_event(t)
		m = 0
	}
	if (flag == 1) { :self event
		nself = nself + 1
		net_send(firetimebound(), 1)
	}else{
		if (w > 0) {
			nexcite = nexcite + 1
			e = e + w
		}else{
			ninhibit = ninhibit + 1
			i1 = i1 + w
		}
        :printf("w=%g e=%g i1=%g\n", w, e, i1)
		net_move(firetimebound() + t)
	}
}

FUNCTION firetimebound() (ms) {
	LOCAL slope
	slope = -km*m + ae*e + ai2*i2
	if (slope <= 1e-9) {
		firetimebound = 1e9
	}else{
		firetimebound = (1 - m)/slope
	}
}
