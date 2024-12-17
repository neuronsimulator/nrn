TITLE svclmp.mod
COMMENT
Single electrode Voltage clamp with three levels.
Clamp is on at time 0, and off at time
dur1+dur2+dur3. When clamp is off the injected current is 0.
The clamp levels are amp1, amp2, amp3.
i is the injected current, vc measures the control voltage)
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

INDEPENDENT {t FROM 0 TO 1 WITH 1 (ms)}

DEFINE NSTEP 3

NEURON {
	POINT_PROCESS SEClamp
	ELECTRODE_CURRENT i
	RANGE dur1, amp1, dur2, amp2, dur3, amp3, rs, vc, i
}

UNITS {
	(nA) = (nanoamp)
	(mV) = (millivolt)
	(uS) = (microsiemens)
}


PARAMETER {
	rs = 1 (megohm) <1e-9, 1e9>
	dur1 (ms) 	  amp1 (mV)
	dur2 (ms) <0,1e9> amp2 (mV)
	dur3 (ms) <0,1e9> amp3 (mV)
}

ASSIGNED {
	v (mV)	: automatically v + vext when extracellular is present
	i (nA)
	vc (mV)
	tc2 (ms)
	tc3 (ms)
	on
}

INITIAL {
	tc2 = dur1 + dur2
	tc3 = tc2 + dur3
	on = 0
}

BREAKPOINT {
	SOLVE icur METHOD after_cvode
	vstim()
}

PROCEDURE icur() {
	if (on) {
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
	on = 1
	if (dur1) {at_time(dur1)}
	if (dur2) {at_time(tc2)}
	if (dur3) {at_time(tc3)}
	if (t < dur1) {
		vc = amp1
	}else if (t < tc2) {
		vc = amp2
	}else if (t < tc3) {
		vc = amp3
	}else {
		vc = 0
		on = 0
	}
	icur()
}

