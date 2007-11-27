NEURON {
	SUFFIX capump
	USEION ca READ cai WRITE ica
	RANGE vmax, kmp, ica
}

UNITS {
	(uM) = (micro/liter)
	(mM) = (milli/liter)
	(mA) = (milliamp)
}

PARAMETER {
	vmax = .0667 (mA/cm2) <0, 1e6>: at 6.3 deg, Q10 = 3
	kmp = .2 (uM) <0, 1e6>
}

ASSIGNED {
	celsius (degC)
	ica (mA/cm2)
	cai (mM)
}

LOCAL Q, s_celsius

BREAKPOINT {
	if (s_celsius*1(degC) != celsius) {
		s_celsius = celsius
		Q = 3^((celsius - 6.3)/10 (degC))
	}		
	ica = vmax*Q*cai/(cai + (.001)*kmp) / 5.18
}
