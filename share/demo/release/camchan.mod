TITLE CaChan
: Calcium Channel with Goldman- Hodgkin-Katz permeability
: The fraction of open calcium channels has the same kinetics as
:   the HH m process but is slower by taufactor

UNITS {
	(molar) = (1/liter)
}

UNITS {
	(mV) =	(millivolt)
	(mA) =	(milliamp)
	(mM) =	(millimolar)
}

NEURON {
	SUFFIX cachan
	USEION ca READ cai, cao WRITE ica
	RANGE pcabar, ica
}

UNITS {
	:FARADAY = 96520 (coul)
	:R = 8.3134 (joule/degC)
	FARADAY = (faraday) (coulomb)
	R = (k-mole) (joule/degC)
}

PARAMETER {
	taufactor=2	<1e-6, 1e6>: Time constant factor relative to standard HH
	pcabar=.2e-7	(cm/s)	<0, 1e9>: Maximum Permeability
}

ASSIGNED {
	celsius		(degC)
	v		(mV)
	cai		(mM)
	cao		(mM)
	ica		(mA/cm2)
}

STATE {	oca }		: fraction of open channels

INITIAL {
	oca = oca_ss(v)
}

BREAKPOINT {
	SOLVE castate METHOD cnexp
	ica = pcabar*oca*oca*ghk(v, cai, cao)
}

DERIVATIVE castate {
	LOCAL inf, tau
	inf = oca_ss(v)  tau = oca_tau(v)
	oca' = (inf - oca)/tau
}

FUNCTION ghk(v(mV), ci(mM), co(mM)) (.001 coul/cm3) {
	LOCAL z, eci, eco
	z = (1e-3)*2*FARADAY*v/(R*(celsius+273.15))
	eco = co*efun(z)
	eci = ci*efun(-z)
	:high cao charge moves inward
	:negative potential charge moves inward
	ghk = (.001)*2*FARADAY*(eci - eco)
}

FUNCTION efun(z) {
	if (fabs(z) < 1e-4) {
		efun = 1 - z/2
	}else{
		efun = z/(exp(z) - 1)
	}
}

FUNCTION oca_ss(v(mV)) {
	LOCAL a, b
	TABLE FROM -150 TO 150 WITH 200
	
	v = v+65
	a = 1(1/ms)*efun(.1(1/mV)*(25-v))
	b = 4(1/ms)*exp(-v/18(mV))
	oca_ss = a/(a + b)
}

FUNCTION oca_tau(v(mV)) (ms) {
	LOCAL a, b, q
	TABLE DEPEND celsius, taufactor FROM -150 TO 150 WITH 200

	q = 3^((celsius - 6.3)/10 (degC))
	v = v+65
	a = 1(1/ms)*efun(.1(1/mV)*(25-v))
	b = 4(1/ms)*exp(-v/18(mV))
	oca_tau = taufactor/(a + b)
}

COMMENT
This model is related to the passive model in that it also describes
a membrane channel. However it involves two new concepts in that the
channel is ion selective and the conductance of the channel is
described by a state variable.

Since many membrane mechanisms involve specific ions whose concentration
governs a channel current (either directly or via a Nernst potential) and since
the sum of the ionic currents of these mechanisms in turn may govern
the concentration, it is necessary that NEURON be explicitly told which
ionic variables are being used by this model and which are being computed.
This is done by the USEION statement.  This statement uses the indicated
base name for an ion (call it `base') and ensures the existance of
four range variables that can be used by any mechanism that requests them
via the USEION statement. I.e. these variables are shared by the different
mechanisms.  The four variables are the current, ibase; the
equilibrium potential, ebase; the internal concentration, basei; and the
external concentration, baseo. (Note that Ca and ca would be distinct
ion species).  The READ part of the statement lists the subset of these
four variables which are needed as input to the this model's computations.
Any changes to those variables within this mechanism will be lost on exit.
The WRITE part of the statement lists the subset which are computed by
the present mechanism.  If the current is computed, then it's value
on exit will be added to the neuron wide value of ibase and will also
be added to the total membrane current that is used to calculate the
membrane potential.

When this model is `insert'ed, fcurrent() executes all the statements
of the EQUATION block EXCEPT the SOLVE statement. I.e. the states are
NOT integrated in time.  The fadvance() function executes the entire
EQUATION block including the SOLVE statement; thus the states are integrated
over the interval t to t+dt.

Notice that several mechanisms can WRITE to ibase; but it is an error
if several mechanisms (in the same section) WRITE to ebase, baseo, or basei.

This model makes use of several variables known specially to NEURON. They are
celsius, v, and t.  It implicitly makes use of dt.

TABLE refers to a special type of FUNCTION in which the value of the
function is computed by table lookup with linear interpolation of
the table entries.  TABLE's are recomputed automatically whenever a
variable that the table depends on (Through the DEPEND list; not needed
in these tables) is changed.
The TABLE statement indicates the minimum and maximum values of the argument
and the number of table entries.  From NEURON, the function oca_ss_cachan(v)
returns the proper value in the table. When the variable "usetable_cachan"
is set to 0, oca_ss_cachan(v)returns the true function value.
Thus the table error can be easily plotted.
ENDCOMMENT
