TITLE sodium calcium exchange
: taken from jwm simulation

NEURON {
	SUFFIX nacax
	USEION ca READ eca WRITE ica
	USEION na READ ena WRITE ina
	RANGE k, ica, ina, ex
}

UNITS {
	(mA) = (milliamp)
	(mV) = (millivolt)
}

PARAMETER {
	k = .0002 (mho/cm2) <0,1e6>: at 6.3 deg. Q10 of 3
}

ASSIGNED {
	celsius (degC)
	v (mV)
	eca (mV)
	ena (mV)
	ica (mA/cm2)
	ina (mA/cm2)
	ex (mV)
}

LOCAL s_celsius, Q

BREAKPOINT {LOCAL kca
	if (s_celsius*1(degC) != celsius) {
		s_celsius = celsius
		Q = 3^((celsius - 6.3)/10 (degC))
	}		
	ex = 2*ena - eca
	ina = 2*k*Q*(v - ex)
	ica = -ina/2
}
