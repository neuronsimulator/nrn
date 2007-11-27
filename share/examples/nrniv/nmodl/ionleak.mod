NEURON {
	USEION na READ ena WRITE ina
	USEION k READ ek WRITE ik
	USEION ca READ eca WRITE ica
	RANGE gna, gk, gca
}

UNITS { (mV) = (millivolt)  (mA) = (milliamp) }

PARAMETER {
	gna (mho/cm2) gk(mho/cm2) gca(mho/cm2)
}

ASSIGNED {
	v (mV) ena (mV)  ek (mV)  eca (mV)
	ina (mA/cm2)  ik (mA/cm2)  ica (mA/cm2)
}

BREAKPOINT {
	ina = gna*(v - ena)
	ik = gk*(v - ek)
	ica = gca*(v - eca)
}

