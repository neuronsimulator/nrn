COMMENT
Voltage clamp with three levels. Clamp is on at time 0, and off at time
dur[0]+dur[1]+dur[2]. When clamp is off the injected current is 0.
Do not insert several instances of this model at the same location in order to
make level changes. That is equivalent to independent clamps and they will
have incompatible internal state values.

The control amplifier has the indicated gain and time constant.  The
input amplifier is ideal.  

		 tau2
	         gain
                +-|\____rstim____>to cell
-amp --'\/`-------|/
                |
	        |----||---
	        |___    __|-----/|___from cell
		    `'`'        \|
		    tau1

The clamp has a three states which are the voltage input of the gain amplifier,
the voltage output of the gain amplfier, and the voltage output of the
measuring amplifier.
A good initial condition for these voltages are 0, 0, and v respectively.

This model is quite stiff.  For this reason the current is updated
within the solve block before updating the state of the clamp. This
gives the correct value of the current on exit from fadvance(). If we
didn't do this and
instead used the values computed in the breakpoint block, it 
would look like the clamp current is much larger than it actually is since it
doesn't take into account the change in voltage within the timestep, ie
equivalent to an almost infinite capacitance.
Also, because of stiffness, do not use this model except with secondorder=0.

This model makes use of implementation details of how models are interfaced
to neuron. At some point I will make the translation such that these kinds
of models can be handled straightforwardly.

Note that since this is an electrode current model v refers to the
internal potential which is equivalent to the membrane potential v when
there is no extracellular membrane mechanism present but is v+vext when
one is present.
Also since i is an electrode current,
positive values of i depolarize the cell. (Normally, positive membrane currents
are outward and thus hyperpolarize the cell)
ENDCOMMENT

DEFINE NSTEP 3

NEURON {
	POINT_PROCESS VClamp
	ELECTRODE_CURRENT i
	RANGE e0,vo0,vi0,dur,amp,gain,rstim,tau1,tau2,fac,i
}

UNITS {
	(nA) = (nanoamp)
	(mV) = (millivolt)
	(uS) = (microsiemens)
}


PARAMETER {
	dur[NSTEP] (ms)		<0, 1e9>
	amp[NSTEP] (mV)
	gain = 1e5		<0,1e9>
	rstim = 1 (megohm)	<1e-9,1e9>
	tau1 = .001 (ms)	<0,1e9>
	tau2 = 0   (ms)		<0,1e9>
	e0 (mV) vo0 (mV) vi0(mV)
	fac=0			<1,10>
}

ASSIGNED {
	v (mV)	: automatically v + vext when extracellular is present
	dt (ms)
	i (nA)
	stim (mV)
	tc (ms)
}


STATE {
	e (mV)
	vo (mV)
	vi (mV)
}

INITIAL {
	e0 = 0
	vo = v
	vo0 = v
	vi = v
	vi0 = v
	e = 0
	FROM j=0 TO NSTEP-1 { if (dur[j] > 0 && amp[j] != 0) {
		: nrn/lib/hoc/electrod.hoc always installs a VClamp
		: stopping cvode here if the clamp is on still allows
		: that tool to control a IClamp under cvode
		VERBATIM
		{extern int cvode_active_;
		if (cvode_active_) {
			hoc_execerror("VClamp", " does not work with CVODE");
		}}
		ENDVERBATIM
	}}
}

BREAKPOINT {
	SOLVE update METHOD after_cvode : but not really (see above)
	vstim()
	i = icur()
}

PROCEDURE vstim() { : can't be called from update since vinput must
			: remain constant throughout dt interval and
			: update is called at t+dt
	tc = 0 (ms)
	FROM j=0 TO NSTEP-1 {
		stim = amp[j]
		tc = tc + dur[j]
		if (t < tc) {
			tc = tc + 100	: clamp is definitely not off
			VERBATIM
			break;
			ENDVERBATIM
		}
	}
}

FUNCTION icur()(nA) {  : since this function uses range variables, it
			: should not be called from hoc. If you want to
			: know the current, look in i_vc
	LOCAL vout
	if (t > tc) {
		e0 = 0
		vo0 = 0
		icur = 0
	}else{
		SOLVE clamp
		icur = (vo - v)/rstim
	}
}

LINEAR clamp {
	LOCAL t1, t2
	t1 = tau1/dt
	t2 = tau2/dt
	~ vi = v + fac*vo - fac*v
	~ t2*vo - t2*vo0 + vo = -gain * e
	~ -stim - e  +  vi - e  +  t1*vi - t1*e - t1*(vi0 - e0)
	  = 0
}

PROCEDURE update() {
	i = icur()
	e0 = e
	vo0 = vo
	vi0 = vi
	VERBATIM
	return 0;
	ENDVERBATIM
}

COMMENT
This implementation is not very high level since the clamp uses a state which
must be computed at the same time as the membrane potential and doesn't fit
into the paradigm normally used for channel states.
The state, vinput0, at t is integrated from its old value saved in vinput.
The value of vinput (as well as the initial values of v and the output
of the control amplifier) is updated when the SOLVE
is executed.  Notice that the icur function is very stiff with respect to
v. For this reason i is only good for the user after the SOLVE is
executed.
ENDCOMMENT
