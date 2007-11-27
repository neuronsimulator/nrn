:this model is built-in to neuron with suffix vc1

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

Note: The model in this file does not work when "extracellular" is
inserted at the same location.  For that case see
$NEURONHOME/src/nrnoc/vclmp.mod which does work in this circumstance. ie
that model makes use of the ELECTRODE_CURRENT directive in the NEURON
block which defines v to be the internal potential (ie v + vext) and
does not treat the current as a transmembrane current.

ENDCOMMENT


DEFINE NSTEP 3

NEURON {
	POINT_PROCESS vc1
	NONSPECIFIC_CURRENT I
	RANGE e0,vo0,vi0,dur,amp,gain,rstim,tau1,tau2,fac,I
}

UNITS {
	(nA) = (nanoamp)
	(mV) = (millivolt)
	(uS) = (micromho)
}


PARAMETER {
	v (mV)
	dt (ms)
	gain = 1e5
	rstim = 1 (megohm)
	tau1 = .001 (ms)
	tau2 = 0   (ms)
	e0 (mV) vo0 (mV) vi0(mV)
	fac=0
}

ASSIGNED {
	I (nA)
	dur[NSTEP] (ms)
	amp[NSTEP] (mV)
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
}

BREAKPOINT {
	SOLVE update
	vstim()
	I = icur()
}

PROCEDURE vstim() { : can't be called from update since vinput must
			: remain constant throughout dt interval and
			: update is called at t+dt
	tc = 0 (ms)
	FROM i=0 TO NSTEP-1 {
		stim = amp[i]
		tc = tc + dur[i]
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
			: know the current, look in I_vc[number]
	LOCAL vout
	if (t > tc) {
		e0 = 0
		vo0 = 0
		icur = 0
	}else{
		SOLVE clamp
		icur = -(vo - v)/rstim
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
	I = icur()
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
v. For this reason I is only good for the user after the SOLVE is
executed.
ENDCOMMENT
