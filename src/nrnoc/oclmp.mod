COMMENT
Clamp for optimization. This is derived from svclmp.mod but only
has an on, off, amp input

Single electrode Voltage clamp a start and stop time.
Clamp is on at time on, and off at time off
When clamp is off the injected current is 0.

i is the injected current, vc measures the control voltage
Do not insert several instances of this model at the same location in order to
make level changes. That is equivalent to independent clamps and they will
have incompatible internal state values.
The electrical circuit for the clamp is exceedingly simple:
vc ---'\/\/`--- cell
        rs

Note that since this is an electrode current model v refers to the
internal potential which is equivalent to the membrane potential v when
there is no extracellular membrane mechanism present but is v+vext when
one is present.
Also since i is an electrode current,
positive values of i depolarize the cell. (Normally, positive membrane currents
are outward and thus hyperpolarize the cell)
ENDCOMMENT

NEURON {
	POINT_PROCESS OClamp
	ELECTRODE_CURRENT i
	RANGE on, off, rs, vc, i, switched_on
}

UNITS {
	(nA) = (nanoamp)
	(mV) = (millivolt)
	(uS) = (microsiemens)
}


PARAMETER {
	rs = 1 (megohm) <1e-9, 1e9>
	on = 0 (ms)
	off = 0 (ms)
	vc (mV)
	switched_on = 0
}

ASSIGNED {
	v (mV)	: automatically v + vext when extracellular is present
	i (nA)
	onflag
}

INITIAL {
	onflag = 0
}

BREAKPOINT {
	SOLVE icur METHOD after_cvode
	vstim()
}

PROCEDURE icur() {
	if (onflag) {
		i = (vc - v)/rs
	}else{
		i = 0
	}
}

COMMENT
The SOLVE of icur() in the BREAKPOINT block is necessary to compute
i=(vc - v(t))/rs instead of i=(vc - v(t-dt))/rs
This is important for time varying vc because the actual i used in
the implicit method is equivalent to (vc - v(t)/rs due to the
calculation of di/dv from the BREAKPOINT block.
The reason this works is because the SOLVE statement in the BREAKPOINT block
is executed after the membrane potential is advanced.

It is a shame that vstim has to be called twice but putting the call
in a SOLVE block would cause playing a Vector into vc to be off by one
time step.
ENDCOMMENT

PROCEDURE vstim() {
	if (switched_on) {
		onflag = 1
		at_time(on)
		at_time(off)
		if (t < on) {
			onflag = 0
		}else if (t > off) {
			onflag = 0
		}
	}else{
		onflag = 0
	}
	icur()
}

