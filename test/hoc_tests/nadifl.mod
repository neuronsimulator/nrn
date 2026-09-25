COMMENT
Longitudinal diffusion of sodium (no buffering)
(equivalent modified euler with standard method and
equivalent to diagonalized linear solver with CVODE )
ENDCOMMENT

NEURON {
	SUFFIX nadifl
	USEION na READ ina WRITE nai
	RANGE D
}

UNITS {
	(mM) = (milli/liter)
	(um) = (micron)
	FARADAY = (faraday) (coulomb)
	PI = (pi) (1)
}

PARAMETER {
	D = .6 (um2/ms)
}

ASSIGNED {
	ina (milliamp/cm2)
	diam (um)
}

STATE {
	nai (mM)
}

BREAKPOINT {
	SOLVE conc METHOD sparse
}

KINETIC conc {
	COMPARTMENT PI*diam*diam/4 {nai}
	LONGITUDINAL_DIFFUSION D*PI*diam*diam/4 {nai}
	~ nai << (-ina/(FARADAY)*PI*diam*(1e4))
}
