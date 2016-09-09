COMMENT
Two state kinetic scheme synapse described by rise time tau1,
decay time constant tau2, and peak conductance gtrig.
Decay time MUST be greater than rise time. As they approach each other
it approaches an alpha function.

The solution of A->G->bath with rate constants 1/tau1 and 1/tau2 is
 A = a*exp(-t/tau1) and
 G = a*tau2/(tau2-tau1)*(-exp(-t/tau1) + exp(-t/tau2))
	where tau1 < tau2

(Notice if tau1 -> 0 then we have just single exponential decay.)
The factor is evaluated in the
initial block such that the peak conductance is 1.

Because the solution is a sum of exponentials, the
coupled equations can be solved as a pair of independent equations
by the more efficient cnexp method.

Specify an incremental delivery event
(synapse starts delay after the source crosses threshold.

The weight is proportional to the quantity of substance G which is
identical to conductance g but with its own Gtau1, Gtau1, and increments
by a factor such that peak G increases by Ginc on each spike (on a single line)
ENDCOMMENT

NEURON {
	POINT_PROCESS GSyn
	RANGE tau1, tau2, e, i
	RANGE Gtau1, Gtau2, Ginc
	NONSPECIFIC_CURRENT i

	RANGE g
}

UNITS {
	(nA) = (nanoamp)
	(mV) = (millivolt)
	(umho) = (micromho)
}

PARAMETER {
	tau1=0.1 (ms)
	tau2 = 1 (ms)
	Gtau1 = 20 (ms)
	Gtau2 = 21 (ms)
	Ginc = 1
	e=0	(mV)
}

ASSIGNED {
	v (mV)
	i (nA)
	g (umho)
	factor
	Gfactor
}

STATE {
	A (umho)
	B (umho)
}

INITIAL {
	LOCAL tp
	A = 0
	B = 0
	tp = (tau1*tau2)/(tau2 - tau1) * log(tau2/tau1)
	factor = -exp(-tp/tau1) + exp(-tp/tau2)
	factor = 1/factor
	tp = (Gtau1*Gtau2)/(Gtau2 - Gtau1) * log(Gtau2/Gtau1)
	Gfactor = -exp(-tp/Gtau1) + exp(-tp/Gtau2)
	Gfactor = 1/Gfactor
}

BREAKPOINT {
	SOLVE state METHOD cnexp
	g = B - A
	i = g*(v - e)
}

DERIVATIVE state {
	A' = -A/tau1
	B' = -B/tau2
}

NET_RECEIVE(weight (umho), w, G1, G2, t0 (ms)) {
	INITIAL { w = 0  G1 = 0  G2 = 0  t0 = t }
	G1 = G1*exp(-(t-t0)/Gtau1)
	G2 = G2*exp(-(t-t0)/Gtau2)
	G1 = G1 + Ginc*Gfactor
	G2 = G2 + Ginc*Gfactor
	t0 = t
	w = weight*(1 + G2 - G1)
	A = A + w*factor
	B = B + w*factor
}
