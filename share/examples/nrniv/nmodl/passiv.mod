TITLE passive membrane channel

UNITS {
	(mV) = (millivolt)
	(mA) = (milliamp)
}

NEURON {
	SUFFIX Passive
	NONSPECIFIC_CURRENT i
	RANGE g, erev
}

PARAMETER {
	g = .001	(mho/cm2)
	erev = -70	(mV)
}

ASSIGNED { i	(mA/cm2)}

BREAKPOINT {
	i = g*(v - erev)
}

COMMENT
The passive channel is very simple but illustrates several features of
the interface to NEURON. As a SCoP or hoc model the NEURON block is
ignored.  About the only thing you can do with this as an isolated channel
in SCoP is plot the current vs the potential. Notice that stand alone
models require
that all variables be declared, The calculation is done in the BREAKPOINT
block. The intended
semantics of the breakpoint block are that after the block is executed, ALL
variables are consistent with the value of the independent variable (t).
In this case, of course a trivial assignment statement suffices.
PARAMETER declares variables which generally do not change during
solution of the BREAKPOINT block and ASSIGNED declares variables which
get values via assignment statements (as opposed to STATE variables whose
values can only be determined by solving differential or simultaneous
algebraic equations.)  The values of PARAMETERs are the default values
and can be changed in SCoP.

The NEURON block serves as the interface to NEURON. One has to imagine
many models linked to NEURON at the same time. Therefore in order to
avoid conflicts with names of variables in other mechanisms a SUFFIX
is applied to all the declared names that are accessible from NEURON.
Accessible PARAMETERs are of two types. Those appearing in the
PARAMETER list become range variables that can be used in any section
in which the mechanism is "insert"ed.  PARAMETERs that do not appear in
the PARAMETER list become global scalars which are the same for every
section.  ASSIGNED variables and STATE variables also become range variables
that depend on position in a section.
NONSPECIFIC_CURRENT specifies a list of currents not associated with
any particular ion but computed by this model
that affect the calculation of the membrane potential. I.e. a nonspecific
current adds its contribution to the total membrane current.

The following  neuron program is suitable for investigating the behavior
of the channel and determining its effect on the membrane.
create a
access a
nseg = 1
insert Passive
g_Passive=.001
erev_Passive=0
proc cur() {
	axis(0,1,1,0,.001,1) axis()
	plot(1)
	for (v=0; v < 1; v=v+.01) {
		fcurrent()
		plot(v, i_Passive)
	}
	plt(-1)
}	

proc run() {
	axis(0,3,3,0,1,1) axis()
	t = 0
	v=1
	plot(1)
	while (t < 3) {
		plot(t,v)
		fadvance()
	}
}
/* the cur() procedure uses the fcurrent() function of neuron to calculate
all the currents and conductances with all states (including v) held
constant.  In the run() procedure fadvance() integrates all equations
by one time step. In this case the Passive channel in combination with
the default capacitance of 1uF/cm2 give a membrane with a time constant of
1 ms. Thus the voltage decreases exponentially toward 0 from its initial
value of 1.

ENDCOMMENT
