COMMENT
Test for derivimplicit solver with sympy and Eigen. Based on cacum.mod
ENDCOMMENT

NEURON {
	SUFFIX cacum
	USEION ca READ ica WRITE cai
	RANGE depth, tau, cai0
}

UNITS {
	(mM) = (milli/liter)
	(mA) = (milliamp)
	F = (faraday) (coulombs)
}

PARAMETER {
	depth = 1 (nm)	: assume volume = area*depth
	tau = 10 (ms)
	cai0 = 50e-6 (mM)	: Requires explicit use in INITIAL
			: block for it to take precedence over cai0_ca_ion
			: Do not forget to initialize in hoc if different
			: from this default.
}

ASSIGNED {
	ica (mA/cm2)
}

STATE {
	cai (mM)
}

INITIAL {
	cai = cai0
	extra_solve()
}

BREAKPOINT {
	SOLVE integrate METHOD derivimplicit
}

PROCEDURE extra_solve() {
    SOLVE integrate METHOD derivimplicit
}

DERIVATIVE integrate {
	cai' = -ica/depth/F/2 * (1e7) + (cai0 - cai)/tau
}
